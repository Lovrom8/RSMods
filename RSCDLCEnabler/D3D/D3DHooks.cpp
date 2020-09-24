#include "D3DHooks.hpp"


HRESULT APIENTRY D3DHooks::Hook_DP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT StartIndex, UINT PrimCount) {
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" && Stride == 12) { //Stride 12 = tails
		MemHelpers::ToggleCB(MemHelpers::IsExtendedRangeSong());
		pDevice->SetTexture(1, ourTexture); //For random textures, use randomTextures[currentRandTexture]
	}

	return oDrawPrimitive(pDevice, PrimType, StartIndex, PrimCount);
}

HRESULT APIENTRY D3DHooks::Hook_SetVertexDeclaration(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexDeclaration9* pdecl) {
	if (pdecl != NULL)
		pdecl->GetDeclaration(decl, &NumElements);

	return oSetVertexDeclaration(pDevice, pdecl);
}

HRESULT APIENTRY D3DHooks::Hook_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, const float* pConstantData, UINT Vector4fCount) {
	if (pConstantData != NULL) {
		StartRegister = StartRegister;
		VectorCount = Vector4fCount;
	}

	return oSetVertexShaderConstantF(pDevice, StartRegister, pConstantData, Vector4fCount);
}

HRESULT APIENTRY D3DHooks::Hook_SetVertexShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexShader9* veShader) {
	if (veShader != NULL) {
		vShader = veShader;
		vShader->GetFunction(NULL, &vSize);
	}

	return oSetVertexShader(pDevice, veShader);
}

HRESULT APIENTRY D3DHooks::Hook_SetPixelShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DPixelShader9* piShader) {
	if (piShader != NULL) {
		pShader = piShader;
		pShader->GetFunction(NULL, &pSize);
	}

	return oSetPixelShader(pDevice, piShader);
}

HRESULT APIENTRY D3DHooks::Hook_SetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) {
	D3DVERTEXBUFFER_DESC desc;

	if (Stride == 32 && NumElements == 8 && VectorCount == 4 && decl->Type == 2) { // Remove Line Markers
		pStreamData->GetDesc(&desc);
		vertexBufferSize = desc.Size;
	}

	return oSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, Stride);
}

HRESULT APIENTRY D3DHooks::Hook_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) { // Gotta do this so that ALT TAB-ing out of the game doesn't mess the whole thing up
	ImGui_ImplDX9_InvalidateDeviceObjects();

	MemHelpers::DX9DrawText("ERROR: Resetting Text (Lost)", 0xFF000000, 0, 150, 300, 450, pDevice, 1);

	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects();

	MemHelpers::DX9DrawText("ERROR: Resetting Text (Reset)", 0xFF000000, 0, 150, 300, 450, pDevice, 2);

	return ResetReturn;
}

HRESULT APIENTRY D3DHooks::Hook_DIP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimCount) {
	static bool calculatedCRC = false, calculatedHeadstocks = false, calculatedSkyline = false;

	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	// This could potentially lead to game locking up (because DIP is called multiple times per frame) if that value is not filled, but generally it should work 
	if (Settings::ReturnSettingValue("ExtendedRangeEnabled").length() < 2) { // Due to some weird reasons, sometimes settings decide to go missing - this may solve the problem
		Settings::UpdateSettings();
		D3D::GenerateTextures(pDevice, D3D::Custom);
		std::cout << "Reloaded settings" << std::endl;
	}

	if (setAllToNoteGradientTexture) {
		pDevice->SetTexture(currStride, gradientTextureSeven);
		return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);
	}

	Mesh current(Stride, PrimCount, NumVertices);
	ThiccMesh currentThicc(Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements);

	if (debug) {
		if (GetAsyncKeyState(VK_PRIOR) & 1 && currIdx < std::size(allMeshes) - 1)// Page up
			currIdx++;
		if (GetAsyncKeyState(VK_NEXT) & 1 && currIdx > 0) // Page down
			currIdx--;

		if (GetAsyncKeyState(VK_END) & 1) // Toggle logging
			startLogging = !startLogging;

		if (GetAsyncKeyState(VK_F8) & 1) { // Save logged meshes to file
			for (auto mesh : allMeshes) {
				//Log(mesh.ToString().c_str());
			}
		}

		if (GetAsyncKeyState(VK_F7) & 1) { // Save only removed 
			for (auto mesh : removedMeshes) {
				//Log(mesh.ToString().c_str());
			}
		}

		if (GetAsyncKeyState(VK_CONTROL) & 1)
			//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements);

			if (startLogging) {
				if (std::find(allMeshes.begin(), allMeshes.end(), currentThicc) == allMeshes.end()) // Make sure we don't log what we'd already logged
					allMeshes.push_back(currentThicc);
				//Log("{ %d, %d, %d},", Stride, PrimCount, NumVertices); // Log Current Texture -> Mesh
				//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements); // Log Current Texture -> ThiccMesh
				//Log("%s", currentMenu.c_str()); // Log Current Menu
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
			return D3D_OK;
		}

		if (IsExtraRemoved(removedMeshes, currentThicc))
			return D3D_OK;
	}

	// Mods

	if (Settings::ReturnSettingValue("DiscoModeEnabled") == "on") {
		// Need Lovro's Help With This :(
		//if (DiscoModeInitialSetting.find(pDevice) == DiscoModeInitialSetting.end()) { // We haven't saved this pDevice's initial values yet
		//	DWORD initialAlphaValue = (DWORD)pDevice, initialSeperateValue = (DWORD)pDevice;
		//	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD*)initialAlphaValue);
		//	pDevice->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD*)initialSeperateValue);
		//	
		//	DiscoModeInitialSetting.insert({ pDevice, std::make_pair(initialAlphaValue, initialSeperateValue) });
		//}
		//else { // We've seen this pDevice value before.
		//	if (DiscoModeEnabled) { // Key was pressed to have Disco Mode on
		//		pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); // Make AMPS Semi-Transparent <- Is the one that makes things glitchy.
		//		pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE); // Sticky Colors
		//	}

		//	else { // Disco mode was turned off, we need to revert the settings so there is no trace of disco mode.
		//		for (auto pDeviceList : DiscoModeInitialSetting) {
		//			pDeviceList.first->SetRenderState(D3DRS_ALPHABLENDENABLE, *(DWORD*)pDeviceList.second.first); // Needs to have *(DWORD*) since it only sets DWORD not DWORD*
		//			pDeviceList.first->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, *(DWORD*)pDeviceList.second.second); // Needs to have *(DWORD*) since it only sets DWORD not DWORD*
		//		}
		//	}
		//}
	}

	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" && MemHelpers::IsExtendedRangeSong() || Settings::GetModSetting("CustomStringColors") == 2) { // Extended Range Mode
		MemHelpers::ToggleCB(MemHelpers::IsExtendedRangeSong());

		if (IsToBeRemoved(sevenstring, current))  // Change all pieces of note head's textures
			pDevice->SetTexture(1, ourTexture);
		else if (FRETNUM_AND_MISS_INDICATOR) { // Colors for note stems (part below the note), and note accents
			pDevice->GetTexture(1, &pBaseTexture);
			pCurrTexture = (IDirect3DTexture9*)pBaseTexture;

			if (!pBaseTexture)
				return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);

			if (CRCForTexture(pCurrTexture, crc)) { // 0x00090000 - fretnums on noteway, 0x005a00b9 - noteway lanes, 0x00035193 -noteway bgd, 0x00004a4a-noteway lanes left and right of chord shape
				//if (startLogging)
				//	Log("0x%08x", crc);

				if (crc == 0x02a50002)  // Same checksum for stems and accents, because they use the same texture
					pDevice->SetTexture(1, ourTexture);
			}

			return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);
		}
	}

	if (Settings::IsTwitchSettingEnabled("RemoveNotes"))
		if (IsToBeRemoved(sevenstring, current) || FRETNUM_AND_MISS_INDICATOR)
			return D3D_OK;

	if (Settings::IsTwitchSettingEnabled("TransparentNotes"))
		if (IsToBeRemoved(sevenstring, current) || FRETNUM_AND_MISS_INDICATOR)
			pDevice->SetTexture(1, nonexistentTexture);
	
	if (Settings::IsTwitchSettingEnabled("SolidNotes")) {
		if (IsToBeRemoved(sevenstring, current) || FRETNUM_AND_MISS_INDICATOR) {
			if (Settings::ReturnSettingValue("SolidNoteColor") == "random") // Random Colors
				pDevice->SetTexture(1, randomTextures[currentRandomTexture]);
			else { // They set the color they want in the GUI | TODO: Colors are changed on chord boxes
				Color userDefColor = Settings::ConvertHexToColor(Settings::ReturnSettingValue("SolidNoteColor"));
				unsigned int red = userDefColor.r * 255, green = userDefColor.g * 255, blue = userDefColor.b * 255;

				std::cout << "Red: " << red << " Green: " << green << " Blue: " << blue << std::endl;

				D3D::GenerateSolidTexture(pDevice, &twitchUserDefinedTexture, D3DCOLOR_ARGB(255, red, green, blue));
				pDevice->SetTexture(1, twitchUserDefinedTexture);
			}
		}
	}
		

	if (GreenScreenWall && IsExtraRemoved(greenScreenWallMesh, currentThicc))
		return D3D_OK;

	if (MemHelpers::IsInStringArray(currentMenu, NULL, songModes)) {
		if (Settings::ReturnSettingValue("FretlessModeEnabled") == "on" && IsExtraRemoved(fretless, currentThicc))
			return D3D_OK;
		if (Settings::ReturnSettingValue("RemoveInlaysEnabled") == "on" && IsExtraRemoved(inlays, currentThicc))
			return D3D_OK;
		if (Settings::ReturnSettingValue("RemoveLaneMarkersEnabled") == "on" && IsExtraRemoved(laneMarkers, currentThicc))
			return D3D_OK;
		if (RemoveLyrics && Settings::ReturnSettingValue("RemoveLyrics") == "on" && IsExtraRemoved(lyrics, currentThicc))
			return D3D_OK;
	}

	else if (MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus) && Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && RemoveHeadstockInThisMenu)
	{
		if (IsExtraRemoved(tuningLetters, currentThicc)) // This is called to remove those pesky tuning letters that share the same texture values as fret numbers and chord fingerings
			return D3D_OK;
		if (IsExtraRemoved(tunerHighlight, currentThicc)) // This is called to remove the tuner's highlights
			return D3D_OK;
		if (IsExtraRemoved(leftyFix, currentThicc)) // Lefties need their own little place in life...
			return D3D_OK;
	}

	if (toggleSkyline && Stride == 16) {
		if (DrawSkylineInMenu) { // If the user is in "Song" mode for Toggle Skyline and is NOT in a song -> draw the UI
			SkylineOff = false;
			return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);
		}

		// Skyline Removal
		pDevice->GetTexture(1, &pBaseTextures[1]);
		pCurrTextures[1] = (IDirect3DTexture9*)pBaseTextures[1];

		if (pBaseTextures[1]) {  // There's only two textures in Stage 1 for meshes with Stride = 16, so we could as well skip CRC calcuation and just check if !pBaseTextures[1] and return D3D_OK directly
			if (CRCForTexture(pCurrTextures[1], crc)) {
				if (crc == 0x65b846aa || crc == 0xbad9e064) { // Purple rectangles + orange line beneath them
					SkylineOff = true;
					return D3D_OK;
				}
			}
		}

		pDevice->GetTexture(0, &pBaseTextures[0]);
		pCurrTextures[0] = (IDirect3DTexture9*)pBaseTextures[0];

		if (pBaseTextures[0]) {
			if (CRCForTexture(pCurrTextures[0], crc)) {
				if (crc == 0xc605fbd2 || crc == 0xff1c61ff) {  // There's a few more of textures used in Stage 0, so doing the same is no-go; Shadow-ish thing in the background + backgrounds of rectangles
					SkylineOff = true;
					return D3D_OK;
				}
			}
		}
	}

	else if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on") {
		if (Stride == 44 || Stride == 56 || Stride == 60 || Stride == 68 || Stride == 76 || Stride == 84) { // If we call GetTexture without any filtering, it causes a lockup when ALT-TAB-ing/changing fullscreen to windowed and vice versa

			if (!RemoveHeadstockInThisMenu) // This user has RemoveHeadstock only on during the song. So if we aren't in the song, we need to draw the headstock texture.
				return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);

			pDevice->GetTexture(1, &pBaseTextures[1]);
			pCurrTextures[1] = (IDirect3DTexture9*)pBaseTextures[1];

			if (resetHeadstockCache && IsExtraRemoved(headstockThicc, currentThicc)) {
				if (!pBaseTextures[1]) //if there's no texture for Stage 1
					return D3D_OK;

				if (CRCForTexture(pCurrTextures[1], crc)) {
					if (crc == 0x008d5439 || crc == 0x000d4439 || crc == 0x00000000 || crc == 0xa55470f6 || crc == 0x008f4039)
						AddToTextureList(headstockTexturePointers, pCurrTextures[1]);
				}

				//Log("0x%08x", crc);

				if (headstockTexturePointers.size() == 3) {
					calculatedHeadstocks = true;
					resetHeadstockCache = false;
					std::cout << "Calculated headstock CRCs (Menu: " << currentMenu << " )" << std::endl;
				}

				return D3D_OK;
			}

			if (calculatedHeadstocks)
				if (std::find(std::begin(headstockTexturePointers), std::end(headstockTexturePointers), pCurrTextures[1]) != std::end(headstockTexturePointers))
					return D3D_OK;
		}
	}

	return oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount); // KEEP THIS LINE. It means for it to display the graphics!
}



