#include "../stdafx.h"
#include "PhraseBanner.hpp"
#include "../MemUtil.hpp"
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include "PhraseBanner/phrase_bars_vtf.h"
#include "PhraseBanner/phrase_progress_vtf.h"

/// <summary>
/// The phrase banner at the top of the screen stays empty for songs with more than 100 phrase iterations.
///
/// The banner's build function (0x0081C050 on the September 2022 build) refuses to build the banner when there are more
/// than 100 phrase iterations. The limit protects a fixed array of 109 float4 phrase slots at the start of the banner
/// object (the next field, the phrase list, starts at + 0x6D4). Every frame the whole array is uploaded to the two banner
/// vertex shaders (the bars and the progress overlay), which read it from shader constants c0 to c108.
/// The shaders also read the next phrase's start, so 108 phrases is the real ceiling. Removing the check on its own would
/// write past the array into the phrase list pointers.
///
/// This removes the limit without touching any game files:
///  - The three functions that write the array (all phrases, one phrase's height, one phrase's state) still run the
///    game's own code, but on a scratch copy of the banner, and we store the result in a list with no size limit.
///    The first 109 entries are still copied back into the banner, so the game's own upload stays correct.
///  - When a banner with more than 108 phrases is drawn, the two banner vertex shaders are swapped for copies that read
///    the phrase data from a small float texture (vertex texture fetch) instead of the 109 constants. They are rebuilt
///    from the shipped shader bytecode, and everything else they do is identical.
///  - Those shaders also draw phrases thinner than a few pixels edge to edge. The game's mesh starts every phrase
///    one pixel after its rounded down start and pulls colored bars in by another pixel per side. That is fine for 100
///    phrases, but with hundreds most phrases would have no width left and the banner would be mostly gaps.
///  - The build limit only goes up (to 4095, the texture width minus the extra "next phrase" slot) once the graphics card
///    supports vertex textures and our shaders were created. Until then it stays at 100, same as the game.
///
/// Banners with 108 phrases or fewer keep using the game's shaders.
/// </summary>
namespace PhraseBanner {

	// Banner object layout (same on both builds).
	constexpr uint32_t nativeSlots = 109;			// float4 phrase slots [109] at + 0x000
	constexpr uint32_t nativeSlotBytes = 0x6D0;
	constexpr uint32_t scratchFieldsStart = 0x6D0;	// Everything the three writers read lives in [0x6D0, 0x784)
	constexpr uint32_t bannerSize = 0x784;
	constexpr uint32_t phraseListBegin = 0x6D4;		// std::vector of 0x30 byte phrase entries
	constexpr uint32_t phraseListEnd = 0x6D8;
	constexpr uint32_t phraseEntrySize = 0x30;
	constexpr uint32_t buildItemSize = 0x18;		// One per phrase iteration in the data passed to the build function

	constexpr uint32_t gameMaxPhrases = 100;		// The build function: cmp eax, 0x64 / ja
	constexpr uint32_t phrasesPerPass = 100;		// Phrases per all-phrases write on the scratch copy
	constexpr uint32_t textureWidth = 4096;
	constexpr uint32_t widthTexel = textureWidth - 1;	// Last texel holds the banner width in pixels, for the shaders
	constexpr uint32_t maxPhrases = widthTexel - 1;		// The shader also reads the slot after the last phrase

	// The game's banner vertex shaders, FNV-1a over the bytecode. A modded shader won't match and is left alone.
	constexpr UINT barsShaderSize = 1356;
	constexpr uint32_t barsShaderHash = 0xF978DE1E;
	constexpr UINT progressShaderSize = 624;
	constexpr uint32_t progressShaderHash = 0x7E23CB26;

	struct Slot { float x, y, z, w; };

	struct BannerState {
		std::vector<Slot> slots;					// Phrase count + 1 (the shader reads the slot after the last phrase)
		uint8_t scratch[0x800] = {};	// Copy of the banner the game's code runs on
		IDirect3DTexture9* texture = nullptr;
		size_t uploadedSlots = 0;
		float width = 0.0f;							// Banner width in pixels (the screen width the game gives the bars shader)
		bool dirty = true;
	};

	enum class ShaderKind : uint8_t { Other, Bars, Progress };

	typedef void(__stdcall* tBuild)(void* banner, void* buildData);
	typedef void(__stdcall* tWritePhraseHeight)(void* banner, uint32_t index, void* phrase);

	tBuild oBuild = nullptr;
	void* oWriteAllPhrases = nullptr;						// EDI = banner
	tWritePhraseHeight oWritePhraseHeight = nullptr;
	void* oWritePhraseState = nullptr;				// ECX = phrase, EDX = index, ESI = banner, stack: int state, float time
	void* oRender = nullptr;							// EAX = banner

	std::mutex bannerMutex;
	std::unordered_map<uintptr_t, BannerState*> banners;
	thread_local bool runningOnScratch = false;			// Set while the game's code runs on a scratch copy, so our hooks pass straight through
	volatile uintptr_t currentBanner = 0;				// The banner whose Render is on the stack

	bool installed = false;
	bool triedD3DSetup = false;
	bool ready = false;									// Our shaders exist and the card supports vertex textures
	IDirect3DVertexShader9* barsShader = nullptr;
	IDirect3DVertexShader9* progressShader = nullptr;
	std::unordered_map<IDirect3DVertexShader9*, ShaderKind> shaderKinds;
	bool vertexTextureBound = false;

	uint32_t PhraseCount(const uint8_t* banner) {
		const uintptr_t begin = *reinterpret_cast<const uintptr_t*>(banner + phraseListBegin);
		const uintptr_t end = *reinterpret_cast<const uintptr_t*>(banner + phraseListEnd);
		return static_cast<uint32_t>((end - begin) / phraseEntrySize);
	}

	BannerState& GetState(const uint8_t* banner) {
		auto found = banners.find(reinterpret_cast<uintptr_t>(banner));
		if (found != banners.end())
			return *found->second;

		BannerState* state = new BannerState();
		banners.emplace(reinterpret_cast<uintptr_t>(banner), state);
		return *state;
	}

	void EnsureSlots(BannerState& state, size_t count) {
		if (state.slots.size() < count)
			state.slots.resize(count, Slot{});
	}

	/// <summary>
	/// Refresh the scratch copy with the fields the game's code reads (display mode, song length, render target, phrase list).
	/// </summary>
	uint8_t* PrepareScratch(BannerState& state, const uint8_t* banner) {
		memcpy(state.scratch + scratchFieldsStart, banner + scratchFieldsStart, bannerSize - scratchFieldsStart);
		return state.scratch;
	}

	void CallWriteAllPhrases(void* banner) {
		void* function = oWriteAllPhrases;
		__asm {
			push edi
			mov edi, banner
			call function
			pop edi
		}
	}

	void CallWritePhraseState(void* phrase, uint32_t index, void* banner, int animState, float time) {
		void* function = oWritePhraseState;
		__asm {
			push esi
			push dword ptr time
			push dword ptr animState
			mov ecx, phrase
			mov edx, index
			mov esi, banner
			call function
			pop esi
		}
	}

	void __cdecl WriteAllPhrases(uint8_t* banner) {
		if (runningOnScratch) {
			CallWriteAllPhrases(banner);
			return;
		}

		std::lock_guard lock(bannerMutex);
		BannerState& state = GetState(banner);
		const uint32_t count = PhraseCount(banner);

		state.slots.assign(count + 1, Slot{});

		if (count < nativeSlots) {
			// Fits the game's array: let it run as normal, then copy the result.
			runningOnScratch = true;
			CallWriteAllPhrases(banner);
			runningOnScratch = false;
			memcpy(state.slots.data(), banner, (count + 1) * sizeof(Slot));
		}
		else {
			// Too many for the game's array: run the game's code on the scratch copy, a window of the phrase list at a time.
			const uintptr_t firstPhrase = *reinterpret_cast<uintptr_t*>(banner + phraseListBegin);
			for (uint32_t first = 0; first < count; first += phrasesPerPass) {
				const uint32_t amount = std::min(phrasesPerPass, count - first);
				uint8_t* scratch = PrepareScratch(state, banner);
				*reinterpret_cast<uintptr_t*>(scratch + phraseListBegin) = firstPhrase + first * phraseEntrySize;
				*reinterpret_cast<uintptr_t*>(scratch + phraseListEnd) = firstPhrase + (first + amount) * phraseEntrySize;

				runningOnScratch = true;
				CallWriteAllPhrases(scratch);
				runningOnScratch = false;

				memcpy(&state.slots[first], scratch, amount * sizeof(Slot));
			}

			// Keep the game's array matching the first 109 slots, since the game still uploads it.
			memcpy(banner, state.slots.data(), nativeSlotBytes);
		}

		state.dirty = true;
	}

	void __stdcall WritePhraseHeightHook(uint8_t* banner, uint32_t index, void* phrase) {
		if (runningOnScratch) {
			oWritePhraseHeight(banner, index, phrase);
			return;
		}

		std::lock_guard lock(bannerMutex);
		BannerState& state = GetState(banner);
		uint8_t* scratch = PrepareScratch(state, banner);

		runningOnScratch = true;
		oWritePhraseHeight(scratch, 0, phrase);
		runningOnScratch = false;

		const float height = reinterpret_cast<Slot*>(scratch)[0].y;
		EnsureSlots(state, static_cast<size_t>(index) + 2);
		state.slots[index].y = height;
		if (index < nativeSlots)
			reinterpret_cast<Slot*>(banner)[index].y = height;
		state.dirty = true;
	}

	void __cdecl WritePhraseState(void* phrase, uint32_t index, uint8_t* banner, int animState, float time) {
		if (runningOnScratch) {
			CallWritePhraseState(phrase, index, banner, animState, time);
			return;
		}

		std::lock_guard lock(bannerMutex);
		BannerState& state = GetState(banner);
		uint8_t* scratch = PrepareScratch(state, banner);

		runningOnScratch = true;
		CallWritePhraseState(phrase, 0, scratch, animState, time);
		runningOnScratch = false;

		const Slot& result = reinterpret_cast<Slot*>(scratch)[0];
		EnsureSlots(state, static_cast<size_t>(index) + 2);
		state.slots[index].z = result.z;
		state.slots[index].w = result.w;
		if (index < nativeSlots) {
			reinterpret_cast<Slot*>(banner)[index].z = result.z;
			reinterpret_cast<Slot*>(banner)[index].w = result.w;
		}
		state.dirty = true;
	}

	void __stdcall BuildHook(void* banner, uint8_t* buildData) {
		const uintptr_t begin = *reinterpret_cast<uintptr_t*>(buildData + 0x8);
		const uintptr_t end = *reinterpret_cast<uintptr_t*>(buildData + 0xC);
		const uint32_t count = static_cast<uint32_t>((end - begin) / buildItemSize);

		// Over the limit the game returns without doing anything, so not calling it is the same thing.
		if (count > (ready ? maxPhrases : gameMaxPhrases))
			return;

		oBuild(banner, buildData);
	}

	// All phrases: EDI = banner, no stack arguments, ret. The game's version changes EAX, ECX and EDX.
	void __declspec(naked) WriteAllPhrasesHook() {
		__asm {
			push edi
			call WriteAllPhrases
			add esp, 4
			ret
		}
	}

	// One phrase's state: ECX = phrase, EDX = index, ESI = banner, [esp+4] state, [esp+8] time, ret 8.
	// The game's version keeps ECX, EBX, ESI and EDI, and changes EAX and EDX.
	void __declspec(naked) WritePhraseStateHook() {
		__asm {
			push ecx
			push dword ptr [esp + 0xC]		// time
			push dword ptr [esp + 0xC]		// state
			push esi						// banner
			push edx						// index
			push ecx						// phrase
			call WritePhraseState
			add esp, 0x14
			pop ecx
			ret 8
		}
	}

	// Render: EAX = banner, no stack arguments, ret. Remember which banner is drawing, so the
	// SetVertexShader hook knows whose phrase texture to use.
	void __declspec(naked) RenderHook() {
		__asm {
			push dword ptr [currentBanner]
			mov dword ptr [currentBanner], eax
			call dword ptr [oRender]
			pop dword ptr [currentBanner]
			ret
		}
	}

	uint32_t Fnv1a(const uint8_t* data, size_t size) {
		uint32_t hash = 0x811C9DC5;
		for (size_t i = 0; i < size; i++)
			hash = (hash ^ data[i]) * 0x01000193;
		return hash;
	}

	ShaderKind Classify(IDirect3DVertexShader9* shader) {
		auto found = shaderKinds.find(shader);
		if (found != shaderKinds.end())
			return found->second;

		ShaderKind kind = ShaderKind::Other;
		UINT size = 0;
		if (SUCCEEDED(shader->GetFunction(nullptr, &size)) && (size == barsShaderSize || size == progressShaderSize)) {
			std::vector<uint8_t> bytecode(size);
			if (SUCCEEDED(shader->GetFunction(bytecode.data(), &size))) {
				const uint32_t hash = Fnv1a(bytecode.data(), size);
				if (size == barsShaderSize && hash == barsShaderHash)
					kind = ShaderKind::Bars;
				else if (size == progressShaderSize && hash == progressShaderHash)
					kind = ShaderKind::Progress;
			}
		}

		shaderKinds.emplace(shader, kind);
		return kind;
	}

	void SetupD3D(IDirect3DDevice9* pDevice) {
		triedD3DSetup = true;

		D3DCAPS9 caps;
		if (FAILED(pDevice->GetDeviceCaps(&caps)) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0)) {
			LOG_WARNING("(PHRASE BANNER) No vs_3_0, keeping the 100 phrase limit" << std::endl);
			return;
		}

		bool vertexTextures = false;
		IDirect3D9* d3d = nullptr;
		if (SUCCEEDED(pDevice->GetDirect3D(&d3d)) && d3d) {
			D3DDEVICE_CREATION_PARAMETERS parameters;
			D3DDISPLAYMODE mode;
			if (SUCCEEDED(pDevice->GetCreationParameters(&parameters)) && SUCCEEDED(pDevice->GetDisplayMode(0, &mode)))
				vertexTextures = SUCCEEDED(d3d->CheckDeviceFormat(parameters.AdapterOrdinal, parameters.DeviceType, mode.Format,
					D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F));
			d3d->Release();
		}

		if (!vertexTextures) {
			LOG_WARNING("(PHRASE BANNER) Graphics card doesn't support float vertex textures, keeping the 100 phrase limit" << std::endl);
			return;
		}

		if (FAILED(pDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(g_phraseBarsVtfVS), &barsShader)) ||
			FAILED(pDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(g_phraseProgressVtfVS), &progressShader))) {
			LOG_ERROR("(PHRASE BANNER) Couldn't create our banner shaders, keeping the 100 phrase limit" << std::endl);
			if (barsShader) {
				barsShader->Release();
				barsShader = nullptr;
			}
			return;
		}

		ready = true;
		LOG_INFO("(PHRASE BANNER) Phrase banner supports up to " << maxPhrases << " phrases" << std::endl);
	}

	/// <summary>
	/// Create or refresh the banner's phrase texture. Caller holds bannerMutex.
	/// </summary>
	bool UploadPhraseTexture(IDirect3DDevice9* pDevice, BannerState& state) {
		if (!state.texture) {
			if (FAILED(pDevice->CreateTexture(textureWidth, 1, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &state.texture, nullptr))) {
				state.texture = nullptr;
				return false;
			}
			state.uploadedSlots = textureWidth; // New texture, so clear all of it on the first upload
			state.dirty = true;
		}

		if (!state.dirty)
			return true;

		D3DLOCKED_RECT locked;
		if (FAILED(state.texture->LockRect(0, &locked, nullptr, 0)))
			return false;

		Slot* texels = static_cast<Slot*>(locked.pBits);
		const size_t slots = std::min(state.slots.size(), static_cast<size_t>(widthTexel));
		memcpy(texels, state.slots.data(), slots * sizeof(Slot));
		if (state.uploadedSlots > slots)
			memset(texels + slots, 0, (state.uploadedSlots - slots) * sizeof(Slot));
		texels[widthTexel] = Slot{ state.width, 0.0f, 0.0f, 0.0f };
		state.texture->UnlockRect(0);

		state.uploadedSlots = slots;
		state.dirty = false;
		return true;
	}

	/// <summary>
	/// The width in pixels the game lays the banner out in: 1024 for the menu banners,
	/// which are drawn into a texture, otherwise the resolution width. EAX = banner, result in ST0.
	/// </summary>
	float BannerWidth(void* banner) {
		void* function = reinterpret_cast<void*>(Offsets::ptr_phraseBannerScreenWidth.Get());
		float width;
		__asm {
			mov eax, banner
			call function
			fstp width
		}
		return width;
	}

	void UnbindVertexTexture(IDirect3DDevice9* pDevice) {
		if (vertexTextureBound) {
			pDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0, nullptr);
			vertexTextureBound = false;
		}
	}

	IDirect3DVertexShader9* OnSetVertexShader(IDirect3DDevice9* pDevice, IDirect3DVertexShader9* shader) {
		if (!installed || !pDevice)
			return shader;

		if (!triedD3DSetup)
			SetupD3D(pDevice);

		const ShaderKind kind = (shader && ready) ? Classify(shader) : ShaderKind::Other;
		const uintptr_t banner = currentBanner;
		if (kind == ShaderKind::Other || !banner) {
			UnbindVertexTexture(pDevice);
			return shader;
		}

		std::lock_guard lock(bannerMutex);
		auto found = banners.find(banner);

		if (found != banners.end() && found->second->slots.size() > nativeSlots) {
			const float width = BannerWidth(reinterpret_cast<void*>(banner));
			if (width != found->second->width) {
				found->second->width = width;
				found->second->dirty = true;
				LOG_INFO("(PHRASE BANNER) Drawing " << found->second->slots.size() - 1 << " phrases on a " << width << " pixel wide banner" << std::endl);
			}
		}

		// Banners that fit the game's array keep the game's shader.
		if (found == banners.end() || found->second->slots.size() <= nativeSlots || !UploadPhraseTexture(pDevice, *found->second)) {
			UnbindVertexTexture(pDevice);
			return shader;
		}

		pDevice->SetTexture(D3DVERTEXTEXTURESAMPLER0, found->second->texture);
		pDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		pDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		pDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		pDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		pDevice->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		vertexTextureBound = true;


		return (kind == ShaderKind::Bars) ? barsShader : progressShader;
	}

	void OnDeviceReset() {
		shaderKinds.clear();
		vertexTextureBound = false; // A reset unbinds every texture
	}

	void Install() {
		// Whole instructions, none relative (see the prologues next to the offsets).
		oWriteAllPhrases = MemUtil::TrampHook(reinterpret_cast<PBYTE>(Offsets::ptr_phraseBannerWriteAllPhrases.Get()), reinterpret_cast<PBYTE>(WriteAllPhrasesHook), 6);
		oWritePhraseHeight = reinterpret_cast<tWritePhraseHeight>(MemUtil::TrampHook(reinterpret_cast<PBYTE>(Offsets::ptr_phraseBannerWritePhraseHeight.Get()), reinterpret_cast<PBYTE>(WritePhraseHeightHook), 9));
		oWritePhraseState = MemUtil::TrampHook(reinterpret_cast<PBYTE>(Offsets::ptr_phraseBannerWritePhraseState.Get()), reinterpret_cast<PBYTE>(WritePhraseStateHook), 6);
		oRender = MemUtil::TrampHook(reinterpret_cast<PBYTE>(Offsets::ptr_phraseBannerRender.Get()), reinterpret_cast<PBYTE>(RenderHook), 6);

		if (!oWriteAllPhrases || !oWritePhraseHeight || !oWritePhraseState || !oRender) {
			// The build limit is still in place, so the hooks that did go in only ever see banners that fit.
			LOG_ERROR("(PHRASE BANNER) Couldn't hook the banner, keeping the 100 phrase limit" << std::endl);
			return;
		}

		oBuild = reinterpret_cast<tBuild>(MemUtil::TrampHook(reinterpret_cast<PBYTE>(Offsets::ptr_phraseBannerBuild.Get()), reinterpret_cast<PBYTE>(BuildHook), 6));
		if (!oBuild) {
			LOG_ERROR("(PHRASE BANNER) Couldn't hook the banner build, keeping the 100 phrase limit" << std::endl);
			return;
		}

		// BuildHook applies the limit from now on (100 until our shaders are ready, then 4095).
		MemUtil::PatchAdr(Offsets::ptr_phraseBannerBuildLimitJump, "\x90\x90\x90\x90\x90\x90", 6);

		installed = true;
		LOG_INFO("(PHRASE BANNER) Installed" << std::endl);
	}
}
