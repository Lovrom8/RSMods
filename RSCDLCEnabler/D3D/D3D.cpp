#include "D3D.hpp"
#include "../Mods/ExtendedRangeMode.hpp"
#include "../Mods/CollectColors.hpp"

void D3D::SetCustomColors() {
	for (int strIdx = 0; strIdx < 6;strIdx++) {
		ColorMap customColorsFull;

		ColorMap normalColors = GetCustomColors(strIdx, false);
		ColorMap cbColors = GetCustomColors(strIdx, true);


		customColorsFull.insert(normalColors.begin(), normalColors.end());
		customColorsFull.insert(cbColors.begin(), cbColors.end());

		ERMode::SetCustomColors(strIdx, customColorsFull);
	}
}

ColorMap D3D::GetCustomColors(int strIdx, bool CB) {
	RSColor iniColor;
	std::string ext;

	if (CB)
		ext = "_CB";
	else
		ext = "_N";

	iniColor = Settings::GetCustomColors(CB)[strIdx];
	int H;
	float S, L;
	CollectColors::RGB2HSL(iniColor.r, iniColor.g, iniColor.b, H, S, L);

	ColorMap customColors = {
		{"Ambient" + ext, CollectColors::GetAmbientStringColor(H, CB)},
		{"Disabled" + ext, CollectColors::GetDisabledStringColor(H, S, L, CB)},
		{"Enabled" + ext, iniColor},
		{"Glow" + ext, CollectColors::GetGlowStringColor(H)},
		{"PegsTuning" + ext, CollectColors::GetGlowStringColor(H)},
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

RSColor GenerateRandomColor() {
	RSColor rndColor;

	static std::uniform_real_distribution<> urd(0.0f, 1.0f);
	rndColor.r = (float)urd(rng);
	rndColor.g = (float)urd(rng);
	rndColor.b = (float)urd(rng);

	return rndColor;
}

void D3D::GenerateTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, ColorList colorSet) {
	while (GetModuleHandleA("gdiplus.dll") == NULL) // JIC, to prevent crashing
		Sleep(500);

	using namespace Gdiplus;
	UINT width = 256, height = 128;
	int lineHeight = 8;

	if (Ok != GdiplusStartup(&token_, &inp, NULL))
		std::cout << "GDI+ failed to start up!" << std::endl;

	Bitmap bmp(width, height, PixelFormat32bppARGB);
	Graphics graphics(&bmp);
	RSColor currColor;

	REAL blendPositions[] = { 0.0f, 0.4f, 1.0f };

	for (int i = 0; i < 16;i++) {
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
		graphics.FillRectangle(&whiteCoverupBrush, width - 3, i * lineHeight, width, lineHeight); // Don't hate me for this hacky fix to the black bars.
	}

	// Uncomment if you want to save the generated texture
	/*CLSID pngClsid;
	CLSIDFromString(L"{557CF406-1A04-11D3-9A73-000F81EF32E}", &pngClsid); //for BMP: {557cf400-1a04-11d3-9a73-0000f81ef32e}
	bmp.Save(L"generatedTexture.png", &pngClsid); */

	BitmapData bitmapData;
	D3DLOCKED_RECT lockedRect;

	D3DXCreateTexture(pDevice, width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture);

	(*ppTexture)->LockRect(0, &lockedRect, 0, 0);

	bmp.LockBits(&Rect(0, 0, width, height), ImageLockModeRead, PixelFormat32bppARGB, &bitmapData); // Strings

	unsigned char* pSourcePixels = (unsigned char*)bitmapData.Scan0;
	unsigned char* pDestPixels = (unsigned char*)lockedRect.pBits;

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

void D3D::GenerateTextures(IDirect3DDevice9* pDevice, TextureType type) {
	ColorList colorSet;

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
			randomTextureColors.push_back(colorSet);

			colorSet.clear();
		}
	}
	else if (type == Rainbow) {
		float h = 0.0f, stringOffset = 20.0f;
		int currTexture = 0;

		Color c;
		ColorList colorsRainbow;

		while (h < 360.f) {
			h += rainbowSpeed;

			for (int i = 0; i < 8;i++) { // There's two extra colors per string, so we may need to think about this a bit more
				c.setH(h + (stringOffset * i));

				colorsRainbow.push_back(c);
			}

			colorSet.insert(colorSet.begin(), colorsRainbow.begin(), colorsRainbow.end()); // Both CB and regular colors will still look the same in rainbow mode
			colorSet.insert(colorSet.end(), colorsRainbow.begin(), colorsRainbow.end());

			GenerateTexture(pDevice, &rainbowTextures[currTexture], colorSet);

			colorSet.clear();
			colorsRainbow.clear();

			currTexture++;
		}
	}
	else if (type == Custom) {
		ColorList colorsN = Settings::GetCustomColors(false);
		ColorList colorsCB = Settings::GetCustomColors(true);

		colorSet.insert(colorSet.begin(), colorsN.begin(), colorsN.end());
		colorSet.insert(colorSet.end(), colorsCB.begin(), colorsCB.end());

		GenerateTexture(pDevice, &ourTexture, colorSet);
	}
}

HRESULT D3D::GenerateSolidTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppD3Dtex, DWORD colour32) {
	if (FAILED(pDevice->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);

	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD* pDst16 = (WORD*)d3dlr.pBits;

	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;

	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}
