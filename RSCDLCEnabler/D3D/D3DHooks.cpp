#include "D3DHooks.hpp"


HRESULT APIENTRY D3DHooks::Hook_DP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT StartIndex, UINT PrimCount) { // Mainly used for Note Tails
	if (pDevice->GetStreamSource(0, &Stream_Data, &Offset, &Stride) == D3D_OK)
		Stream_Data->Release();

	// Extended Range / Custom Colors
	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" && NOTE_TAILS) {
		MemHelpers::ToggleCB(MemHelpers::IsExtendedRangeSong());
		pDevice->SetTexture(1, ourTexture); // For random textures, use randomTextures[currentRandTexture]
	}

	// Solid Notes Twitch Reward
	if (Settings::IsTwitchSettingEnabled("SolidNotes") && NOTE_TAILS) {
		if (Settings::ReturnSettingValue("SolidNoteColor") == "random")
			pDevice->SetTexture(1, randomTextures[currentRandomTexture]);
		else
			pDevice->SetTexture(1, twitchUserDefinedTexture);
	}
	
	// Rainbow Notes
	if (ERMode::RainbowEnabled && ERMode::customNoteColorH > 0 && NOTE_TAILS) // Rainbow Notes
		pDevice->SetTexture(1, rainbowTextures[ERMode::customNoteColorH]);

	return oDrawPrimitive(pDevice, PrimType, StartIndex, PrimCount);
}

HRESULT APIENTRY D3DHooks::Hook_SetVertexDeclaration(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexDeclaration9* pdecl) {
	if (pdecl != NULL)
		pdecl->GetDeclaration(decl, &NumElements);

	return oSetVertexDeclaration(pDevice, pdecl);
}

HRESULT APIENTRY D3DHooks::Hook_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 pDevice, UINT i_StartRegister, const float* pConstantData, UINT Vector4fCount) {
	if (pConstantData != NULL) {
		StartRegister = i_StartRegister;
		VectorCount = Vector4fCount;
	}

	return oSetVertexShaderConstantF(pDevice, i_StartRegister, pConstantData, Vector4fCount);
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

HRESULT APIENTRY D3DHooks::Hook_SetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT i_Stride) {
	D3DVERTEXBUFFER_DESC desc;

	// Remove Line Markers
	if (i_Stride == 32 && NumElements == 8 && VectorCount == 4 && decl->Type == 2) { 
		pStreamData->GetDesc(&desc);
		vertexBufferSize = desc.Size;
	}

	return oSetStreamSource(pDevice, StreamNumber, pStreamData, OffsetInBytes, i_Stride);
}

HRESULT APIENTRY D3DHooks::Hook_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) { // Gotta do this so that ALT+TAB-ing out of the game doesn't mess the whole thing up
	// Lost Device
	ImGui_ImplDX9_InvalidateDeviceObjects();

	if (MemHelpers::DX9FontEncapsulation)
		MemHelpers::DX9FontEncapsulation->OnLostDevice();

	// Reset Device
	HRESULT ResetReturn = oReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects();

	if (MemHelpers::DX9FontEncapsulation)
		MemHelpers::DX9FontEncapsulation->OnResetDevice();

	return ResetReturn;
}

HRESULT APIENTRY D3DHooks::Hook_DIP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimCount) { // Draw things on screen
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
		return SHOW_TEXTURE;
	}

	Mesh current(Stride, PrimCount, NumVertices);
	ThiccMesh currentThicc(Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements);

	if (debug) {
		if (GetAsyncKeyState(VK_PRIOR) & 1 && currIdx < std::size(allMeshes) - 1)// Page up
			currIdx++;
		if (GetAsyncKeyState(VK_NEXT) & 1 && currIdx > 0) // Page down
			currIdx--;

		if (GetAsyncKeyState(VK_END) & 1) { // Toggle logging
			std::cout << "Logging is ";
			startLogging = !startLogging;
			if (!startLogging)
				std::cout << "no longer ";
			std::cout << "armed!" << std::endl;
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
				if (Stride == 32 && PrimCount == 2 && NumVertices == 4) // Criteria for search
					std::cout << "{ " << Stride << ", " << PrimCount << ", " << NumVertices << ", " << StartIndex << ", " << StartRegister << ", " << (UINT)PrimType << ", " << (UINT)decl->Type << ", " << VectorCount << ", " << NumElements << " },"<< std::endl; // Thicc Mesh -> Console
				//std::cout << "{ "<< Stride << ", " << PrimCount << ", " << NumVertices << " }," std::endl; // Mesh -> Console
				//Log("{ %d, %d, %d},", Stride, PrimCount, NumVertices); // Mesh -> Log File
				//Log("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }, ", Stride, PrimCount, NumVertices, StartIndex, StartRegister, PrimType, decl->Type, VectorCount, NumElements); // ThiccMesh -> Log File
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

	// Rainbow Notes | This part NEEDS to be above Extended Range / Custom Colors or it won't work.
	if (ERMode::RainbowEnabled && ERMode::customNoteColorH > 0) { 

		if (ERMode::customNoteColorH > 179)
			ERMode::customNoteColorH -= 180;

		RainbowNotes = true;
		if (NOTE_STEMS) {
			pDevice->GetTexture(1, &pBaseRainbowTexture);
			pCurrRainbowTexture = (IDirect3DTexture9*)pBaseRainbowTexture;

			if (!pBaseRainbowTexture)
				return SHOW_TEXTURE;

			if (CRCForTexture(pCurrRainbowTexture, crc)) {
				if (crc == crcStemsAccents || crc == crcBendSlideIndicators)
					pDevice->SetTexture(1, rainbowTextures[ERMode::customNoteColorH]);
			}
		}

		if (PrideMode && NOTE_TAILS) // As of right now, this requires rainbow strings to be toggled on
			pDevice->SetTexture(1, rainbowTextures[ERMode::customNoteColorH]);
	}
		
	//if (Settings::ReturnSettingValue("DiscoModeEnabled") == "on") {
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
	//}

	// Extended Range / Custom Colors
	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" && MemHelpers::IsExtendedRangeSong() || Settings::GetModSetting("CustomStringColors") == 2) { // Extended Range Mode
		MemHelpers::ToggleCB(MemHelpers::IsExtendedRangeSong());

		if (IsToBeRemoved(sevenstring, current))  // Change all pieces of note head's textures
			pDevice->SetTexture(1, ourTexture);

		else if (NOTE_STEMS) { // Colors for note stems (part below the note), bends, slides, and accents
			pDevice->GetTexture(1, &pBaseTexture);
			pCurrTexture = (IDirect3DTexture9*)pBaseTexture;

			if (!pBaseTexture)
				return SHOW_TEXTURE;

			if (CRCForTexture(pCurrTexture, crc)) {
				//if (startLogging)
				//	Log("0x%08x", crc);

				if (crc == crcStemsAccents || crc == crcBendSlideIndicators)  // Same checksum for stems and accents, because they use the same texture. Bends and slides use the same texture.
					pDevice->SetTexture(1, ourTexture);
			}

			return SHOW_TEXTURE;
		}
	}

	// Twitch Settings
	if (Settings::IsTwitchSettingEnabled("RemoveNotes"))
		if (IsToBeRemoved(sevenstring, current) || NOTE_STEMS)
			return REMOVE_TEXTURE;

	if (Settings::IsTwitchSettingEnabled("TransparentNotes"))
		if (IsToBeRemoved(sevenstring, current) || NOTE_STEMS)
			pDevice->SetTexture(1, nonexistentTexture);

	if (Settings::IsTwitchSettingEnabled("SolidNotes")) {
		if (IsToBeRemoved(sevenstring, current) || NOTE_STEMS) {
			if (Settings::ReturnSettingValue("SolidNoteColor") == "random") // Random Colors
				pDevice->SetTexture(1, randomTextures[currentRandomTexture]);
			else // They set the color they want in the GUI | TODO: Colors are changed on chord boxes
				pDevice->SetTexture(1, twitchUserDefinedTexture);
		}
	}

	if (Settings::IsTwitchSettingEnabled("FYourFC")) {
		uintptr_t currentNoteStreak = 0;

		if (MemHelpers::IsInStringArray(currentMenu, 0, learnASongModes))
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakLASOffsets);
		else if (MemHelpers::IsInStringArray(currentMenu, 0, scoreAttackModes))
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakSAOffsets);

		if (currentNoteStreak != 0)
			*(BYTE*)currentNoteStreak = 0;
	}

	if (Settings::IsTwitchSettingEnabled("DrunkMode")) {
		std::uniform_real_distribution<> keepValueWithin(-1.5, 1.5);
		*(float*)Offsets::ptr_drunkShit = (float)keepValueWithin(rng);
	}

	// Greenscreen Wall
	if (GreenScreenWall && IsExtraRemoved(greenScreenWallMesh, currentThicc))
		return REMOVE_TEXTURE;

	// Thicc Mesh Mods
	if (MemHelpers::IsInStringArray(currentMenu, NULL, songModes)) {
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
	else if (MemHelpers::IsInStringArray(currentMenu, NULL, tuningMenus) && Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" && RemoveHeadstockInThisMenu)
	{
		if (IsExtraRemoved(tuningLetters, currentThicc)) // This is called to remove those pesky tuning letters that share the same texture values as fret numbers and chord fingerings
			return REMOVE_TEXTURE;
		if (IsExtraRemoved(tunerHighlight, currentThicc)) // This is called to remove the tuner's highlights
			return REMOVE_TEXTURE;
		if (IsExtraRemoved(leftyFix, currentThicc)) // Lefties need their own little place in life...
			return REMOVE_TEXTURE;
	}

	// Skyline Removal
	if (toggleSkyline && POSSIBLE_SKYLINE) {
		if (DrawSkylineInMenu) { // If the user is in "Song" mode for Toggle Skyline and is NOT in a song -> draw the UI
			SkylineOff = false;
			return SHOW_TEXTURE;
		}

		pDevice->GetTexture(1, &pBaseTextures[1]);
		pCurrTextures[1] = (IDirect3DTexture9*)pBaseTextures[1];

		if (pBaseTextures[1]) {  // There's only two textures in Stage 1 for meshes with Stride = 16, so we could as well skip CRC calcuation and just check if !pBaseTextures[1] and return D3D_OK directly
			if (CRCForTexture(pCurrTextures[1], crc)) {
				if (crc == crcSkylinePurple || crc == crcSkylineOrange) { // Purple rectangles + orange line beneath them
					SkylineOff = true;
					return REMOVE_TEXTURE;
				}
			}
		}

		pDevice->GetTexture(0, &pBaseTextures[0]);
		pCurrTextures[0] = (IDirect3DTexture9*)pBaseTextures[0];

		if (pBaseTextures[0]) {
			if (CRCForTexture(pCurrTextures[0], crc)) {
				if (crc == crcSkylineBackground || crc == crcSkylineShadow) {  // There's a few more of textures used in Stage 0, so doing the same is no-go; Shadow-ish thing in the background + backgrounds of rectangles
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

			if (resetHeadstockCache && IsExtraRemoved(headstockThicc, currentThicc)) {
				if (!pBaseTextures[1]) //if there's no texture for Stage 1
					return REMOVE_TEXTURE;

				if (CRCForTexture(pCurrTextures[1], crc)) {
					if (crc == crcHeadstock0 || crc == crcHeadstock1 || crc == crcHeadstock2 || crc == crcHeadstock3 || crc == crcHeadstock4)
						AddToTextureList(headstockTexturePointers, pCurrTextures[1]);
				}

				//Log("0x%08x", crc);

				if (headstockTexturePointers.size() == 3) {
					calculatedHeadstocks = true;
					resetHeadstockCache = false;
					std::cout << "Calculated headstock CRCs (Menu: " << currentMenu << " )" << std::endl;
				}

				return REMOVE_TEXTURE;
			}

			if (calculatedHeadstocks)
				if (std::find(std::begin(headstockTexturePointers), std::end(headstockTexturePointers), pCurrTextures[1]) != std::end(headstockTexturePointers))
					return REMOVE_TEXTURE;
		}
	}

	// Rainbow Notes || This part NEEDS to be below Extended Range / Custom Colors or it won't work.
	if (RainbowNotes) { 
		if (IsToBeRemoved(sevenstring, current)) // Note Heads
			pDevice->SetTexture(1, rainbowTextures[ERMode::customNoteColorH]);
		RainbowNotes = false;
	}

	return SHOW_TEXTURE; // KEEP THIS LINE. It means for it to display the graphics!
}