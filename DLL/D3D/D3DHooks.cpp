#include "../stdafx.h"
#include "D3DHooks.hpp"
#include "../Framework/Framework.hpp"
#include "../AspectRatio.hpp"
#include "UltrawideShaders.hpp"
#include "UltrawideFullStage.hpp"
#include "../Mods/UltrawideMod.hpp"
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;

void D3DHooks::InitializeCrcProvider() {
	Framework::DrawContext::SetDefaultCrcProvider([](IDirect3DDevice9* device, DWORD stage) -> std::optional<DWORD> {
		if (!device) return std::nullopt;
		LPDIRECT3DBASETEXTURE9 pBase = nullptr;
		if (FAILED(device->GetTexture(stage, &pBase)) || !pBase) return std::nullopt;
		DWORD crcVal = 0;
		const bool ok = D3D::CRCForTexture(reinterpret_cast<LPDIRECT3DTEXTURE9>(pBase), device, crcVal);
		pBase->Release();
		return ok ? std::optional<DWORD>(crcVal) : std::nullopt;
	});
}

/// <summary>
/// IDirect3DDevice9::DrawPrimitive Middleware. Mainly used for Note Tails
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="PrimType"> - Member of the D3DPRIMITIVETYPE enumerated type, describing the type of primitive to render.</param>
/// <param name="StartIndex"> - Index of the first vertex to load.</param>
/// <param name="PrimCount"> - Number of primitives to render.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. If the method fails, the return value can be D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_DP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT StartIndex, UINT PrimCount) { // Mainly used for Note Tails
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	// Declared before the DrawContext so its viewport restore runs after the interceptors' AfterDraw restores.
	UltrawideShaders::DrawScope ultrawideScope(pDevice, PrimCount);
	ultrawideScope.Apply();

	Mesh currentMesh(Stride, PrimCount, 0);
	ThiccMesh currentThicc(Stride, PrimCount, 0, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements);
	DrawContext ctx{ pDevice, currentMesh, currentThicc, DrawPath::Primitive, GameState::IsInSong() };
	auto active = Framework::Draw().ActiveSnapshot(DrawPath::Primitive);
	for (const auto& e : *active) {
		DrawResult r = e.fn(ctx);
		switch (r.outcome) {
			case DrawOutcome::Hide: return REMOVE_TEXTURE;
			case DrawOutcome::Show: return oDrawPrimitive(pDevice, PrimType, StartIndex, PrimCount);
			case DrawOutcome::ReplaceTexture: pDevice->SetTexture(r.stage, r.texture); break;
			case DrawOutcome::Pass: break;
		}
	}

	// Call the original DrawPrimitive.
	return oDrawPrimitive(pDevice, PrimType, StartIndex, PrimCount);
}

/// <summary>
/// IDirect3DDevice9::DrawPrimitiveUP Middleware. Exists so the ultrawide correction covers every
/// draw entry point; the game's own behaviour passes through untouched.
/// </summary>
HRESULT APIENTRY D3DHooks::Hook_DrawPrimitiveUP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT PrimCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
	UltrawideShaders::DrawScope ultrawideScope(pDevice, PrimCount);
	ultrawideScope.Apply();

	if (auto active = Framework::Draw().ActiveSnapshot(DrawPath::PrimitiveUP); !active->empty()) {
		Mesh currentMesh(VertexStreamZeroStride, PrimCount, 0);
		ThiccMesh currentThicc(VertexStreamZeroStride, PrimCount, 0, 0, StartRegister, PrimType, decl->Type, VectorCount, NumElements);
		DrawContext ctx{ pDevice, currentMesh, currentThicc, DrawPath::PrimitiveUP, GameState::IsInSong() };
		for (const auto& e : *active) {
			DrawResult r = e.fn(ctx);
			switch (r.outcome) {
				case DrawOutcome::Hide: return REMOVE_TEXTURE;
				case DrawOutcome::Show: return oDrawPrimitiveUP(pDevice, PrimType, PrimCount, pVertexStreamZeroData, VertexStreamZeroStride);
				case DrawOutcome::ReplaceTexture: pDevice->SetTexture(r.stage, r.texture); break;
				case DrawOutcome::Pass: break;
			}
		}
		return oDrawPrimitiveUP(pDevice, PrimType, PrimCount, pVertexStreamZeroData, VertexStreamZeroStride);
	}

	return oDrawPrimitiveUP(pDevice, PrimType, PrimCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

/// <summary>
/// IDirect3DDevice9::DrawIndexedPrimitiveUP Middleware. See Hook_DrawPrimitiveUP.
/// </summary>
HRESULT APIENTRY D3DHooks::Hook_DrawIndexedPrimitiveUP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT MinVertexIndex, UINT NumVertices, UINT PrimCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
	UltrawideShaders::DrawScope ultrawideScope(pDevice, PrimCount);
	ultrawideScope.Apply();

	if (auto active = Framework::Draw().ActiveSnapshot(DrawPath::IndexedUP); !active->empty()) {
		Mesh currentMesh(VertexStreamZeroStride, PrimCount, NumVertices);
		ThiccMesh currentThicc(VertexStreamZeroStride, PrimCount, NumVertices, 0, StartRegister, PrimType, decl->Type, VectorCount, NumElements);
		DrawContext ctx{ pDevice, currentMesh, currentThicc, DrawPath::IndexedUP, GameState::IsInSong() };
		for (const auto& e : *active) {
			DrawResult r = e.fn(ctx);
			switch (r.outcome) {
				case DrawOutcome::Hide: return REMOVE_TEXTURE;
				case DrawOutcome::Show: return oDrawIndexedPrimitiveUP(pDevice, PrimType, MinVertexIndex, NumVertices, PrimCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
				case DrawOutcome::ReplaceTexture: pDevice->SetTexture(r.stage, r.texture); break;
				case DrawOutcome::Pass: break;
			}
		}
		return oDrawIndexedPrimitiveUP(pDevice, PrimType, MinVertexIndex, NumVertices, PrimCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
	}

	return oDrawIndexedPrimitiveUP(pDevice, PrimType, MinVertexIndex, NumVertices, PrimCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

/// <summary>
/// IDirect3DDevice9::SetVertexDeclaration Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="pdecl"> - Pointer to an IDirect3DVertexDeclaration9 object, which contains the vertex declaration.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. The return value can be D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_SetVertexDeclaration(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexDeclaration9* pdecl) {
	if (pdecl != NULL)
		pdecl->GetDeclaration(decl, &NumElements);

	// Call the original SetVertexDeclaration.
	return oSetVertexDeclaration(pDevice, pdecl);
}

/// <summary>
/// IDirect3DDevice9::SetVertexShaderConstantF Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="i_StartRegister"> - Register number that will contain the first constant value.</param>
/// <param name="pConstantData"> - Pointer to an array of constants.</param>
/// <param name="Vector4fCount"> - Number of four float vectors in the array of constants.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. If the method fails, the return value can be D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 pDevice, UINT i_StartRegister, const float* pConstantData, UINT Vector4fCount) {
	if (pConstantData != NULL) {
		StartRegister = i_StartRegister;
		VectorCount = Vector4fCount;
	}

	// Call the original SetVertexShaderConstantF
	return oSetVertexShaderConstantF(pDevice, i_StartRegister, pConstantData, Vector4fCount);
}

/// <summary>
/// Refresh the ultrawide correction state. Called once per frame from Hook_EndScene, so the
/// per-constant-upload path never has to touch Settings or query the swap chain.
/// </summary>
void D3DHooks::UpdateUltrawideState(IDirect3DDevice9* pDevice) {
	// Only widen. A narrower-than-16:9 display gives a scale above 1.0, which would push the
	// camera frustum and the remapped viewport outside the backbuffer, so the mod stays inert.
	const bool active = ultrawideSettingOn.load(std::memory_order_relaxed)
		&& ultrawideBackBufferValid && AspectRatio::IsWiderThanReference(ultrawideClipXScale);

	ultrawideActive.store(active, std::memory_order_relaxed);

	// Once per frame: the query compares strings, and the draw path reads the
	// result per draw.
	ultrawideInGuitarcade.store(GameState::Menus::IsInGuitarcadeGame(), std::memory_order_relaxed);
	ultrawideInVideoPlayer.store(GameState::currentMenu == "VideoPlayer", std::memory_order_relaxed);
	// Full-stage bitmaps: the title backdrop is widened on the title screens (the "connecting"
	// SelectionListDialog included, the loft is not up yet there) and dropped under the
	// sign-in dialogs.
	UltrawideFullStage::onTitle.store(GameState::currentMenu.empty() || GameState::currentMenu == "pre_enter_prompt"
		|| GameState::currentMenu == "TitleScreen" || GameState::currentMenu == "SelectionListDialog", std::memory_order_relaxed);

	// Publish the backbuffer aspect only while the correction applies. Otherwise the game must
	// keep building its stock 16:9 frustum: the viewport and HUD paths are inert on a display
	// the gate rejected, and a camera that follows the display alone would drift from them.
	const double displayAspect = active && ultrawideBackBufferHeight
		? static_cast<double>(ultrawideBackBufferWidth) / ultrawideBackBufferHeight
		: AspectRatio::referenceAspect;
	UltrawideMod::SetDisplayAspect(displayAspect);

	if (ultrawideBackBufferValid || !pDevice)
		return;

	IDirect3DSwapChain9* swapChain = nullptr;
	if (FAILED(pDevice->GetSwapChain(0, &swapChain)) || !swapChain)
		return;

	D3DPRESENT_PARAMETERS presentParameters{};
	if (SUCCEEDED(swapChain->GetPresentParameters(&presentParameters))
		&& presentParameters.BackBufferWidth && presentParameters.BackBufferHeight) {

		ultrawideClipXScale = AspectRatio::ClipXScale(presentParameters.BackBufferWidth, presentParameters.BackBufferHeight);
		ultrawideBackBufferWidth = presentParameters.BackBufferWidth;
		ultrawideBackBufferHeight = presentParameters.BackBufferHeight;
		ultrawideBackBufferValid = true;

		// Stage verdicts compare a target's size against the backbuffer aspect, so any cached
		// before this point was known was decided on the wrong comparison.
		UltrawideShaders::TextureStages::Invalidate();
	}

	swapChain->Release();
}

/// <summary>
/// IDirect3DDevice9::StretchRect Middleware.
/// </summary>
HRESULT APIENTRY D3DHooks::Hook_StretchRect(LPDIRECT3DDEVICE9 pDevice, IDirect3DSurface9* pSourceSurface, const RECT* pSourceRect, IDirect3DSurface9* pDestSurface, const RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) {
	// The video player composites its frame through the same path at its own aspect; leave it.
	if (ultrawideActive.load(std::memory_order_relaxed) && !ultrawideInVideoPlayer.load(std::memory_order_relaxed)
		&& pDestRect && pDestSurface) {
		D3DSURFACE_DESC destinationDescription{};

		if (SUCCEEDED(pDestSurface->GetDesc(&destinationDescription))) {
			const AspectRatio::Rect destination{ pDestRect->left, pDestRect->top, pDestRect->right, pDestRect->bottom };
			AspectRatio::Rect widened{};

			if (AspectRatio::WidenLetterbox(destination, destinationDescription.Width, destinationDescription.Height, widened)) {
				const RECT widenedRect{ widened.left, widened.top, widened.right, widened.bottom };

				return oStretchRect(pDevice, pSourceSurface, pSourceRect, pDestSurface, &widenedRect, Filter);
			}
		}
	}

	return oStretchRect(pDevice, pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter);
}

// IDirect3DDevice9::SetTexture Middleware. Records the binding for the ultrawide texture-stage
// mirror; classification happens lazily on the draw path.
HRESULT APIENTRY D3DHooks::Hook_SetTexture(LPDIRECT3DDEVICE9 pDevice, DWORD Stage, IDirect3DBaseTexture9* pTexture) {
	// Bind first, mirror after: a rejected bind leaves the device on its previous texture.
	const HRESULT result = oSetTexture(pDevice, Stage, pTexture);
	if (SUCCEEDED(result))
		UltrawideShaders::TextureStages::OnTextureBound(Stage, pTexture);

	return result;
}

/// <summary>
/// IDirect3DDevice9::SetRenderTarget Middleware. Tracks whether the scene target is bound, so the
/// aspect correction can confine itself to draws that actually reach the screen.
/// </summary>
HRESULT APIENTRY D3DHooks::Hook_SetRenderTarget(LPDIRECT3DDEVICE9 pDevice, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) {
	// Bind first, classify after: a rejected target keeps the previous one bound, and the draw
	// path must keep reading that one's classification.
	const HRESULT result = oSetRenderTarget(pDevice, RenderTargetIndex, pRenderTarget);

	if (RenderTargetIndex == 0 && SUCCEEDED(result)) {

		// One GetDesc serves both the render-target registry below and the scene test after it.
		D3DSURFACE_DESC targetDescription{};
		const bool haveDescription = pRenderTarget && SUCCEEDED(pRenderTarget->GetDesc(&targetDescription));

		if (haveDescription) {
			IDirect3DTexture9* container = nullptr;
			if (SUCCEEDED(pRenderTarget->GetContainer(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&container))) && container) {
				const auto existing = ultrawideRenderTargetTextures.find(container);
				const std::pair<UINT, UINT> size{ targetDescription.Width, targetDescription.Height };

				// A texture only just discovered to be a render target may already be sitting in a
				// texture stage classified as an ordinary texture, so retire the cached verdicts.
				if (existing == ultrawideRenderTargetTextures.end() || existing->second != size) {
					ultrawideRenderTargetTextures[container] = size;
					UltrawideShaders::TextureStages::Invalidate();
				}

				container->Release();
			}
		}

		bool isScene = false;
		UINT width = 0;
		UINT height = 0;

		if (haveDescription && ultrawideBackBufferValid) {
			width = targetDescription.Width;
			height = targetDescription.Height;

			isScene = AspectRatio::SameAspect(width, height, ultrawideBackBufferWidth, ultrawideBackBufferHeight);
		}

		ultrawideRenderTargetIsScene = isScene;
		ultrawideRenderTargetWidth = width;
		ultrawideRenderTargetHeight = height;
	}

	return result;
}

/// <summary>
/// IDirect3DDevice9::SetVertexShader Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="veShader"> - Vertex shader interface.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. If the method fails, the return value can be D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_SetVertexShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexShader9* veShader) {
	if (veShader != NULL)
		vShader = veShader;

	// Bind first, publish after: a rejected bind leaves the previous shader current.
	const HRESULT result = oSetVertexShader(pDevice, veShader);
	if (SUCCEEDED(result))
		UltrawideShaders::OnVertexShaderBound(veShader);

	return result;
}

/// <summary>
/// IDirect3DDevice9::SetPixelShader Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="piShader"> - Pixel shader interface.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. If the method fails, the return value can be D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_SetPixelShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DPixelShader9* piShader) {
	if (piShader != NULL)
		pShader = piShader;

	// Call the original SetPixelShader.
	return oSetPixelShader(pDevice, piShader);
}

/// <summary>
/// IDirect3DDevice9::SetStreamSource Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="StreamNumber"> - Specifies the data stream, in the range from 0 to the maximum number of streams -1.</param>
/// <param name="pStreamData"> - Pointer to an IDirect3DVertexBuffer9 interface, representing the vertex buffer to bind to the specified data stream.</param>
/// <param name="OffsetInBytes"> - Offset from the beginning of the stream to the beginning of the vertex data, in bytes.</param>
/// <param name="i_Stride"> - Stride of the component, in bytes.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. If the method fails, the return value can be D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_SetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT i_Stride) {
	// Call original SetStreamSource.
	return oSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, i_Stride);
}

/// <summary>
/// IDirect3DDevice9::Reset Middleware. Required so Alt+Tab won't break the game (ImGUI & UI Text).
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="pPresentationParameters"> - Pointer to a D3DPRESENT_PARAMETERS structure, describing the new presentation parameters. This value cannot be NULL.</param>
/// <returns>Possible return values include: D3D_OK, D3DERR_DEVICELOST, D3DERR_DEVICEREMOVED, D3DERR_DRIVERINTERNALERROR, or D3DERR_OUTOFVIDEOMEMORY.</returns>
HRESULT APIENTRY D3DHooks::Hook_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
	// Release device-dependent resources before Reset. The ImGui backend owns the HUD font texture now,
	// so invalidating it is all the overlay needs.
	ImGui_ImplDX9_InvalidateDeviceObjects();

	// Reset Device. Call original Reset.
	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	// Only recreate device objects once the device is actually back. If Reset failed
	// (e.g. still D3DERR_DEVICELOST mid-Alt+Tab out of exclusive fullscreen), the game
	// retries Reset next frame; recreating against a lost device leaves a broken frame.
	if (SUCCEEDED(ResetReturn)) {
		ImGui_ImplDX9_CreateDeviceObjects();
		Framework::Draw().RunDeviceReset(pDevice);
	}

	return ResetReturn;
}

/// <summary>
/// IDirect3DDevice9::DrawIndexedPrimitive Middleware. This is where most of our texture modifying mods are located.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="PrimType"> - Member of the D3DPRIMITIVETYPE enumerated type, describing the type of primitive to render.</param>
/// <param name="BaseVertexIndex"> - Offset from the start of the vertex buffer to the first vertex.</param>
/// <param name="MinVertexIndex"> - Minimum vertex index for vertices used during this call. This is a zero based index relative to BaseVertexIndex.</param>
/// <param name="NumVertices"> - Number of vertices used during this call.</param>
/// <param name="StartIndex"> - Index of the first index to use when accesssing the vertex buffer.</param>
/// <param name="PrimCount"> - Number of primitives to render.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. If the method fails, the return value can be the following: D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_DIP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimCount) { // Draw things on screen
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	UltrawideShaders::DrawScope ultrawideScope(pDevice, PrimCount);
	ultrawideScope.Apply();

	// Full-stage bitmaps: the dim plate and the title backdrop are widened over the whole
	// backbuffer, the backdrops dropped under the sign-in dialogs. Confined draws only, so
	// inert at 16:9.
	{
		const auto decision = UltrawideFullStage::Decide(pDevice, PrimCount, ultrawideScope.Confined());
		switch (decision) {
		case UltrawideFullStage::Decision::Skip:
			return D3D_OK;
		case UltrawideFullStage::Decision::Stretch:
		case UltrawideFullStage::Decision::StretchWide: {
			UltrawideFullStage::StretchViewport(pDevice, ultrawideBackBufferWidth,
				decision == UltrawideFullStage::Decision::Stretch);
			const HRESULT stretched = oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);
			UltrawideFullStage::Restore(pDevice);
			return stretched;
		}
		default:
			break;
		}
	}

	// This could potentially lead to game locking up (because DIP is called multiple times per frame) if that value is not filled, but generally it should work 
	if (Settings::ReturnSettingValue(Settings::Setting::ExtendedRangeEnabled).length() < 2) { // Due to some weird reasons, sometimes settings decide to go missing - this may solve the problem
		static std::atomic_bool reloadQueued = false;
		if (!reloadQueued.exchange(true)) {
			Framework::Registry().EnqueueSettingsUpdate([] {
				Settings::UpdateSettings();
				reloadQueued.store(false);
				LOG_INFO("Reloaded settings" << std::endl);
			});
		}
	}

	Mesh current(Stride, PrimCount, NumVertices);
	ThiccMesh currentThicc(Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements);

	// Debugging of DIP. Compiled out of Release: `debug` is false there anyway, and the mesh
	// logger is an investigation tool the draw path should not carry.
	#ifdef _DEBUG
	if (debug) {
		if (GetAsyncKeyState(VK_PRIOR) & 1 && currIdx < std::size(allMeshes) - 1)// Page up
			currIdx++;
		if (GetAsyncKeyState(VK_NEXT) & 1 && currIdx > 0) // Page down
			currIdx--;

		if (GetAsyncKeyState(VK_END) & 1) { // Toggle logging
			LOG_INFO("Logging is ");
			startLogging = !startLogging;
			if (!startLogging)
				LOG_NOHEAD("no longer ");
			LOG_NOHEAD("armed!" << std::endl);
		}

		if (GetAsyncKeyState(VK_F8) & 1) { // Save logged meshes to file
			for (const auto& mesh : allMeshes) {
				(void)mesh;
				//Log(mesh.ToString().c_str());
			}
		}

		if (GetAsyncKeyState(VK_F7) & 1) { // Save only removed 
			for (const auto& mesh : removedMeshes) {
				(void)mesh;
				//Log(mesh.ToString().c_str());
			}
		}

		if (GetAsyncKeyState(VK_CONTROL) & 1)
			//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements);

			if (startLogging) {
				if (std::ranges::find(allMeshes, currentThicc) == allMeshes.end()) // Make sure we don't log what we'd already logged
					allMeshes.push_back(currentThicc);
				if (NOTE_STEMS) // Criteria for search
					LOG_INFO("{ " << Stride << ", "
									  << PrimCount << ", "
									  << NumVertices << ", "
									  << StartIndex << ", "
									  << StartRegister << ", "
									  << (UINT)PrimType << ", "
									  << (UINT)decl->Type << ", "
									  << VectorCount << ", "
									  << NumElements << " },"
									  << std::endl); // Thicc Mesh -> Log
				
				//_LOG("{ "<< Stride << ", " << PrimCount << ", " << NumVertices << " }," std::endl; // Mesh -> Console
				//_LOG(std::hex << crc << std::endl);
			}

		if (std::size(allMeshes) > 0 && allMeshes.at(currIdx) == currentThicc) {
			currStride = Stride;
			currNumVertices = NumVertices;
			currPrimCount = PrimCount;
			currStartIndex = StartIndex;
			currStartRegister = StartRegister;
			currPrimType = PrimType;
			currDeclType = decl->Type;
			currVectorCount = VectorCount;
			currNumElements = NumElements;
			//pDevice->SetTexture(1, Yellow);
			return REMOVE_TEXTURE;
		}

		if (IsExtraRemoved(removedMeshes, currentThicc))
			return REMOVE_TEXTURE;
	}
	#endif

	// Interceptor Registry Walk
	DrawContext ctx{ pDevice, current, currentThicc, DrawPath::Indexed, GameState::IsInSong() };
	auto active = Framework::Draw().ActiveSnapshot(DrawPath::Indexed);
	for (const auto& e : *active) {
		DrawResult r = e.fn(ctx);
		switch (r.outcome) {
			case DrawOutcome::Hide: return REMOVE_TEXTURE;
			case DrawOutcome::Show: return SHOW_TEXTURE;
			case DrawOutcome::ReplaceTexture: pDevice->SetTexture(r.stage, r.texture); break;
			case DrawOutcome::Pass: break;
		}
	}

	return SHOW_TEXTURE; // KEEP THIS LINE. This translates to "Display Graphics".
}

void D3DHooks::CheckRecreateTextures(IDirect3DDevice9* pDevice) {
	if (!pDevice) return;

	if (RecreateTextures.exchange(false)) {
		Framework::Draw().RegenerateAllTextures(pDevice);
	}
}
