#include "../stdafx.h"
#include "D3D.hpp"
#include <gdiplus.h>

#pragma comment (lib, "gdiplus.lib")

void D3D::ReleaseTexture(IDirect3DTexture9** ppTexture) {
	if (ppTexture && *ppTexture) {
		(*ppTexture)->Release();
		*ppTexture = nullptr;
	}
}

namespace {
	std::unique_ptr<Gdiplus::Bitmap> CreateGradientBitmap(UINT width, UINT height, const ColorList& colorSet, int lineHeight, int howManyLines) {
		using namespace Gdiplus;

		auto bmp = std::make_unique<Bitmap>(width, height, PixelFormat32bppARGB);
		Graphics graphics(bmp.get());

		std::array<REAL, 3> blendPositions = { 0.0f, 0.4f, 1.0f };

		for (int i = 0; i < howManyLines; i++) {
			RSColor currColor = colorSet[i];
			Color middleColor(static_cast<byte>(currColor.r * 255), static_cast<byte>(currColor.g * 255), static_cast<byte>(currColor.b * 255));
			std::array<Color, 3> gradientColors = { Color::Black, middleColor, Color::White };

			LinearGradientBrush linGrBrush(
				Point(0, 0),
				Point(width, lineHeight),
				Color::Black,
				Color::White
			);

			LinearGradientBrush whiteCoverupBrush(
				Point(width - 3, lineHeight * 5),
				Point(width, height),
				Color::White,
				Color::White
			);

			linGrBrush.SetInterpolationColors(gradientColors.data(), blendPositions.data(), gradientColors.size());
			graphics.FillRectangle(&linGrBrush, 0, i * lineHeight, width, lineHeight);
			graphics.FillRectangle(&whiteCoverupBrush, width - 3, i * lineHeight, width, lineHeight);
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
}

/// <summary>
/// Generate a gradient texture with the specified color set.
/// </summary>
void D3D::GenerateGradientTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, const ColorList& colorSet, UINT in_width, UINT in_height, int in_lineHeight, int howManyLines) {
	const auto& gdiplusManager = GdiplusManager::GetInstance();
	if (!gdiplusManager.IsInitialized()) {
		LOG_ERROR("GDI+ failed to initialize" << std::endl);
		return;
	}

	if (!ppTexture || !pDevice) {
		return;
	}

	if (*ppTexture != nullptr) {
		ReleaseTexture(ppTexture);
	}

	auto bmp = CreateGradientBitmap(in_width, in_height, colorSet, in_lineHeight, howManyLines);

	if (!bmp) {
		LOG_ERROR("Failed to create bitmap" << std::endl);
		return;
	}

	HRESULT hr_D3DX = D3DXCreateTexture(pDevice, in_width, in_height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, ppTexture);

	if (*ppTexture == nullptr) {
		LOG_ERROR("D3DXCreateTexture returned " << hr_D3DX << std::endl);
		return;
	}

	if (!CopyBitmapToTexture(*ppTexture, *bmp, in_width, in_height)) {
		LOG_ERROR("Failed to copy bitmap to texture" << std::endl);
		ReleaseTexture(ppTexture);
		return;
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

namespace {
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
