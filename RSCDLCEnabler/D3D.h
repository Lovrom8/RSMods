#pragma once
#include "windows.h"
#include "d3d9.h"

HRESULT GenerateTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppD3Dtex, DWORD colour32)
{
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

#define SKYLINE1 (Stride == 16 && primCount == 66 && NumVertices == 132) 
#define SKYLINE2 (Stride == 16 && primCount == 38 && NumVertices == 76)
#define SKYLINE3 (Stride == 16 && primCount == 2 && NumVertices == 4)
#define SKYLINE4 (Stride == 16 && primCount == 104 && NumVertices == 208)

#define HIGHLIGHTED_NOTE_HEAD (Stride == 32 && primCount == 104 && NumVertices == 187) 
#define HIGHLIGHTED_NOTE_HEAD2 (Stride == 32 && primCount == 104 && NumVertices == 187)
#define NOTE_HEADS (Stride == 36 && primCount == 40 && NumVertices == 76)
#define OPEN_STRINGS (Stride == 36 && primCount == 40 && NumVertices == 50)
#define INDICATORS (Stride == 32 && primCount == 8 && NumVertices == 10)
#define NOTE_HEAD_SYMBOLS (Stride == 36 && primCount == 100 && NumVertices == 200)
#define FRETNUM_AND_MISS_INDICATOR (Stride == 32 && primCount == 2 && NumVertices == 4)


UINT mStartregister;
UINT mVectorCount;

LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow;
LPDIRECT3DTEXTURE9 gradientTextureNormal, gradientTextureSeven;