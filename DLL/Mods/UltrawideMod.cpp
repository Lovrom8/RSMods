#include "../stdafx.h"
#include "UltrawideMod.hpp"
#include "UltrawideRRDim.hpp"
#include "UltrawideShaders.hpp"
#include "UltrawideFullStage.hpp"
#include "UltrawideState.hpp"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
using Framework::Observe;
namespace DeviceEvent = Framework::DeviceEvent;
namespace Setting = Settings::Setting;

namespace {
	// Makes the game build its camera frustums at the display aspect instead of the 16:9 it
	// assumes.
	constexpr BYTE aspectLoadSignature[] = {
		0xD9, 0x44, 0x24, 0x1C,
		0xDD, 0x05, 0x00, 0x00, 0x00, 0x00,
		0xD8, 0xC9,
		0xD9, 0x5C, 0x24, 0x1C
	};
	constexpr char aspectLoadMask[] = "xxxxxx????xxxxxx";
	constexpr size_t aspectLoadOperandOffset = 6;
	constexpr double sixteenByNine = 16.0 / 9.0;

	double displayAspect = sixteenByNine;

	// Back to stock 16:9: no draw confinement, stock camera, HUD over the whole display.
	void PublishInactive() {
		UltrawideState::active.store(false);
		displayAspect = sixteenByNine;
		Framework::Hud().SetLayoutAspect(0.0f);
	}

	// Once per frame, so the per-draw and per-bind paths never touch Settings or query the swap chain.
	void UpdateState(IDirect3DDevice9* device) {
		using namespace UltrawideState;

		// Only widen. A narrower-than-16:9 display gives a scale above 1.0, which would push the
		// camera frustum and the remapped viewport outside the backbuffer, so the mod stays inert.
		const bool isActive = settingOn.load(std::memory_order_relaxed)
			&& backBufferValid && AspectRatio::IsWiderThanReference(clipXScale);

		active.store(isActive, std::memory_order_relaxed);

		// Once per frame: the query compares strings, and the draw path reads the
		// result per draw.
		inGuitarcade.store(GameState::Menus::IsInGuitarcadeGame(), std::memory_order_relaxed);
		inVideoPlayer.store(GameState::currentMenu == "VideoPlayer", std::memory_order_relaxed);
		// Full-stage bitmaps: the title backdrop is widened on the title screens (the "connecting"
		// SelectionListDialog included, the loft is not up yet there) and dropped under the
		// sign-in dialogs.
		UltrawideFullStage::onTitle.store(GameState::currentMenu.empty() || GameState::currentMenu == "pre_enter_prompt"
			|| GameState::currentMenu == "TitleScreen" || GameState::currentMenu == "SelectionListDialog", std::memory_order_relaxed);

		// Publish the backbuffer aspect only while the correction applies. Otherwise the game must
		// keep building its stock 16:9 frustum: the viewport and HUD paths are inert on a display
		// the gate rejected, and a camera that follows the display alone would drift from them.
		displayAspect = isActive && backBufferHeight
			? static_cast<double>(backBufferWidth) / backBufferHeight
			: AspectRatio::referenceAspect;

		if (backBufferValid || !device)
			return;

		IDirect3DSwapChain9* swapChain = nullptr;
		if (FAILED(device->GetSwapChain(0, &swapChain)) || !swapChain)
			return;

		D3DPRESENT_PARAMETERS presentParameters{};
		if (SUCCEEDED(swapChain->GetPresentParameters(&presentParameters))
			&& presentParameters.BackBufferWidth && presentParameters.BackBufferHeight) {

			clipXScale = AspectRatio::ClipXScale(presentParameters.BackBufferWidth, presentParameters.BackBufferHeight);
			backBufferWidth = presentParameters.BackBufferWidth;
			backBufferHeight = presentParameters.BackBufferHeight;
			backBufferValid = true;

			// Stage verdicts compare a target's size against the backbuffer aspect, so any cached
			// before this point was known was decided on the wrong comparison.
			UltrawideShaders::TextureStages::Invalidate();
		}

		swapChain->Release();
	}

	// Tracks whether the scene target is bound, so the correction can confine itself to draws that
	// actually reach the screen.
	void OnRenderTargetBound(const DeviceEvent::RenderTargetBound& e) {
		using namespace UltrawideState;

		if (e.index != 0)
			return;

		// One GetDesc serves both the render-target registry below and the scene test after it.
		D3DSURFACE_DESC targetDescription{};
		const bool haveDescription = e.target && SUCCEEDED(e.target->GetDesc(&targetDescription));

		if (haveDescription) {
			IDirect3DTexture9* container = nullptr;
			if (SUCCEEDED(e.target->GetContainer(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&container))) && container) {
				const auto existing = renderTargetTextures.find(container);
				const std::pair<UINT, UINT> size{ targetDescription.Width, targetDescription.Height };

				// A texture only just discovered to be a render target may already be sitting in a
				// texture stage classified as an ordinary texture, so retire the cached verdicts.
				if (existing == renderTargetTextures.end() || existing->second != size) {
					renderTargetTextures[container] = size;
					UltrawideShaders::TextureStages::Invalidate();
				}

				container->Release();
			}
		}

		bool isScene = false;
		UINT width = 0;
		UINT height = 0;

		if (haveDescription && backBufferValid) {
			width = targetDescription.Width;
			height = targetDescription.Height;

			isScene = AspectRatio::SameAspect(width, height, backBufferWidth, backBufferHeight);
		}

		renderTargetIsScene = isScene;
		renderTargetWidth = width;
		renderTargetHeight = height;
	}

	// Widens the game's 16:9 letterbox composite. The video player composites its frame through the
	// same path at its own aspect; leave it.
	void WidenLetterbox(DeviceEvent::StretchRect& e) {
		if (!UltrawideState::active.load(std::memory_order_relaxed) || UltrawideState::inVideoPlayer.load(std::memory_order_relaxed)
			|| !e.destRect || !e.dest)
			return;

		D3DSURFACE_DESC destinationDescription{};
		if (FAILED(e.dest->GetDesc(&destinationDescription)))
			return;

		const AspectRatio::Rect destination{ e.destRect->left, e.destRect->top, e.destRect->right, e.destRect->bottom };
		AspectRatio::Rect widened{};

		if (AspectRatio::WidenLetterbox(destination, destinationDescription.Width, destinationDescription.Height, widened)) {
			static RECT widenedRect; // Render thread; only has to outlive this dispatch.
			widenedRect = { widened.left, widened.top, widened.right, widened.bottom };
			e.destRect = &widenedRect;
		}
	}

	Framework::DrawResult ConfineDraw(Framework::DrawContext& ctx) {
		UltrawideShaders::DrawScope scope(ctx.device, ctx.mesh.PrimCount);
		scope.Apply();
		scope.RestoreAfter(ctx); // Registered before the full-stage restore, so it runs after it.

		// Full-stage bitmaps: the dim plate and the title backdrop are widened over the whole
		// backbuffer, the backdrops dropped under the sign-in dialogs. Confined draws only, so
		// inert at 16:9.
		if (ctx.path != Framework::DrawPath::Indexed)
			return { Framework::DrawOutcome::Pass };

		const auto decision = UltrawideFullStage::Decide(ctx.device, ctx.mesh.PrimCount, scope.Confined());
		switch (decision) {
		case UltrawideFullStage::Decision::Skip:
			return { Framework::DrawOutcome::Hide };
		case UltrawideFullStage::Decision::Stretch:
		case UltrawideFullStage::Decision::StretchWide:
			UltrawideFullStage::StretchViewport(ctx.device, UltrawideState::backBufferWidth,
				decision == UltrawideFullStage::Decision::Stretch);
			ctx.AfterDraw([device = ctx.device] { UltrawideFullStage::Restore(device); });
			return { Framework::DrawOutcome::Show };
		default:
			return { Framework::DrawOutcome::Pass };
		}
	}
}

SettingDefs UltrawideMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::Ultrawide, "Ultrawide", "Ultrawide")
			.Hint("Ultrawide (21:9 / 32:9) display support. The interface stays 16:9.")
	};
}

bool UltrawideMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::Ultrawide);
}

void UltrawideMod::OnInitialize(ModContext& c) {
	c.Draw().RegisterFrame([](IDirect3DDevice9* device) {
		UpdateState(device);

		// A disable can land while this runs. OnDisabled clears the setting flag before the state, so
		// re-checking the flag after publishing means whichever side finishes last leaves it off.
		std::atomic_thread_fence(std::memory_order_seq_cst);
		if (!UltrawideState::settingOn.load()) {
			PublishInactive();
			return;
		}

		Framework::Hud().SetLayoutAspect(UltrawideState::active.load(std::memory_order_relaxed)
			? static_cast<float>(sixteenByNine) : 0.0f);
	});

	// All paths: parts of the interface are drawn through the UP calls.
	c.Draw().Register("UltrawideConfine", Framework::DrawPriority::DeviceState, Framework::DrawPath::All, ConfineDraw);
	c.Draw().OnStretchRect(WidenLetterbox);

	// Mirrors of what is bound. They run while disabled too: enabling mid-frame must not start
	// from a stale texture stage or render-target map.
	c.Draw().OnRenderTargetBound(OnRenderTargetBound, Observe::Always);
	c.Draw().OnTextureBound([](DeviceEvent::TextureBound& e) {
		UltrawideShaders::TextureStages::OnTextureBound(e.stage, e.texture);
	}, Observe::Always);
	c.Draw().OnVertexShaderBound([](DeviceEvent::VertexShaderBound& e) {
		UltrawideShaders::OnVertexShaderBound(e.shader);
	}, Observe::Always);

	// The game recreates its shaders, textures and render targets across a reset, and these caches are
	// keyed by raw pointer. Runs whether or not the mod is enabled.
	c.Draw().RegisterDeviceReset([](IDirect3DDevice9*) {
		UltrawideState::backBufferValid = false;
		UltrawideState::renderTargetTextures.clear();
		UltrawideShaders::Forget();
		UltrawideShaders::TextureStages::Forget();
	});
}

void UltrawideMod::OnEnabled(ModContext&) {
	UltrawideState::settingOn = true;
	SetPatched(true);
	// Installed once and left in place; the gate below makes it inert when the correction
	// is not active, so toggling the setting never rewrites game code.
	UltrawideRRDim::Install();
}

void UltrawideMod::OnDisabled(ModContext&) {
	// The frame callback stops once disabled, so undo what it published here (flag first, see OnInitialize).
	UltrawideState::settingOn = false;
	std::atomic_thread_fence(std::memory_order_seq_cst);
	PublishInactive();
	SetPatched(false);
	UltrawideRRDim::SetActive(false);
}

void UltrawideMod::OnTick(ModContext&) {
	// UltrawideState::active is decided per frame on the render thread from the setting and the
	// backbuffer aspect, so it is the one flag that already means "corrections apply".
	UltrawideRRDim::SetActive(UltrawideState::active.load(std::memory_order_relaxed));
}

void UltrawideMod::OnShutdown(ModContext&) {
	UltrawideRRDim::Uninstall();
}

void UltrawideMod::SetPatched(bool enable) {
	if (enable == patched)
		return;

	if (!located) {
		const uintptr_t match = MemUtil::FindPattern<uintptr_t>(MemUtil::GetTextSectionAddress(), MemUtil::GetTextSectionLength(),
			const_cast<PBYTE>(aspectLoadSignature), aspectLoadMask);

		if (match) {
			const uintptr_t candidate = match + aspectLoadOperandOffset;
			const uint32_t constantAddress = *reinterpret_cast<const uint32_t*>(candidate);

			if (!MemUtil::IsBadReadPtr(reinterpret_cast<void*>(constantAddress))) {
				const double constant = *reinterpret_cast<const double*>(constantAddress);
				if (constant >= sixteenByNine - 0.01 && constant <= sixteenByNine + 0.01) {
					operandAddress = candidate;
					originalOperand = constantAddress;
					located = true; // Only latch on success, so a scan run before the text section is ready can retry.
				}
			}
		}
	}

	if (!operandAddress)
		return;

	const uint32_t operand = enable
		? static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&displayAspect))
		: originalOperand;

	if (MemUtil::PatchAdr(reinterpret_cast<LPVOID>(operandAddress), &operand, sizeof(operand)))
		patched = enable;
}

static Framework::ModRegistrar<UltrawideMod> _ultrawideReg;
