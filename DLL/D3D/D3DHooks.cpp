#include "../stdafx.h"
#include "D3DHooks.hpp"
#include "../Framework/Framework.hpp"
#include "../D3DOverlay.hpp"
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
/// IDirect3DDevice9::SetVertexShader Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="veShader"> - Vertex shader interface.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. If the method fails, the return value can be D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_SetVertexShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexShader9* veShader) {
	if (veShader != NULL) {
		vShader = veShader;
		vShader->GetFunction(NULL, &vSize);
	}
	
	// Call the original SetVertexShader.
	return oSetVertexShader(pDevice, veShader);
}

/// <summary>
/// IDirect3DDevice9::SetPixelShader Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="piShader"> - Pixel shader interface.</param>
/// <returns>If the method succeeds, the return value is D3D_OK. If the method fails, the return value can be D3DERR_INVALIDCALL.</returns>
HRESULT APIENTRY D3DHooks::Hook_SetPixelShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DPixelShader9* piShader) {
	if (piShader != NULL) {
		pShader = piShader;
		pShader->GetFunction(NULL, &pSize);
	}

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
	// Lost Device - release device-dependent resources before Reset.
	ImGui_ImplDX9_InvalidateDeviceObjects();
	GameOverlay::OnLostDevice();

	// Reset Device. Call original Reset.
	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	// Only recreate device objects once the device is actually back. If Reset failed
	// (e.g. still D3DERR_DEVICELOST mid-Alt+Tab out of exclusive fullscreen), the game
	// retries Reset next frame; recreating against a lost device leaves a broken frame.
	if (SUCCEEDED(ResetReturn)) {
		ImGui_ImplDX9_CreateDeviceObjects();
		GameOverlay::OnResetDevice();
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

	// Debugging of DIP.
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
				//Log(mesh.ToString().c_str());
			}
		}

		if (GetAsyncKeyState(VK_F7) & 1) { // Save only removed 
			for (const auto& mesh : removedMeshes) {
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
