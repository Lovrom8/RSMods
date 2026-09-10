#include "../stdafx.h"
#include "D3D.hpp"

/// <summary>
/// Apply Custom String Colors
/// </summary>
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
	float S;
	float L;
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

void WaitForGdiPlus() {
	while (GetModuleHandleA("gdiplus.dll") == NULL) {
		Sleep(500);
	}
}

void ReleaseTexture(IDirect3DTexture9** ppTexture) {
	if (ppTexture && *ppTexture) {
		(*ppTexture)->Release();
		*ppTexture = nullptr;
	}
}

std::unique_ptr<Gdiplus::Bitmap> CreateGradientBitmap(UINT width, UINT height, ColorList colorSet, int lineHeight, int howManyLines, bool saveTextureToFile = false) {
	using namespace Gdiplus;

	auto bmp = std::make_unique<Bitmap>(width, height, PixelFormat32bppARGB);
	Graphics graphics(bmp.get());

	std::array<REAL, 3> blendPositions = { 0.0f, 0.4f, 1.0f };

	for (int i = 0; i < howManyLines; i++) {
		RSColor currColor = colorSet[i]; // If we are in range of 0-7, grab the normal colors, otherwise grab CB colors
		Color middleColor(static_cast<byte>(currColor.r * 255), static_cast<byte>(currColor.g * 255), static_cast<byte>(currColor.b * 255));
		std::array<Color, 3> gradientColors = { Color::Black, middleColor, Color::White };

		LinearGradientBrush linGrBrush( // Base texture for note gradients (top / normal)
			Point(0, 0),
			Point(width, lineHeight),
			Color::Black,
			Color::White
		);

		LinearGradientBrush whiteCoverupBrush( // Coverup for some spotty gradients (top / normal)
			Point(width - 3, lineHeight * 5),
			Point(width, height),
			Color::White,
			Color::White
		);

		linGrBrush.SetInterpolationColors(gradientColors.data(), blendPositions.data(), gradientColors.size());
		graphics.FillRectangle(&linGrBrush, 0, i * lineHeight, width, lineHeight);
		graphics.FillRectangle(&whiteCoverupBrush, width - 3, i * lineHeight, width, lineHeight);
	}

	if (saveTextureToFile) {
		CLSID pngClsid;

		if (CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid) >= 0) { // For BMP: {557cf400-1a04-11d3-9a73-0000f81ef32e}
			bmp->Save(L"generatedTexture.png", &pngClsid);
		}
	}

	return bmp;
}

bool CopyBitmapToTexture(IDirect3DTexture9* pTexture, Gdiplus::Bitmap& bitmap, UINT width, UINT height) {
	using namespace Gdiplus;

	BitmapData bitmapData;
	D3DLOCKED_RECT lockedRect;

	if (FAILED(pTexture->LockRect(0, &lockedRect, 0, 0))) {
		return false;
	}

	if (Rect rect(0, 0, width, height); Ok != bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData)) {
		pTexture->UnlockRect(0);
		return false;
	}

	auto pSourcePixels = static_cast<unsigned char*>(bitmapData.Scan0);
	auto pDestPixels = static_cast<unsigned char*>(lockedRect.pBits);

	for (unsigned int y = 0; y < height; ++y) {
		memcpy(pDestPixels, pSourcePixels, width * 4);
		pSourcePixels += bitmapData.Stride;
		pDestPixels += lockedRect.Pitch;
	}

	bitmap.UnlockBits(&bitmapData);
	pTexture->UnlockRect(0);

	return true;
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
	const auto& gdiplusManager = GdiplusManager::GetInstance();
	if (!gdiplusManager.IsInitialized()) {
		LOG_ERROR("GDI+ failed to initialize" << std::endl);
		return;
	}

	if (*ppTexture != nullptr) {
		ReleaseTexture(ppTexture);
	}

	using namespace Gdiplus;

	auto bmp = CreateGradientBitmap(in_width, in_height, colorSet, in_lineHeight, howManyLines);

	if (!bmp) {
		LOG_ERROR("Failed to create bitmap" << std::endl);
		return;
	}

	HRESULT hr_D3DX = D3DXCreateTexture(pDevice, in_width, in_height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture);

	if (*ppTexture == NULL) { // User is spam updating their INI through the GUI. D3DX textures are becoming NULL references.
		LOG_ERROR("User is spam updating their INI through the GUI. D3DXCreateTexture returned ");

		switch (hr_D3DX) {
			case (D3D_OK): 
				LOG_NOHEAD("D3D_OK" << std::endl);
				break;
			case (D3DERR_INVALIDCALL):
				LOG_NOHEAD("D3DERR_INVALIDCALL" << std::endl);
				break;
			case (D3DERR_NOTAVAILABLE):
				LOG_NOHEAD("D3DERR_NOTAVAILABLE" << std::endl);
				break;
			case (D3DERR_OUTOFVIDEOMEMORY):
				LOG_NOHEAD("D3DERR_OUTOFVIDEOMEMORY" << std::endl);
				break;
			case (E_OUTOFMEMORY):
				LOG_NOHEAD("E_OUTOFMEMORY" << std::endl);
				break;
			default: // Non-documented error
				LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
				break;
		}
		return;
	}

	if (!CopyBitmapToTexture(*ppTexture, *bmp, in_width, in_height)) {
		LOG_ERROR("Failed to copy bitmap to texture" << std::endl);
		ReleaseTexture(ppTexture);
		return;
	}

	//D3DXSaveTextureToFileA("generatedTexture_d3d.dds", D3DXIFF_DDS, (*ppTexture), 0);

	SetCustomColors();
}

void GenerateRandomTextures(IDirect3DDevice9* pDevice, bool solidColor) {
	for (int textIdx = 0; textIdx < randomTextureCount; ++textIdx) {
		ColorList colorSet;

		if (solidColor) {
			RSColor color = GenerateRandomColor();
			colorSet.assign(16, color);
		}
		else {
			colorSet.reserve(16);
			for (int i = 0; i < 16; ++i) {
				colorSet.push_back(GenerateRandomColor());
			}
		}

		D3D::GenerateTexture(pDevice, &randomTextures[textIdx], colorSet);
		randomTextureColors = colorSet;
	}
}

/// <summary>
/// Helper to clamp RGB components to [0, 1]
/// </summary>
void ClampColorComponents(RSColor& c) {
	auto clamp = [](float& value) {
		if (value > 1.0f) value = value - (value - 1.0f);
		if (value < 0.0f) value *= -1.0f;
		};

	clamp(c.r);
	clamp(c.g);
	clamp(c.b);
}

void GenerateRainbowTextures(IDirect3DDevice9* pDevice) {
	int currTexture = 0;
	constexpr float stringOffset = 20.0f;
	constexpr int stringCount = 6;

	for (float h = 0.0f; h < 360.0f; h += rainbowSpeed) {
		ColorList colorsRainbow;
		colorsRainbow.reserve(stringCount + 2);

		// Generate colors for each string
		for (int i = 0; i < stringCount; ++i) {
			RSColor c;
			c.setH(h + (stringOffset * i));
			ClampColorComponents(c);
			colorsRainbow.push_back(c);
		}

		// Add two extra colors
		colorsRainbow.insert(colorsRainbow.end(), 2, colorsRainbow.back());

		// Combine normal and colorblind colors, as both CB and regular colors will still look the same in rainbow mode
		ColorList colorSet;
		colorSet.reserve(colorsRainbow.size() * 2);
		colorSet.insert(colorSet.end(), colorsRainbow.begin(), colorsRainbow.end());
		colorSet.insert(colorSet.end(), colorsRainbow.begin(), colorsRainbow.end());

		D3D::GenerateTexture(pDevice, &rainbowTextures[currTexture++], colorSet);
	}
}

void GenerateColorTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, const ColorList& colorsNormal, const ColorList& colorsColorBlind) {
	ColorList colorSet;
	colorSet.reserve(colorsNormal.size() + colorsColorBlind.size());
	colorSet.insert(colorSet.end(), colorsNormal.begin(), colorsNormal.end());
	colorSet.insert(colorSet.end(), colorsColorBlind.begin(), colorsColorBlind.end());

	D3D::GenerateTexture(pDevice, ppTexture, colorSet);
}

void GenerateNotewayTexture(IDirect3DDevice9* pDevice) {
	ColorList colorSet = {
		Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayNumbered")),
		Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayUnNumbered"))
	};

	D3D::GenerateTexture(pDevice, &notewayTexture, colorSet, 256, 32, 16, 2);
}

void GenerateSingleColorTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, const std::string& colorKey, UINT width, UINT height, int lineHeight, int lines) {
	ColorList colorSet = { Settings::ConvertHexToColor(Settings::ReturnNotewayColor(colorKey)) };
	D3D::GenerateTexture(pDevice, ppTexture, colorSet, width, height, lineHeight, lines);
}

/// <summary>
/// Generate Texture Middleware.
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="type"> - What type of texture should be created? (enum D3D::TextureType)</param>
void D3D::GenerateTextures(IDirect3DDevice9* pDevice, TextureType type) {
	ColorList colorSet;

	switch (type) {
	case Random:
	case Random_Solid:
		GenerateRandomTextures(pDevice, type == Random_Solid);
		break;
	case Rainbow:
		GenerateRainbowTextures(pDevice);
		break;
	case Strings:
		GenerateColorTexture(pDevice, &customStringColorTexture, Settings::GetStringColors(false), Settings::GetStringColors(true));
		break;
	case Notes:
		GenerateColorTexture(pDevice, &customStringColorTexture, Settings::GetNoteColors(false), Settings::GetNoteColors(true));
		break;
	case Noteway:
		GenerateNotewayTexture(pDevice);
		break;
	case Gutter:
		GenerateSingleColorTexture(pDevice, &gutterTexture, Settings::ReturnNotewayColor("CustomHighwayGutter"), 256, 16, 16, 1);
		break;
	case FretNums:
		GenerateSingleColorTexture(pDevice, &fretNumTexture, Settings::ReturnNotewayColor("CustomFretNubmers"), 256, 16, 16, 1);
		break;
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
	if (FAILED(pDevice->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	// Get the color
	auto colour16 = static_cast<WORD>(((colour32 >> 28) & 0xF) << 12)
		| static_cast<WORD>(((colour32 >> 20) & 0xF) << 8)
		| static_cast<WORD>(((colour32 >> 12) & 0xF) << 4)
		| static_cast<WORD>(((colour32 >> 4) & 0xF) << 0);

	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	auto pDst16 = (WORD*)d3dlr.pBits;

	// Copy the color into the texture
	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = static_cast<WORD>(colour16);

	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}

bool CalculateCRCFromLockedTexture(IDirect3DTexture9* texture, const D3DLOCKED_RECT& lockedRect, DWORD& o_crc) {
	if (!lockedRect.pBits) {
		LOG_ERROR("CRCForTexture: lockedRect.pBits is null" << std::endl);
		return false;
	}

	auto* pData = static_cast<DWORD*>(lockedRect.pBits);
	o_crc = QuickCheckSum(pData, 20);
	texture->UnlockRect(0);

	return true;
}

void DebugCRCLocking(IDirect3DDevice9* pDevice) {
	D3DCAPS9 pDeviceCaps;
	IDirect3DSurface9* pRenderTarget;
	D3DSURFACE_DESC surfaceDesc;

	pDevice->GetDeviceCaps(&pDeviceCaps);

	for (int i = 0; i < pDeviceCaps.NumSimultaneousRTs - 1; i++) {
		LOG_INFO("CRCForTexture: Trying RenderTarget(" << i << ")" << std::endl);

		HRESULT rRenderTarget = pDevice->GetRenderTarget(i, &pRenderTarget);

		if (rRenderTarget == D3D_OK) {
			pRenderTarget->GetDesc(&surfaceDesc);
			std::string poolType;

			switch (surfaceDesc.Pool) {
			case D3DPOOL_DEFAULT:
				poolType = "D3DPOOL_DEFAULT";
				break;
			case D3DPOOL_MANAGED:
				poolType = "D3DPOOL_MANAGED";
				break;
			case D3DPOOL_SYSTEMMEM:
				poolType = "D3DPOOL_SYSTEMMEM";
				break;
			case D3DPOOL_SCRATCH:
				poolType = "D3DPOOL_SCRATCH";
				break;
			case D3DPOOL_FORCE_DWORD:
				poolType = "D3DPOOL_FORCE_DWORD";
				break;

			default:
				poolType = "UNKNOWN";
			}

			LOG_INFO("CRCForTexture: RenderTarget(" << i << ")->Pool == " << poolType << std::endl);
		}
		else if (rRenderTarget == D3DERR_NOTFOUND)
		{
			LOG_ERROR("CRCForTexture: No Render Target At Index: " << i << std::endl);
		}
		else
		{
			LOG_ERROR("CRCForTexure: pDevice->GetRenderTarget(" << i << ") has an invalid argument" << std::endl);
		}

		if (pRenderTarget != nullptr)
			pRenderTarget->Release();
	}
}

bool D3D::CRCForTexture(LPDIRECT3DTEXTURE9 texture, IDirect3DDevice9* pDevice, DWORD& o_crc) {
	if (!texture || !pDevice) {
		LOG_ERROR("CRCForTexture: Invalid parameters" << std::endl);
		return false;
	}

	D3DLOCKED_RECT lockedRect;
	HRESULT hr = texture->LockRect(0, &lockedRect, nullptr, D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY);

	if (SUCCEEDED(hr)) {
		return CalculateCRCFromLockedTexture(texture, lockedRect, o_crc);
	}
	else {
		LOG_ERROR("CRCForTexture: FAILED. LockRect == D3DERR_INVALIDCALL" << std::endl);

		DebugCRCLocking(pDevice);

		LOG_INFO("CRCForTexture: END" << std::endl);
		return false;
	}
}

void D3D::LoadTextures(IDirect3DDevice9* pDevice) {
	if (!pDevice) return;

	if (!nonexistentTexture) {
		D3DXCreateTextureFromFile(pDevice, L"nonexistenttexture.dds", &nonexistentTexture); // Black Notes
	}
}
