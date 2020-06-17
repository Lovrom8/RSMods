#pragma once
#include "windows.h"
#include "d3d9.h"
#include <vector>
#include <iostream>

HINSTANCE hWnd;
LPDIRECT3D9 pD3D;
LPDIRECT3DDEVICE9 pd3dDevice;

HWND hNewWnd = NULL;
WNDPROC oWndProc = NULL;

UINT mStartregister;
UINT mVectorCount;

UINT Stride;
D3DVERTEXBUFFER_DESC vdesc;

IDirect3DVertexDeclaration9* pDecl;
D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
UINT numElements;

IDirect3DVertexShader9* vShader;
UINT vSize;

IDirect3DPixelShader9* pShader;
UINT pSize;

IDirect3DTexture9* texture;
D3DSURFACE_DESC sDesc;
DWORD qCRC;
D3DLOCKED_RECT pLockedRect;

LPDIRECT3DVERTEXBUFFER9 Stream_Data;
UINT Offset = 0;


LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow;
LPDIRECT3DTEXTURE9 gradientTextureNormal, gradientTextureSeven, nonexistentTexture, additiveNoteTexture;

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

bool WorldToScreen(LPDIRECT3DDEVICE9 pDevice, D3DXVECTOR3* pos, D3DXVECTOR3* out) {
	D3DVIEWPORT9 viewPort;
	D3DXMATRIX view, projection, world;

	pDevice->GetViewport(&viewPort);
	pDevice->GetTransform(D3DTS_VIEW, &view);
	pDevice->GetTransform(D3DTS_PROJECTION, &projection);
	D3DXMatrixIdentity(&world);

	D3DXVec3Project(out, pos, &viewPort, &projection, &view, &world);
	if (out->z < 1) {
		return true;
	}
	return false;
}

struct Mesh {
	UINT Stride;
	UINT PrimCount;
	UINT NumVertices;

	Mesh(UINT s, UINT p, UINT n) {
		Stride = s;
		PrimCount = p;
		NumVertices = n;
	}
};

bool IsToBeRemoved(std::vector<Mesh> list, Mesh mesh) {
	for (auto currentMesh : list) {
		if (currentMesh.Stride == mesh.Stride && currentMesh.PrimCount == mesh.PrimCount && currentMesh.NumVertices == mesh.NumVertices)
			return true;
	}

	return false;
}

std::vector<Mesh> skyline{ {16, 66, 132}, {16, 38, 76}, {16, 2, 4}, {16, 104, 208}};
std::vector<Mesh> skylineLesson{ {16, 66, 132}, {16, 38, 76}, {16, 2, 4}, {16, 104, 208}, {20, 2, 4}, {16, 18, 16}, {8, 20, 16} };
//, {8, 10, 8}, removes a bit too much

#define SKYLINE1 (Stride == 16 && primCount == 66 && NumVertices == 132) 
#define SKYLINE2 (Stride == 16 && primCount == 38 && NumVertices == 76)
#define SKYLINE3 (Stride == 16 && primCount == 2 && NumVertices == 4)
#define SKYLINE4 (Stride == 16 && primCount == 104 && NumVertices == 208)

#define HIGHLIGHTED_NOTE_HEAD (Stride == 32 && primCount == 104 && NumVertices == 187) 
#define NOTE_HEADS (Stride == 36 && primCount == 40 && NumVertices == 76)
#define OPEN_STRINGS (Stride == 36 && primCount == 40 && NumVertices == 50)
#define INDICATORS (Stride == 32 && primCount == 8 && NumVertices == 10)
#define NOTE_HEAD_SYMBOLS (Stride == 36 && primCount == 100 && NumVertices == 200)
#define FRETNUM_AND_MISS_INDICATOR (Stride == 32 && primCount == 2 && NumVertices == 4)
#define SOME_NOTE_HEAD_STUFF (Stride == 34 && primCount == 40 && NumVertices == 76)
#define CHORD_OUTLINE_THINGY (Stride == 34 && primCount == 48 && NumVertices == 80)
#define CHORD_OUTLINE_CORNERS (Stride == 24 && primCount == 108 && NumVertices == 208)
#define PAUSE_MENU_BGD (Stride == 16 && primCount == 18 && NumVertices == 16)
#define BUTTONS_BOTTOM_1 (Stride == 8 && primCount == 20 && NumVertices == 16)
#define VERTICAL_LINES (Stride == 32 && primCount == 6 && NumVertices == 8)
#define CHORD_NAMES (Stride == 24 && primCount == 2 && NumVertices == 4)
#define FHP (Stride == 12 && primCount == 6 && NumVertices == 8)
#define SLIDERS_AND_BUTTONS (Stride == 8 && primCount == 20 && NumVertices == 16)

std::vector<Mesh> headstock{ { 32, 2, 4 }, { 8, 10, 8 }, { 24, 2, 4 }, { 60, 588, 316 }, { 76, 2311, 1268 }, { 76, 4062, 3165 }, { 60, 392, 198 }, { 60, 68, 52 }, { 44, 538, 311 }, { 68, 4123, 2983 }, { 76, 3284, 1787 } };

#define HEADSTOCK_STUFF (Stride == 32 && primCount == 2 && NumVertices == 4)
#define HEADSTOCKS (Stride == 8 && primCount == 10 && NumVertices == 8)
#define HEADSTUCK (Stride == 24 && primCount == 2 && NumVertices == 4)
#define HEADSTOCK_TEXTU1 (Stride == 60 && primCount == 588 && NumVertices == 316)
#define HEADSTOCK_TEXTU2 (Stride == 76 && primCount == 2311 && NumVertices == 1268)
#define HEADSTOCK_TEXTU3 (Stride == 76 && primCount == 4062 && NumVertices == 3165)
#define HEADSTOCK_1 (Stride == 60 && primCount == 392 && NumVertices == 198)
#define HEADSTOCK_2 (Stride == 60 && primCount == 68 && NumVertices == 52)
#define HEADSTOCK_3 (Stride == 44 && primCount == 538 && NumVertices == 311)
#define HEADSTOCK_4 (Stride == 68 && primCount == 4123 && NumVertices == 2983)
#define HEADSTOCK_5 (Stride == 76 && primCount == 3284 && NumVertices == 1787)

std::vector<Mesh> greenscreenwall{ {92, 2, 6} };

#define GREENSCREEN_WALL (Stride == 92 && primCount == 2 && NumVertices == 6)

