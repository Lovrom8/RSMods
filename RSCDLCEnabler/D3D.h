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

//  {8, 10, 8}, //  Massive UI (Artwork, preview skylines, loading screens, etc)

std::vector<Mesh> skyline{ 
	{16, 66, 132}, //
	{16, 38, 76}, // 
	{16, 2, 4}, //
	{16, 104, 208}, //
};
std::vector<Mesh> skylineLesson{
	{16, 66, 132}, //
	{16, 38, 76}, //
	{16, 2, 4}, // 
	{16, 104, 208}, // 
	{20, 2, 4}, // 
	{16, 18, 16}, // 
	{8, 20, 16} // 
};

std::vector<Mesh> sevenstring{
	{36, 40, 76}, // Note Heads
	{36, 40, 50}, // Open Strings
	{32, 8, 10}, // Indicators
	{36, 100, 200}, // Note Head Symbols
	{32, 104, 187}, // Highlighted Note Head
	{34, 40, 76} // "Some Note Head Stuff" - Lovro
};

std::vector<Mesh> headstock{
	// 3+3 Guitar
		//{ 32, 2, 4 }, // Letters above Pegs in tuning, ingame Fret #'s, Pg Down key in pause menu, Note Highway, Fingering of chords | Needs to be specified more
		{ 24, 2, 4 }, // Tuners, Tuning circle, Keys in pause menu
		{ 60, 392, 198 }, // Tuning Pegs
		{ 60, 68, 52 }, // Stick things coming off pegs to connect to the headstock
		{ 44, 538, 311 }, // Strings past nut
		{ 68, 4123, 2983 }, // Tuner Blocks
		{ 76, 3284, 1787 }, // Headstock texture
		{ 68, 2760, 1890} // Tuner

};


// All the single ladies
std::vector<Mesh> fretless{ {56, 264, 273} };
std::vector<Mesh> inlays{ {32, 8, 9} };
std::vector<Mesh> greenscreenwall{{92, 2, 6}};

// Misc
#define FRETNUM_AND_MISS_INDICATOR (Stride == 32 && primCount == 2 && NumVertices == 4)
#define CHORD_OUTLINE_THINGY (Stride == 34 && primCount == 48 && NumVertices == 80)
#define CHORD_OUTLINE_CORNERS (Stride == 24 && primCount == 108 && NumVertices == 208)
#define PAUSE_MENU_BGD (Stride == 16 && primCount == 18 && NumVertices == 16)
#define BUTTONS_BOTTOM_1 (Stride == 8 && primCount == 20 && NumVertices == 16)
#define VERTICAL_LINES (Stride == 32 && primCount == 6 && NumVertices == 8)
#define CHORD_NAMES (Stride == 24 && primCount == 2 && NumVertices == 4)
#define FHP (Stride == 12 && primCount == 6 && NumVertices == 8)
#define SLIDERS_AND_BUTTONS (Stride == 8 && primCount == 20 && NumVertices == 16)