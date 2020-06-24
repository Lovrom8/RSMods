#pragma once
#include "windows.h"
#include "d3d9.h"
#include <vector>
#include <iostream>

HWND hThisWnd = NULL;
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

int currIdx = 0, selectedIdx = 0, counter = 0;
INT currStride, currNumVertices, currPrimCount, currStartIndex, currStartRegister, currPrimType, currDeclType, currVectorCount, currNumElements;
bool cbEnabled;

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

	bool operator ==(const Mesh& meshB) const {
		return (Stride == meshB.Stride
			&& PrimCount == meshB.PrimCount
			&& NumVertices == meshB.NumVertices);
	}
};

struct ThiccMesh {
	UINT Stride;
	UINT PrimCount;
	UINT NumVertices;
	UINT startIndex;
	UINT mStartRegister;
	UINT PrimType;
	UINT DeclType;
	UINT mVectorCount;
	UINT NumElements;

	ThiccMesh(UINT s, UINT p, UINT n, UINT si, UINT mSR, UINT pt, UINT dt, UINT mvc, UINT nm) {
		Stride = s;
		PrimCount = p;
		NumVertices = n;
		startIndex = si;
		mStartRegister = mSR;
		PrimType = pt;
		DeclType = dt;
		mVectorCount = mvc;
		NumElements = nm;
	}

	std::string ToString() {
		std::string ret = "{ ";

		ret += std::to_string(Stride);
		ret += ",";
		ret += std::to_string(PrimCount);
		ret += ",";
		ret += std::to_string(NumVertices);
		ret += ",";
		ret += std::to_string(startIndex);
		ret += ",";
		ret += std::to_string(mStartRegister);
		ret += ",";
		ret += std::to_string(PrimType);
		ret += ",";
		ret += std::to_string(DeclType);
		ret += ",";
		ret += std::to_string(mVectorCount);
		ret += ",";
		ret += std::to_string(NumElements);

		ret += " },";

		return ret;
	}

	bool operator ==(const ThiccMesh& meshB) const {
		return (Stride == meshB.Stride
			&& PrimCount == meshB.PrimCount
			&& NumVertices == meshB.NumVertices
			&& startIndex == meshB.startIndex
			&& mStartRegister == meshB.mStartRegister
			&& PrimType == meshB.PrimType
			&& DeclType == meshB.DeclType
			&& mVectorCount == meshB.mVectorCount
			&& NumElements == meshB.NumElements);
	}
};

bool IsExtraRemoved(std::vector<ThiccMesh> list, ThiccMesh mesh) {
	for (auto currentMesh : list) 
		if(currentMesh == mesh)
			return true;

	return false;
}

bool IsToBeRemoved(std::vector<Mesh> list, Mesh mesh) {
	for (auto currentMesh : list) 
		if(currentMesh == mesh)
			return true;

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


std::vector<ThiccMesh> headstockThicc{
	// General
		// { 24, 2, 4, 0, 0, 4, 2, 8, 4 }, Tuner (Disabled to make it easier on the end user on tuning)
		//{ 32, 2, 4, 0, 0, 4, 2, 12, 4 }, // Check tuningLetters
		{ 32, 2, 4, 0, 0, 4, 2, 4, 2 },
		
	// Guitar
		// 3+3
			{ 60, 392, 198, 0, 0, 4, 2, 9, 7 }, { 60, 392, 198, 0, 0, 4, 2, 4, 2 }, // Machine Heads (2nd vector is Highlight)
			{ 60, 68, 52, 0, 0, 4, 2, 9, 7 }, { 60, 68, 52, 0, 0, 4, 2, 4, 2 }, // Shaft of Machine Heads (2nd vector is Highlight)
			{ 44, 538, 311, 0, 0, 4, 2, 8, 3 }, { 44, 538, 311, 0, 0, 4, 2, 9, 4 }, { 44, 538, 406, 0, 0, 4, 2, 9, 4}, // Strings Past Nut (3rd vector is for Leftires)
			{ 76, 3284, 1787, 0, 0, 4, 2, 8, 3 }, { 76, 3284, 1787, 0, 0, 4, 2, 9, 7 }, // Headstock Texture
			{ 68, 2760, 1890, 0, 0, 4, 2, 8, 3 }, { 68, 4123, 2983, 0, 0, 4, 2, 8, 3 }, { 68, 2760, 1890, 0, 0, 4, 2, 9, 7 },  { 68, 4123, 2983, 0, 0, 4, 2, 9, 7 },  // Tuning Peg (2nd vector is the block on the back of the peg, 3rd and 4th for Lefties)

		// 6-inline
			{ 60, 588, 316, 0, 0, 4, 2, 9, 7 }, { 60, 588, 316, 0, 0, 4, 2, 4, 2 }, // Machine Heads (2nd vector is Highlight)
			{ 76, 67, 76, 0, 0, 4, 2, 9, 7 }, { 76, 4062, 3165, 0, 0, 4, 2, 9, 7 },  // Truss Rod Cover (2nd vector is the Screws)
			{ 44, 1074, 618, 0, 0, 4, 2, 8, 3 }, { 44, 1074, 618, 0, 0, 4, 2, 9, 4 }, { 44, 538, 416, 0, 0, 4, 2, 8, 3 }, { 44, 538, 416, 0, 0, 4, 2, 9, 4 }, // Strings Past Nut (3rd and 4th vectors are for Lefties)
			{ 76, 2311, 1268, 0, 0, 4, 2, 9, 7 }, // Headstock Texture
			{ 76, 2142, 1458, 0, 0, 4, 2, 8, 3 }, { 76, 2142, 1458, 0, 0, 4, 2, 9, 7 }, // Tuning Peg

	// Bass
		// 2+2
			{ 60, 992, 604, 0, 0, 4, 2, 9, 7 }, { 60, 992, 604, 0, 0, 4, 2, 4, 2 }, // Machine Heads (2nd vector is Highlight)
			{ 44, 522, 303, 0, 0, 4, 2, 9, 4 }, { 44, 522, 303, 0, 0, 4, 2, 8, 3 },  // Strings Past Nut (D, & G are this texture. E & A are {44, 538, 311}, which is applied in 3+3 guitar)
			{ 84, 1232, 699, 0, 0, 4, 2, 9, 7 },  { 84, 1232, 699, 0, 0, 4, 2, 8, 3 }, // Headstock Texture
			{ 76, 2560, 2140, 0, 0, 4, 2, 9, 7 }, { 76, 2560, 2842, 0, 0, 4, 2, 9, 7 },{ 76, 6304, 4880, 0, 0, 4, 2, 9, 7 }, { 76, 2560, 2842, 0, 0, 4, 2, 9, 7 }, // Tuning Peg (2nd vector is the block on the back of the peg, 3rd and 4th for Lefties)

		// 4-inline
			{ 60, 306, 218, 0, 0, 4, 2, 9, 7 }, { 60, 306, 218, 0, 0, 4, 2, 4, 2 }, // Machine Heads (2nd vector is Highlight)
			{ 84, 237, 150, 0, 0, 4, 2, 9, 7 }, { 68, 237, 150, 0, 0, 4, 2, 9, 7 },  // Truss Rod Cover (2nd vector is for Lefties)
			{ 44, 538, 522, 0, 0, 4, 2, 8, 3 }, { 44, 538, 522, 0, 0, 4, 2, 9, 4 }, // Strings Past Nut
			{ 84, 1067, 869, 0, 0, 4, 2, 9, 7 }, { 68, 1067, 612, 0, 0, 4, 2, 9, 7}, // Headstock Texture (2nd vector is for Lefties)
			{ 68, 1340, 1120, 0, 0, 4, 2, 9, 7 }, { 68, 2216, 1852, 0, 0, 4, 2, 9, 7 }, { 56, 1340, 1120, 0, 0, 4, 2, 10, 6}, { 56, 2216, 1854, 0, 0, 4, 2, 10, 6}, // Tuning Peg (2nd vector is the block on the back of the peg, 3rd and 4th for Lefties)
};

// All the single ladies
std::vector<ThiccMesh> laneMarkers{ { 32, 6, 8, 0, 0, 4, 2, 4, 7 } };
std::vector<ThiccMesh> fretless{ { 56, 264, 273, 0, 0, 4, 2, 13, 6 } };
std::vector<ThiccMesh> inlays{ { 32, 8, 9, 0, 0, 4, 2, 12, 4 } };
std::vector<ThiccMesh> tuningLetters{ { 32, 2, 4, 0, 0, 4, 2, 12, 4 } };
std::vector<ThiccMesh> greenscreenwall{ { 92, 2, 6, 0, 0, 4, 2, 8, 9 } };
std::vector<ThiccMesh> nostrings{ { 12, 1536, 1199, 0, 0, 4, 2, 9, 4 } };
std::vector<ThiccMesh> drunkMode{ { 12, 2, 4, 0, 0, 4, 2, 12, 2 } };

std::vector<ThiccMesh> allMeshes;
std::vector<ThiccMesh> removedMeshes;

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

std::string getRidOfTuningLettersOnTheseMenus[16] = {
	(std::string)"SelectionListDialog",
	(std::string)"LearnASong_PreSongTuner",
	(std::string)"LearnASong_PreSongTunerMP",
	(std::string)"LearnASong_SongOptions",
	(std::string)"NonStopPlay_PreSongTuner",
	(std::string)"NonStopPlay_PreSongTunerMP",
	(std::string)"ScoreAttack_PreSongTuner",
	(std::string)"SessionMode_PreSMTunerMP",
	(std::string)"SessionMode_PreSMTuner",
	(std::string)"UIMenu_Tuner",
	(std::string)"UIMenu_TunerMP",
	(std::string)"Guitarcade_Tuner",
	(std::string)"GETuner",
	(std::string)"Tuner",
	(std::string)"Duet_PreSongTuner",
	(std::string)"H2H_PreSongTuner"
};

std::string songModes[6] = {
	(std::string)"LearnASong_Game",
	(std::string)"NonStopPlay_Game",
	(std::string)"ScoreAttack_Game",
	(std::string)"LearnASong_Pause",
	(std::string)"NonStopPlay_Pause",
	(std::string)"ScoreAttack_Pause"
};