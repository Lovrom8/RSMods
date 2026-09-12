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

	/// <summary>
	/// Drops every cached layout. Call on device reset: the registry is keyed by raw shader
	/// pointer, and once the game releases its shaders a later allocation can reuse the same
	/// address and inherit an unrelated layout.
	/// </summary>
	inline void Forget() {
		std::lock_guard lock(registryMutex);
		currentEntry.store(nullptr, std::memory_order_relaxed);
		registry.clear();
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

		std::lock_guard lock(registryMutex);

		auto found = registry.find(shader);
		if (found == registry.end())
			found = registry.emplace(shader, Entry{ ParseLayout(shader) }).first;

		currentEntry.store(&found->second, std::memory_order_relaxed);
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
			if (FAILED(device->GetVertexShaderConstantF(entry.layout.registerIndex, matrix, 4)))
				return;

			if (!AspectRatio::IsAffine(matrix))
				return;

			const Sampled source = SampledSource();
			if (source == Sampled::SceneTarget || source == Sampled::Nothing)
				return;

			ApplyViewport(scale);
		}

		enum class Sampled { Nothing, Textures, StripTarget, SceneTarget };

		Sampled SampledSource() const {
			Sampled result = Sampled::Nothing;

			for (DWORD stage = 0; stage < 8; ++stage) {
				IDirect3DBaseTexture9* bound = nullptr;
				if (FAILED(device->GetTexture(stage, &bound)) || !bound)
					continue;

				const auto target = D3DHooks::ultrawideRenderTargetTextures.find(bound);
				bound->Release();

				if (target == D3DHooks::ultrawideRenderTargetTextures.end()) {
					if (result == Sampled::Nothing)
						result = Sampled::Textures;
				}
				else if (AspectRatio::SameAspect(target->second.first, target->second.second, D3DHooks::ultrawideBackBufferWidth, D3DHooks::ultrawideBackBufferHeight))
					return Sampled::SceneTarget;
				else
					result = Sampled::StripTarget;
			}

			return result;
		}

		void ApplyViewport(float scale) {
			if (FAILED(device->GetViewport(&savedViewport)))
				return;

			const unsigned int surfaceWidth = D3DHooks::ultrawideBackBufferWidth;

			D3DVIEWPORT9 remapped = savedViewport;
			int left = static_cast<int>(savedViewport.X);
			int right = static_cast<int>(savedViewport.X + savedViewport.Width);
			AspectRatio::RemapHorizontal(left, right, surfaceWidth, scale);
			remapped.X = static_cast<DWORD>(left);
			remapped.Width = static_cast<DWORD>(right - left);

			device->SetViewport(&remapped);
			viewportChanged = true;

			if (SUCCEEDED(device->GetScissorRect(&savedScissor))) {
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
