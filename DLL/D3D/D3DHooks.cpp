#include "D3DHooks.hpp"

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

	// Note-tails for Extended Range / Custom Colors
	if (AttemptedERInThisSong && UseEROrColorsInThisSong && NOTE_TAILS) {
		MemHelpers::ToggleCB(UseERExclusivelyInThisSong);

		switch (Settings::GetModSetting("SeparateNoteColors")) {
			case 0: // Use same color scheme on notes as we do on strings
				pDevice->SetTexture(1, customStringColorTexture);
				break;
			case 1: // Default Colors, so don't do anything.
				break;
			case 2: // Use Custom Note Color Scheme
				pDevice->SetTexture(1, customNoteColorTexture);
				break;
			default:
				break;
		}
	}

	// Note-tails for Twitch mod - Remove Notes.
	if (Settings::IsTwitchSettingEnabled("RemoveNotes") && NOTE_TAILS)
		return REMOVE_TEXTURE;

	// Note-tails for Twitch mod - Transparent Notes.
	if (Settings::IsTwitchSettingEnabled("TransparentNotes") && NOTE_TAILS)
		pDevice->SetTexture(1, nonexistentTexture);

	// Note-tails for Twitch mod - Solid Colored notes.
	if (Settings::IsTwitchSettingEnabled("SolidNotes") && NOTE_TAILS) {
		if (Settings::ReturnSettingValue("SolidNoteColor") == "random")
			pDevice->SetTexture(1, randomTextures[currentRandomTexture]);
		else
			pDevice->SetTexture(1, twitchUserDefinedTexture);
	}
	
	// Note-tails for Rainbow Notes.
	if (ERMode::RainbowNotesEnabled && ERMode::customNoteColorH > 0 && NOTE_TAILS)
		pDevice->SetTexture(1, rainbowTextures[ERMode::customNoteColorH]);

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
	D3DVERTEXBUFFER_DESC desc;

	// Remove Line Markers mod.
	if (i_Stride == 32 && NumElements == 8 && VectorCount == 4 && decl->Type == 2) { 
		pStreamData->GetDesc(&desc);
		vertexBufferSize = desc.Size;
	}

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
	// Lost Device
	ImGui_ImplDX9_InvalidateDeviceObjects();

	if (MemHelpers::DX9FontEncapsulation)
		MemHelpers::DX9FontEncapsulation->OnLostDevice();

	// Reset Device. Call original Reset.
	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects();

	if (MemHelpers::DX9FontEncapsulation)
		MemHelpers::DX9FontEncapsulation->OnResetDevice();

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
	_LOG_INIT;

	static bool calculatedCRC = false, calculatedHeadstocks = false, calculatedSkyline = false;

	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	// This could potentially lead to game locking up (because DIP is called multiple times per frame) if that value is not filled, but generally it should work 
	if (Settings::ReturnSettingValue("ExtendedRangeEnabled").length() < 2) { // Due to some weird reasons, sometimes settings decide to go missing - this may solve the problem
		Settings::UpdateSettings();
		D3D::GenerateTextures(pDevice, D3D::Strings);
		D3D::GenerateTextures(pDevice, D3D::Notes);
		D3D::GenerateTextures(pDevice, D3D::Rainbow);
		_LOG("Reloaded settings" << std::endl);
	}

	if (setAllToNoteGradientTexture) {
		pDevice->SetTexture(currStride, gradientTextureSeven);
		return SHOW_TEXTURE;
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
			_LOG("Logging is ");
			startLogging = !startLogging;
			if (!startLogging)
				_LOG_NOHEAD("no longer ");
			_LOG_NOHEAD("armed!" << std::endl);
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
				if (std::find(allMeshes.begin(), allMeshes.end(), currentThicc) == allMeshes.end()) // Make sure we don't log what we'd already logged
					allMeshes.push_back(currentThicc);
				if (NOTE_STEMS) // Criteria for search
					_LOG("{ " << Stride << ", "
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

	// Mods

	// Change Noteway Color | This NEEDS to be above Extended Range / Custom Colors or it won't work.
	if (IsToBeRemoved(noteHighway, current) && Settings::ReturnSettingValue("CustomHighwayColors") == (std::string)"on") {
		pDevice->GetTexture(1, &pBaseNotewayTexture);
		pCurrNotewayTexture = (IDirect3DTexture9*)pBaseNotewayTexture;

		if (pBaseNotewayTexture) {
			if (CRCForTexture(pCurrNotewayTexture, pDevice, crc)) {

				// Noteway Texture
				if (crc == crcNoteLanes && Settings::ReturnNotewayColor("CustomHighwayNumbered") != (std::string)"" && Settings::ReturnNotewayColor("CustomHighwayUnNumbered") != (std::string)"")
					pDevice->SetTexture(1, notewayTexture);

				// Fret Number texture
				else if (crc == crcNotewayFretNumbers && Settings::ReturnNotewayColor("CustomFretNubmers") != (std::string)"")
					pDevice->SetTexture(1, fretNumTexture);

				// Gutter texture
				else if (crc == crcNotewayGutters && Settings::ReturnNotewayColor("CustomHighwayGutter") != (std::string)"")
					pDevice->SetTexture(1, gutterTexture);
			}
		}
	}


	// // Custom Loft Gameplay Wall / Narnia / Portal / Venue wall
	//if (IsExtraRemoved(greenScreenWallMesh, currentThicc)) {
	//		//// Save Loft Texture To File
	//	//DumpTextureStages(pDevice, "greenscreenwall");

	//	// Use Custom Texture (File names can be found in venues/loft01.psarc/assets/generic/env/the_loft/
	//	// Files sent in currently require the name "stage#.png" where # is the number attached to the texture variable. Ex: customGreenScreenWall_Stage3 would need a file named "stage3.png"

	//	// Background tile displays as follows:
	//	// Top 512 of Background Tile are shown at the bottom of the screen, repeated 1-1/2 times.
	//	// Bottom 512 of Background Tile are shown at the top of the screen, repeated 2-1/2 times.

	//	// Example: What is shown - https://cdn.discordapp.com/attachments/711634485388771439/813523398587711488/unknown.png vs What is sent - https://cdn.discordapp.com/attachments/711634485388771439/813523420947152916/stage0.png
	//	// Example: Full wall (which you will never see for more than a second or so) - https://cdn.discordapp.com/attachments/711634485388771439/813524110390460436/unknown.png

	//	pDevice->SetTexture(0, customGreenScreenWall_Stage0);   // Background Tile | loft_concrete_wall_b.dds | 1024x1024 | Can be modified. Used for the background.
	//	//pDevice->SetTexture(1, customGreenScreenWall_Stage1); // Noise | noise03.dds | 256x256 | Doesn't have any effect
	//	//pDevice->SetTexture(2, customGreenScreenWall_Stage2); // Caustic (Indirect) | caustic_indirect01.dds | 256x256 | Doesn't have any effect
	//	//pDevice->SetTexture(3, customGreenScreenWall_Stage3); // Narnia / Venue Fade In Mask | fade_shape.dds | 512x512 | Can be modified. If you use a single colored square, you can make an almost "movie like" flashback.
	//	//pDevice->SetTexture(4, customGreenScreenWall_Stage4); // White square (Unknown) | 1024x1024 | Doesn't have any effect
	//	//pDevice->SetTexture(5, customGreenScreenWall_Stage5); // Pipes and wall trim | portal_wall_ao.dds | 1024x1024 | Can be modified.
	//	//pDevice->SetTexture(6, customGreenScreenWall_Stage6); // N Mask of Background tile | loft_concrete_wall_b_n.dds | 1024x1024 | Don't modify
	//}

	// Rainbow Notes | This part NEEDS to be above Extended Range / Custom Colors or it won't work.
	if (ERMode::RainbowNotesEnabled && ERMode::customNoteColorH > 0) { 

		if (ERMode::customNoteColorH > 179)
			ERMode::customNoteColorH -= 180;

		RainbowNotes = true;

		// Colors for note stems (part below the note), bends, slides, and accents
		if (NOTE_STEMS || OPEN_NOTE_ACCENTS) {
			pDevice->GetTexture(1, &pBaseRainbowTexture);
			pCurrRainbowTexture = (IDirect3DTexture9*)pBaseRainbowTexture;

			if (!pBaseRainbowTexture)
				return SHOW_TEXTURE;

			if (CRCForTexture(pCurrRainbowTexture, pDevice, crc)) {

				// Same checksum for stems and accents, because they use the same texture. Bends and slides use the same texture.
				if (crc == crcStemsAccents || crc == crcBendSlideIndicators)
					pDevice->SetTexture(1, rainbowTextures[ERMode::customNoteColorH]);
			}
		}

		// As of right now, this requires rainbow strings to be toggled on
		if (PrideMode && NOTE_TAILS) 
			pDevice->SetTexture(1, rainbowTextures[ERMode::customNoteColorH]);
	}

	// User has updated their settings, and we need to recreate our textures
	if (RecreateTextures && RecreateTextureTimer) {
		// Generate textures to be called later
		D3D::GenerateTextures(pDevice, D3D::Random_Solid);
		D3D::GenerateTextures(pDevice, D3D::Strings);
		D3D::GenerateTextures(pDevice, D3D::Notes);
		D3D::GenerateTextures(pDevice, D3D::Noteway);
		D3D::GenerateTextures(pDevice, D3D::Gutter);
		D3D::GenerateTextures(pDevice, D3D::FretNums);
		D3D::GenerateTextures(pDevice, D3D::Rainbow);

		RecreateTextures = false;
		RecreateTextureTimer = false;
	}
		
	//if (Settings::ReturnSettingValue("DiscoModeEnabled") == "on") {
	//	 //Need Lovro's Help With This :(
	//	if (DiscoModeInitialSetting.find(pDevice) == DiscoModeInitialSetting.end()) { // We haven't saved this pDevice's initial values yet
	//		DWORD initialAlphaValue = (DWORD)pDevice, initialSeperateValue = (DWORD)pDevice;
	//		pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD*)initialAlphaValue);
	//		pDevice->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD*)initialSeperateValue);
	//		
	//		DiscoModeInitialSetting.insert({ pDevice, std::make_pair(initialAlphaValue, initialSeperateValue) });
	//	}
	//	else { // We've seen this pDevice value before.
	//		if (DiscoModeEnabled) { // Key was pressed to have Disco Mode on
	//			pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); // Make AMPS Semi-Transparent <- Is the one that makes things glitchy.
	//			pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE); // Sticky Colors
	//		}

	//		else { // Disco mode was turned off, we need to revert the settings so there is no trace of disco mode.
	//			for (auto pDeviceList : DiscoModeInitialSetting) {
	//				pDeviceList.first->SetRenderState(D3DRS_ALPHABLENDENABLE, *(DWORD*)pDeviceList.second.first); // Needs to have *(DWORD*) since it only sets DWORD not DWORD*
	//				pDeviceList.first->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, *(DWORD*)pDeviceList.second.second); // Needs to have *(DWORD*) since it only sets DWORD not DWORD*
	//			}
	//		}
	//	}
	//}

	/*if (IsExtraRemoved(lyrics, currentThicc)) { // Move Lyrics to different file. Current State: CRC never updates, BUT does pass CRCForTexture().
		pDevice->GetTexture(0, &pBaseTexture);

		D3DXSaveTextureToFile(L"lyrics_temp.png", D3DXIFF_PNG, pBaseTexture, NULL);

		D3DXCreateTextureFromFile(pDevice, L"lyrics_temp.png", &pCurrTexture);

		if (CRCForTexture(pCurrTexture, pDevice, crc)) {
			if (crc != crcLyrics) {
				D3DXSaveTextureToFile(L"lyrics.png", D3DXIFF_PNG, pBaseTexture, NULL);
				crcLyrics = crc;
				_LOG("new lyric posted to lyrics.png" << std::endl);
			}
			_LOG(std::hex << crcLyrics << " = " << std::hex << crc << std::endl);
		}
	}*/

	// Extended Range / Custom Colors (includes separate note colors)
	if (AttemptedERInThisSong && UseEROrColorsInThisSong) {
		MemHelpers::ToggleCB(UseERExclusivelyInThisSong);

		// Settings::GetModSetting("SeparateNoteColors") == 1 -> Default Colors, so don't do anything.

		// Use same color scheme on notes as we do on strings (0) || Use Custom Note Color Scheme (2)
		if (Settings::GetModSetting("SeparateNoteColorsMode") == 0 || (Settings::ReturnSettingValue("SeparateNoteColors") == "on" && Settings::GetModSetting("SeparateNoteColorsMode") == 2)) { 

			// Color notes like string colors
			LPDIRECT3DTEXTURE9 textureToUseOnNotes = customStringColorTexture; 

			// Custom colored notes
			if (Settings::GetModSetting("SeparateNoteColorsMode") == 2)
				textureToUseOnNotes = customNoteColorTexture; 

			// Change all pieces of note head's textures
			if (IsToBeRemoved(sevenstring, current) || IsExtraRemoved(noteModifiers, currentThicc))  
				pDevice->SetTexture(1, textureToUseOnNotes);

			// Colors for note stems (part below the note), bends, slides, and accents
			else if (NOTE_STEMS || OPEN_NOTE_ACCENTS) { 
				pDevice->GetTexture(1, &pBaseTexture);
				pCurrTexture = (IDirect3DTexture9*)pBaseTexture;

				if (!pBaseTexture)
					return SHOW_TEXTURE;

				if (CRCForTexture(pCurrTexture, pDevice, crc)) {

					// Same checksum for stems and accents, because they use the same texture. Bends and slides use the same texture.
					if (crc == crcStemsAccents || crc == crcBendSlideIndicators)  
						pDevice->SetTexture(1, textureToUseOnNotes);
				}

				return SHOW_TEXTURE;
			}
		}
	}

	// Twitch wants notes to be removed.
	if (Settings::IsTwitchSettingEnabled("RemoveNotes"))
		// Note textures, outside of note stems and open note accents.
		if (IsToBeRemoved(sevenstring, current) || IsExtraRemoved(noteModifiers, currentThicc))
			return REMOVE_TEXTURE;

		// Colors for note stems (part below the note), bends, slides, and accents
		else if (NOTE_STEMS || OPEN_NOTE_ACCENTS) { 
			pDevice->GetTexture(1, &pBaseTexture);
			pCurrTexture = (IDirect3DTexture9*)pBaseTexture;

			if (!pBaseTexture)
				return REMOVE_TEXTURE;

			if (CRCForTexture(pCurrTexture, pDevice, crc)) {

				// Same checksum for stems and accents, because they use the same texture. Bends and slides use the same texture.
				if (crc == crcStemsAccents || crc == crcBendSlideIndicators)  
					return REMOVE_TEXTURE;
			}

			return REMOVE_TEXTURE;
		}

	// Twitch wants transparent notes.
	if (Settings::IsTwitchSettingEnabled("TransparentNotes"))
		// Note textures, outside of note stems and open note accents.
		if (IsToBeRemoved(sevenstring, current) || IsExtraRemoved(noteModifiers, currentThicc) || NOTE_STEMS || OPEN_NOTE_ACCENTS)
			pDevice->SetTexture(1, nonexistentTexture);

		// Colors for note stems (part below the note), bends, slides, and accents
		else if (NOTE_STEMS || OPEN_NOTE_ACCENTS) { 
			pDevice->GetTexture(1, &pBaseTexture);
			pCurrTexture = (IDirect3DTexture9*)pBaseTexture;

			if (!pBaseTexture)
				return SHOW_TEXTURE;

			if (CRCForTexture(pCurrTexture, pDevice, crc)) {

				// Same checksum for stems and accents, because they use the same texture. Bends and slides use the same texture.
				if (crc == crcStemsAccents || crc == crcBendSlideIndicators)  
					pDevice->SetTexture(1, nonexistentTexture);
			}

			return SHOW_TEXTURE;
		}

	// Twitch wants solid note colors
	if (Settings::IsTwitchSettingEnabled("SolidNotes")) {
		// Note textures, outside of note stems and open note accents.
		if (IsToBeRemoved(sevenstring, current) || IsExtraRemoved(noteModifiers, currentThicc)) {

			// Random Colors
			if (Settings::ReturnSettingValue("SolidNoteColor") == "random") 
				pDevice->SetTexture(1, randomTextures[currentRandomTexture]);
			// They set the color they want in the GUI | TODO: Colors are changed on chord boxes
			else 
				pDevice->SetTexture(1, twitchUserDefinedTexture);
		}

		// Colors for note stems (part below the note), bends, slides, and accents
		else if (NOTE_STEMS || OPEN_NOTE_ACCENTS) { 
			pDevice->GetTexture(1, &pBaseTexture);
			pCurrTexture = (IDirect3DTexture9*)pBaseTexture;

			if (!pBaseTexture)
				return SHOW_TEXTURE;

			if (CRCForTexture(pCurrTexture, pDevice, crc)) {

				// Same checksum for stems and accents, because they use the same texture. Bends and slides use the same texture.
				if (crc == crcStemsAccents || crc == crcBendSlideIndicators) {  

					// Random Colors
					if (Settings::ReturnSettingValue("SolidNoteColor") == "random") 
						pDevice->SetTexture(1, randomTextures[currentRandomTexture]);
					else
						pDevice->SetTexture(1, twitchUserDefinedTexture);
				}
			}

			return SHOW_TEXTURE;
		}
	}

	// Twitch wants us to reset your note streak.
	if (Settings::IsTwitchSettingEnabled("FYourFC")) {
		uintptr_t currentNoteStreak = 0;

		if (MemHelpers::Contains(currentMenu, learnASongModes))
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakLASOffsets);
		else if (MemHelpers::Contains(currentMenu, scoreAttackModes))
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakSAOffsets);

		if (currentNoteStreak != 0)
			*(BYTE*)currentNoteStreak = 0;
	}

	// Twitch wants to see the user play in Drunk Mode.
	if (Settings::IsTwitchSettingEnabled("DrunkMode")) {
		std::uniform_real_distribution<> keepValueWithin(-1.5, 1.5);
		*(float*)Offsets::ptr_drunkShit = (float)keepValueWithin(rng);
	}

	// Greenscreen Wall
	if ((Settings::ReturnSettingValue("GreenScreenWallEnabled") == "on" || GreenScreenWall) && IsExtraRemoved(greenScreenWallMesh, currentThicc))
		return REMOVE_TEXTURE;

	// Thicc Mesh Mods that are as simple as doing a simple check against the params of this function.
	if (MemHelpers::IsInSong()) {
		if (Settings::ReturnSettingValue("FretlessModeEnabled") == "on" && IsExtraRemoved(fretless, currentThicc))
			return REMOVE_TEXTURE;
		if (Settings::ReturnSettingValue("RemoveInlaysEnabled") == "on" && IsExtraRemoved(inlays, currentThicc))
			return REMOVE_TEXTURE;
		if (Settings::ReturnSettingValue("RemoveLaneMarkersEnabled") == "on" && IsExtraRemoved(laneMarkers, currentThicc))
			return REMOVE_TEXTURE;
		if (RemoveLyrics && Settings::ReturnSettingValue("RemoveLyrics") == "on" && IsExtraRemoved(lyrics, currentThicc))
			return REMOVE_TEXTURE;
	}

	// Remove Headstock Artifacts
	else if (MemHelpers::Contains(currentMenu, tuningMenus) && Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && RemoveHeadstockInThisMenu)
	{
		// This is called to remove those pesky tuning letters that share the same texture values as fret numbers and chord fingerings
		if (IsExtraRemoved(tuningLetters, currentThicc)) 
			return REMOVE_TEXTURE;

		// This is called to remove the tuner's highlights
		if (IsExtraRemoved(tunerHighlight, currentThicc))
			return REMOVE_TEXTURE;

		// Lefties need their own little place in life...
		if (IsExtraRemoved(leftyFix, currentThicc)) 
			return REMOVE_TEXTURE;
	}

	// Skyline Removal
	if (toggleSkyline && POSSIBLE_SKYLINE) {

		// If the user is in "Song" mode for Toggle Skyline and is NOT in a song -> draw the UI.
		// This means we show the skyline in the learn a song - Song Details page.
		if (DrawSkylineInMenu) { 
			SkylineOff = false;
			return SHOW_TEXTURE;
		}

		pDevice->GetTexture(1, &pBaseTextures[1]);
		pCurrTextures[1] = (IDirect3DTexture9*)pBaseTextures[1];

		// There's only two textures in Stage 1 for meshes with Stride = 16, so we could as well skip CRC calcuation and just check if !pBaseTextures[1] and return REMOVE_TEXTURE directly
		if (pBaseTextures[1]) {  
			if (CRCForTexture(pCurrTextures[1], pDevice, crc)) {

				// Purple rectangles + orange line beneath them
				if (crc == crcSkylinePurple || crc == crcSkylineOrange) { 
					SkylineOff = true;
					return REMOVE_TEXTURE;
				}
			}
		}

		pDevice->GetTexture(0, &pBaseTextures[0]);
		pCurrTextures[0] = (IDirect3DTexture9*)pBaseTextures[0];

		if (pBaseTextures[0]) {
			if (CRCForTexture(pCurrTextures[0], pDevice, crc)) {

				// There's a few more of textures used in Stage 0, so doing the same is no-go; Shadow-ish thing in the background + backgrounds of rectangles.
				if (crc == crcSkylineBackground || crc == crcSkylineShadow) {  
					SkylineOff = true;
					return REMOVE_TEXTURE;
				}
			}
		}
	}

	// Headstock Removal
	else if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on") {
		if (POSSIBLE_HEADSTOCKS) { // If we call GetTexture without any filtering, it causes a lockup when ALT-TAB-ing/changing fullscreen to windowed and vice versa
			if (!RemoveHeadstockInThisMenu) // This user has RemoveHeadstock only on during the song. So if we aren't in the song, we need to draw the headstock texture.
				return SHOW_TEXTURE;

			pDevice->GetTexture(1, &pBaseTextures[1]);
			pCurrTextures[1] = (IDirect3DTexture9*)pBaseTextures[1];

			// Need to reset cache, and this is a headstock texture.
			if (resetHeadstockCache && IsExtraRemoved(headstockThicc, currentThicc)) {
				if (!pBaseTextures[1]) //if there's no texture for Stage 1
					return REMOVE_TEXTURE;

				// Take a CRC of the texture, and check it against our preset CRCs.
				if (CRCForTexture(pCurrTextures[1], pDevice, crc)) {
					if (crc == crcHeadstock0 || crc == crcHeadstock1 || crc == crcHeadstock2 || crc == crcHeadstock3 || crc == crcHeadstock4)
						AddToTextureList(headstockTexturePointers, pCurrTextures[1]);
				}

				int headstockCRCLimit = 3;

				// If the user is in multiplayer, we have to make sure our CRC limit is double or some bugs appear.
				if (MemHelpers::Contains(currentMenu, multiplayerTuners))
					headstockCRCLimit = 6;

				// We've calculated all CRCs that we can, within our limit.
				if (headstockTexturePointers.size() == headstockCRCLimit) {
					calculatedHeadstocks = true;
					resetHeadstockCache = false;
				}

				return REMOVE_TEXTURE;
			}

			// We've already cached the headstocks we're using, so find the one we are working with and remove it.
			if (calculatedHeadstocks)
				if (std::find(std::begin(headstockTexturePointers), std::end(headstockTexturePointers), pCurrTextures[1]) != std::end(headstockTexturePointers))
					return REMOVE_TEXTURE;
		}
	}

	// Rainbow Notes || This part NEEDS to be below Extended Range / Custom Colors or it won't work.
	if (RainbowNotes) { 

		// Rainbow Note Heads
		if (IsToBeRemoved(sevenstring, current) || IsExtraRemoved(noteModifiers, currentThicc)) 
			pDevice->SetTexture(1, rainbowTextures[ERMode::customNoteColorH]);

		RainbowNotes = false;
	}

	
	return SHOW_TEXTURE; // KEEP THIS LINE. This translates to "Display Graphics".
}