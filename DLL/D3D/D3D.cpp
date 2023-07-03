#include "D3D.hpp"
#include "../Mods/ExtendedRangeMode.hpp"
#include "../Mods/CollectColors.hpp"

/// <summary>
/// Apply Custom String Colors
/// </summary>
void D3D::SetCustomColors() {
	// Fpr each string
	for (int strIdx = 0; strIdx < 6;strIdx++) {
		ColorMap customColorsFull;

		// Get normal and colorblind colors.
		ColorMap normalColors = GetCustomColors(strIdx, false);
		ColorMap cbColors = GetCustomColors(strIdx, true);

		// Save Colors into ColorMap.
		customColorsFull.insert(normalColors.begin(), normalColors.end());
		customColorsFull.insert(cbColors.begin(), cbColors.end());

		// Set Custom Colors
		ERMode::SetCustomColors(strIdx, customColorsFull);
	}
}

/// <param name="strIdx"> - Current String thickest to thinnest (zero-indexed)</param>
/// <param name="CB"> - Are we using colorblind colors?</param>
/// <returns>List of All Potential Colors for a String</returns>
ColorMap D3D::GetCustomColors(int strIdx, bool CB) {
	RSColor iniColor;
	std::string ext;

	if (CB)
		ext = "_CB";
	else
		ext = "_N";

	// Get user-defined string color
	iniColor = Settings::GetStringColors(CB)[strIdx];
	int H;
	float S, L;
	CollectColors::RGB2HSL(iniColor.r, iniColor.g, iniColor.b, H, S, L);

	// Create different colors from the user-defined color.
	ColorMap customColors = {
		{"Ambient" + ext, CollectColors::GetAmbientStringColor(H, CB)},
		{"Disabled" + ext, CollectColors::GetDisabledStringColor(H, S, L, CB)},
		{"Enabled" + ext, iniColor},
		{"Glow" + ext, CollectColors::GetGlowStringColor(H)},
		{"PegsTuning" + ext, CollectColors::GetTuningPegColor(H)},
		{"PegsReset" + ext, CollectColors::GetPegResetColor()},
		{"PegsSuccess" + ext, CollectColors::GetPegSuccessColor(CB)},
		{"PegsInTune" + ext, CollectColors::GetPegInTuneColor(H, CB)},
		{"PegsOutTune" + ext, CollectColors::GetPegOutTuneColor()},
		{"TextIndicator" + ext, CollectColors::GetRegTextIndicatorColor(H, CB)},
		{"ForkParticles" + ext, CollectColors::GetRegForkParticlesColor(H, CB)},
		{"NotewayNormal" + ext, CollectColors::GetNotewayNormalColor(H, S, L, CB)},
		{"NotewayAccent" + ext, CollectColors::GetNotewayAccentColor(H, CB)},
		{"NotewayPreview" + ext, CollectColors::GetNotewayPreviewColor(H, CB)},
		{"GC_Main" + ext, CollectColors::GetGuitarcadeMainColor(H, strIdx, CB)},
		{"GC_Add" + ext, CollectColors::GetGuitarcadeAdditiveColor(H, strIdx, CB)},
		{"GC_UI" + ext, CollectColors::GetGuitarcadeUIColor(H, strIdx, CB)}
	};

	return customColors;
}

/// <returns>Random Color</returns>
RSColor GenerateRandomColor() {
	RSColor rndColor;

	static std::uniform_real_distribution<> urd(0.0f, 1.0f);
	rndColor.r = (float)urd(rng);
	rndColor.g = (float)urd(rng);
	rndColor.b = (float)urd(rng);

	return rndColor;
}

/// <summary>
/// Generate a texture to later be used to override a texture.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="ppTexture"> - Output Texture</param>
/// <param name="colorSet"> - List of colors to jam into the texture</param>
/// <param name="in_width"> - How wide should the texture be?</param>
/// <param name="in_height"> - How tall should the texture be?</param>
/// <param name="in_lineHeight"> - How thick should each line be?</param>
/// <param name="howManyLines"> - How many lines should there be?</param>
void D3D::GenerateTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, ColorList colorSet, UINT in_width, UINT in_height, int in_lineHeight, int howManyLines) {
	_LOG_INIT;
	_LOG_SETLEVEL(LogLevel::Error);

	// JIC, to prevent crashing
	while (GetModuleHandleA("gdiplus.dll") == NULL) 
		Sleep(500);

	using namespace Gdiplus;

	// Initialize GDI+
	UINT width = in_width, height = in_height;
	int lineHeight = in_lineHeight;

	if (Ok != GdiplusStartup(&token_, &inp, NULL))
	{
		_LOG("GDI+ failed to start up!" << std::endl);
	}
		
	// Create bitmap image
	Bitmap bmp(width, height, PixelFormat32bppARGB);
	Graphics graphics(&bmp);
	RSColor currColor;

	REAL blendPositions[] = { 0.0f, 0.4f, 1.0f };

	// Create string color bitmap.
	for (int i = 0; i < howManyLines; i++) {
		currColor = colorSet[i]; // If we are in range of 0-7, grab the normal colors, otherwise grab CB colors

		Gdiplus::Color middleColor(currColor.r * 255, currColor.g * 255, currColor.b * 255); // Notes

		Gdiplus::Color gradientColors[] = { Gdiplus::Color::Black, middleColor , Gdiplus::Color::White };
		LinearGradientBrush linGrBrush( // Base texture for note gradients (top / normal)
			Point(0, 0),
			Point(width, lineHeight),
			Gdiplus::Color::Black,
			Gdiplus::Color::White);
		LinearGradientBrush whiteCoverupBrush( // Coverup for some spotty gradients (top / normal)
			Point(width - 3, lineHeight * 5),
			Point(width, height),
			Gdiplus::Color::White,
			Gdiplus::Color::White);

		linGrBrush.SetInterpolationColors(gradientColors, blendPositions, 3);
		graphics.FillRectangle(&linGrBrush, 0, i * lineHeight, width, lineHeight);
		graphics.FillRectangle(&whiteCoverupBrush, width - 3, i * lineHeight, width, lineHeight);
	}

	// Uncomment if you want to save the generated texture
	/*CLSID pngClsid;
	CLSIDFromString(L"{557CF406-1A04-11D3-9A73-000F81EF32E}", &pngClsid); //for BMP: {557cf400-1a04-11d3-9a73-0000f81ef32e}
	bmp.Save(L"generatedTexture.png", &pngClsid); */

	BitmapData bitmapData;
	D3DLOCKED_RECT lockedRect;

	// Create texture from bitmap.
	HRESULT hr_D3DX = D3DXCreateTexture(pDevice, width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture);

	if (*ppTexture == NULL) { // User is spam updating their INI through the GUI. D3DX textures are becoming NULL references.
		_LOG("User is spam updating their INI through the GUI. D3DXCreateTexture returned ");

		switch (hr_D3DX) {
			case (D3D_OK):
				_LOG_NOHEAD("D3D_OK" << std::endl);
				break;
			case (D3DERR_INVALIDCALL):
				_LOG_NOHEAD("D3DERR_INVALIDCALL" << std::endl);
				break;
			case (D3DERR_NOTAVAILABLE):
				_LOG_NOHEAD("D3DERR_NOTAVAILABLE" << std::endl);
				break;
			case (D3DERR_OUTOFVIDEOMEMORY):
				_LOG_NOHEAD("D3DERR_OUTOFVIDEOMEMORY" << std::endl);
				break;
			case (E_OUTOFMEMORY):
				_LOG_NOHEAD("E_OUTOFMEMORY" << std::endl);
				break;
			default: // Non-documented error
				_LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
				break;
		}
		return;
	}
	
	// Lock texture and lock bitmap.
	(*ppTexture)->LockRect(0, &lockedRect, 0, 0);

	bmp.LockBits(&Rect(0, 0, width, height), ImageLockModeRead, PixelFormat32bppARGB, &bitmapData); // Strings

	unsigned char* pSourcePixels = (unsigned char*)bitmapData.Scan0;
	unsigned char* pDestPixels = (unsigned char*)lockedRect.pBits;

	// Copy colors from bitmap into locked rect.
	for (unsigned int y = 0; y < height; ++y)
	{
		// copy a row
		memcpy(pDestPixels, pSourcePixels, width * 4);   // 4 bytes per pixel

		// advance row pointers
		pSourcePixels += bitmapData.Stride;
		pDestPixels += lockedRect.Pitch;
	}

	(*ppTexture)->UnlockRect(0);

	//D3DXSaveTextureToFileA("generatedTexture_d3d.dds", D3DXIFF_DDS, (*ppTexture), 0);

	D3D::SetCustomColors();
}

/// <summary>
/// Generate Texture Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="type"> - What type of texture should be created? (enum D3D::TextureType)</param>
void D3D::GenerateTextures(IDirect3DDevice9* pDevice, TextureType type) {
	ColorList colorSet;
	
	// Create Random colors.
	if (type == Random || type == Random_Solid) {
		for (int textIdx = 0; textIdx < randomTextureCount;textIdx++) {
			if (type == Random_Solid) {
				RSColor iniColor = GenerateRandomColor();

				for (int i = 0; i < 16; i++) 
					colorSet.push_back(iniColor);
			}
			else {
				for (int i = 0; i < 16; i++)
					colorSet.push_back(GenerateRandomColor());
			}

			GenerateTexture(pDevice, &randomTextures[textIdx], colorSet);
			randomTextureColors = ColorList(colorSet);

			colorSet.clear();
		}
	}
	// Create Rainbow colors.
	else if (type == Rainbow) {
		float h = 0.0f, stringOffset = 20.0f;
		int currTexture = 0;

		Color c;
		ColorList colorsRainbow;

		while (h < 360.f) {
			h += rainbowSpeed;

			// There's two extra colors per string, so we may need to think about this a bit more
			for (int i = 0; i < 6;i++) { 
				c.setH(h + (stringOffset * i));

				if (c.r > 1.0f)
					c.r = c.r - (c.r - 1.0f);
				if (c.r < .0f)
					c.r *= -1;

				if (c.g > 1.0f)
					c.g = c.g - (c.g - 1.0f);
				if (c.g < .0f)
					c.g *= -1;

				if (c.b > 1.0f)
					c.b = c.b - (c.b - 1.0f);
				if (c.b < .0f)
					c.b *= -1;

				colorsRainbow.push_back(c);
			}

			for (int i = 0; i < 2;i++)
				colorsRainbow.push_back(c);

			// Both CB and regular colors will still look the same in rainbow mode
			colorSet.insert(colorSet.begin(), colorsRainbow.begin(), colorsRainbow.end()); 
			colorSet.insert(colorSet.end(), colorsRainbow.begin(), colorsRainbow.end());

			GenerateTexture(pDevice, &rainbowTextures[currTexture], colorSet);

			colorSet.clear();
			colorsRainbow.clear();

			currTexture++;
		}
	}

	// Generate Custom String color Texture
	else if (type == Strings) {
		ColorList colorsN = Settings::GetStringColors(false);
		ColorList colorsCB = Settings::GetStringColors(true);

		colorSet.insert(colorSet.begin(), colorsN.begin(), colorsN.end());
		colorSet.insert(colorSet.end(), colorsCB.begin(), colorsCB.end());

		GenerateTexture(pDevice, &customStringColorTexture, colorSet);
	}

	// Generate Custom Note color Texture
	else if (type == Notes) {
		ColorList colorsN = Settings::GetNoteColors(false);
		ColorList colorsCB = Settings::GetNoteColors(true);

		colorSet.insert(colorSet.begin(), colorsN.begin(), colorsN.end());
		colorSet.insert(colorSet.end(), colorsCB.begin(), colorsCB.end());

		GenerateTexture(pDevice, &customNoteColorTexture, colorSet);
	}

	// Generate Custom Noteway color Texture
	else if (type == Noteway) {
		colorSet.insert(colorSet.begin(), Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayNumbered")));
		colorSet.insert(colorSet.end(), Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayUnNumbered")));

		GenerateTexture(pDevice, &notewayTexture, colorSet, 256, 32, 16, 2);
	}

	// Generate Custom Gutter color Texture
	else if (type == Gutter) {
		colorSet.insert(colorSet.begin(), Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayGutter")));
		GenerateTexture(pDevice, &gutterTexture, colorSet, 256, 16, 16, 1);
	}

	// Generate Custom FretNums color Texture
	else if (type == FretNums) {
		colorSet.insert(colorSet.begin(), Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomFretNubmers")));

		GenerateTexture(pDevice, &fretNumTexture, colorSet, 256, 16, 16, 1);
	}
}

/// <summary>
/// Generate a texture with one color.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="ppD3Dtex"> - Output Texture</param>
/// <param name="colour32"> - Color for Texture</param>
/// <returns>E_FAIL if a texture can't be created, S_OK if it was created.</returns>
HRESULT D3D::GenerateSolidTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppD3Dtex, DWORD colour32) {
	// Create Texture
	if (FAILED(pDevice->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	// Get the color
	WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);

	// Lock the texture
	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD* pDst16 = (WORD*)d3dlr.pBits;

	// Copy the color into the texture
	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;

	// Unlock the texture
	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}
