#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "D3DHooks.hpp"
#include "../AspectRatio.hpp"

/// <summary>
/// The 2D half of the ultrawide correction.
/// </summary>
namespace UltrawideShaders {

	inline constexpr unsigned int registerFileSize = 256;

	struct Entry {
		AspectRatio::ShaderLayout layout;
	};

	inline std::mutex registryMutex;
	inline std::unordered_map<IDirect3DVertexShader9*, Entry> registry;
	inline std::atomic<Entry*> currentEntry = nullptr;

	// The game rebinds the same shader for most SetVertexShader calls; remembering the last one
	// turns that into a pointer compare instead of a mutex and a hash lookup per bind.
	inline std::atomic<IDirect3DVertexShader9*> lastBoundShader = nullptr;
	inline std::atomic<Entry*> lastBoundEntry = nullptr;

	/// <summary>
	/// Drops every cached layout. Call on device reset: the registry is keyed by raw shader
	/// pointer, and once the game releases its shaders a later allocation can reuse the same
	/// address and inherit an unrelated layout.
	/// </summary>
	inline void Forget() {
		std::lock_guard lock(registryMutex);
		lastBoundShader.store(nullptr, std::memory_order_relaxed);
		lastBoundEntry.store(nullptr, std::memory_order_relaxed);
		currentEntry.store(nullptr, std::memory_order_relaxed);
		registry.clear();
	}

	// Leave a Matrix-classified draw at full width in Guitarcade. The 2D minigames use an
	// orthographic projection that passes IsAffine, so they were confined to 16:9 like an
	// interface element while the 3D ones filled the display. Scaleform draws are untouched.
	inline bool SkipSceneViewportClamp() {
		return D3DHooks::ultrawideInGuitarcade.load(std::memory_order_relaxed);
	}

	inline AspectRatio::ShaderLayout ParseLayout(IDirect3DVertexShader9* shader) {
		AspectRatio::ShaderLayout layout;

		UINT size = 0;
		if (!shader || FAILED(shader->GetFunction(NULL, &size)) || size == 0)
			return layout;

		std::vector<BYTE> bytecode(size);
		if (FAILED(shader->GetFunction(bytecode.data(), &size)))
			return layout;

		ID3DXConstantTable* table = nullptr;
		if (FAILED(D3DXGetShaderConstantTable(reinterpret_cast<const DWORD*>(bytecode.data()), &table)) || !table)
			return layout;

		D3DXCONSTANTTABLE_DESC description{};
		table->GetDesc(&description);

		if (description.Constants == 0)
			AspectRatio::MarkNoConstants(layout);

		for (UINT i = 0; i < description.Constants; ++i) {
			D3DXCONSTANT_DESC constants[4]{};
			UINT count = 4;

			if (FAILED(table->GetConstantDesc(table->GetConstant(NULL, i), constants, &count)))
				continue;

			for (UINT k = 0; k < count; ++k)
				if (constants[k].RegisterSet == D3DXRS_FLOAT4 && constants[k].Name)
					AspectRatio::AccumulateConstant(layout, constants[k].Name, constants[k].RegisterIndex, constants[k].RegisterCount);
		}

		table->Release();
		return layout;
	}

	inline void OnVertexShaderBound(IDirect3DVertexShader9* shader) {
		if (!shader) {
			currentEntry.store(nullptr, std::memory_order_relaxed);
			return;
		}

		if (shader == lastBoundShader.load(std::memory_order_relaxed)) {
			currentEntry.store(lastBoundEntry.load(std::memory_order_relaxed), std::memory_order_relaxed);
			return;
		}

		std::lock_guard lock(registryMutex);

		auto found = registry.find(shader);
		if (found == registry.end())
			found = registry.emplace(shader, Entry{ ParseLayout(shader) }).first;

		currentEntry.store(&found->second, std::memory_order_relaxed);
		lastBoundEntry.store(&found->second, std::memory_order_relaxed);
		lastBoundShader.store(shader, std::memory_order_relaxed);
	}

	// What the bound textures say about a draw: the scene target (a post-processing pass), an
	// off-aspect target (a 16:9 text strip), plain textures, or nothing.
	enum class Sampled { Nothing, Textures, StripTarget, SceneTarget };

	// Mirror of the device's texture stages, kept by Hook_SetTexture so the draw path does not
	// poll all 8 stages with GetTexture per draw. A bound texture can change meaning later (it
	// becomes a render target, or the backbuffer is resized); both bump classGeneration so the
	// cached verdicts are recomputed. Pointers are compared and used as keys, never dereferenced,
	// with the same recycled-address caveat as the render-target map.
	namespace TextureStages {
		inline constexpr DWORD count = 8;

		inline IDirect3DBaseTexture9* bound[count]{};
		inline Sampled verdict[count]{};
		inline unsigned int verdictGeneration[count]{};

		// Starts at 1 so the zero-initialised verdictGeneration entries above never look current.
		inline std::atomic<unsigned int> classGeneration = 1;

		inline void Invalidate() {
			classGeneration.fetch_add(1, std::memory_order_relaxed);
		}

		inline void Forget() {
			for (DWORD stage = 0; stage < count; ++stage) {
				bound[stage] = nullptr;
				verdict[stage] = Sampled::Nothing;
				verdictGeneration[stage] = 0;
			}

			Invalidate();
		}

		inline Sampled Classify(IDirect3DBaseTexture9* texture) {
			if (!texture)
				return Sampled::Nothing;

			const auto target = D3DHooks::ultrawideRenderTargetTextures.find(texture);
			if (target == D3DHooks::ultrawideRenderTargetTextures.end())
				return Sampled::Textures;

			return AspectRatio::SameAspect(target->second.first, target->second.second,
				D3DHooks::ultrawideBackBufferWidth, D3DHooks::ultrawideBackBufferHeight)
				? Sampled::SceneTarget
				: Sampled::StripTarget;
		}

		inline void OnTextureBound(DWORD stage, IDirect3DBaseTexture9* texture) {
			if (stage >= count)
				return;

			if (bound[stage] == texture)
				return;

			bound[stage] = texture;
			verdictGeneration[stage] = 0; // Force a reclassify on next use rather than doing it here.
		}

		// One answer per draw, same precedence as the old poll: scene target short-circuits,
		// off-aspect target outranks plain textures, plain textures only when nothing stronger.
		inline Sampled Fold() {
			const unsigned int generation = classGeneration.load(std::memory_order_relaxed);
			Sampled result = Sampled::Nothing;

			for (DWORD stage = 0; stage < count; ++stage) {
				if (!bound[stage])
					continue;

				if (verdictGeneration[stage] != generation) {
					verdict[stage] = Classify(bound[stage]);
					verdictGeneration[stage] = generation;
				}

				switch (verdict[stage]) {
				case Sampled::SceneTarget:
					return Sampled::SceneTarget;

				case Sampled::StripTarget:
					result = Sampled::StripTarget;
					break;

				case Sampled::Textures:
					if (result == Sampled::Nothing)
						result = Sampled::Textures;
					break;

				default:
					break;
				}
			}

			return result;
		}
	}

	/// <summary>
	/// Confines one draw to the 16:9 region if it belongs to the interface, and puts the device
	/// state back afterwards.
	/// </summary>
	class DrawScope {
	public:
		DrawScope(IDirect3DDevice9* device, unsigned int primitiveCount) : device(device), primitiveCount(primitiveCount) {}

		void Apply() {
			if (!D3DHooks::ultrawideActive.load(std::memory_order_relaxed))
				return;

			Entry* entry = currentEntry.load(std::memory_order_relaxed);
			if (!entry)
				return;

			if (!D3DHooks::ultrawideRenderTargetIsScene)
				return;

			const float scale = D3DHooks::ultrawideClipXScale;

			switch (entry->layout.kind) {
			case AspectRatio::LayoutKind::Matrix:
				if (!SkipSceneViewportClamp())
					ApplyMatrix(*entry, scale);
				break;

			case AspectRatio::LayoutKind::Scaleform:
			case AspectRatio::LayoutKind::ScreenSpace:
				ApplyViewport(scale);
				break;

			case AspectRatio::LayoutKind::PassThrough:
				if (AspectRatio::RemapPassThrough(primitiveCount) || SampledSource() == Sampled::StripTarget)
					ApplyViewport(scale);
				break;

			default:
				if (SampledSource() == Sampled::StripTarget)
					ApplyViewport(scale);
				break;
			}
		}

		~DrawScope() {
			if (viewportChanged)
				device->SetViewport(&savedViewport);

			if (scissorChanged)
				device->SetScissorRect(&savedScissor);
		}

		DrawScope(const DrawScope&) = delete;
		DrawScope& operator=(const DrawScope&) = delete;

	private:
		void ApplyMatrix(Entry& entry, float scale) {
			if (entry.layout.registerIndex + 4 > registerFileSize)
				return;

			float matrix[16];
			if (FAILED(device->GetVertexShaderConstantF(entry.layout.registerIndex, matrix, 4))) {
				return;
			}

			if (!AspectRatio::IsAffine(matrix)) {
				return;
			}

			const Sampled source = SampledSource();
			if (source == Sampled::SceneTarget || source == Sampled::Nothing) {
				return;
			}

			ApplyViewport(scale);
		}

		Sampled SampledSource() const {
			const Sampled result = TextureStages::Fold();

			return result;
		}

		void ApplyViewport(float scale) {
			if (FAILED(device->GetViewport(&savedViewport))) {
				return;
			}

			const unsigned int surfaceWidth = D3DHooks::ultrawideBackBufferWidth;

			D3DVIEWPORT9 remapped = savedViewport;
			int left = static_cast<int>(savedViewport.X);
			int right = static_cast<int>(savedViewport.X + savedViewport.Width);
			AspectRatio::RemapHorizontal(left, right, surfaceWidth, scale);
			remapped.X = static_cast<DWORD>(left);
			remapped.Width = static_cast<DWORD>(right - left);

			// A remap that lands on the values already set is not worth two D3D
			// calls (the Set here plus the restore in ~DrawScope) on every draw
			// that hits this path. Leaving viewportChanged false skips both.
			if (remapped.X != savedViewport.X || remapped.Width != savedViewport.Width) {
				device->SetViewport(&remapped);
				viewportChanged = true;
			}

			const HRESULT scissorResult = device->GetScissorRect(&savedScissor);

			if (SUCCEEDED(scissorResult)) {
				RECT remappedScissor = savedScissor;
				int scissorLeft = static_cast<int>(savedScissor.left);
				int scissorRight = static_cast<int>(savedScissor.right);
				AspectRatio::RemapHorizontal(scissorLeft, scissorRight, surfaceWidth, scale);
				remappedScissor.left = scissorLeft;
				remappedScissor.right = scissorRight;

				device->SetScissorRect(&remappedScissor);
				scissorChanged = true;
			}
		}

		IDirect3DDevice9* device;
		unsigned int primitiveCount;

		bool viewportChanged = false;
		D3DVIEWPORT9 savedViewport{};

		bool scissorChanged = false;
		RECT savedScissor{};
	};
}
