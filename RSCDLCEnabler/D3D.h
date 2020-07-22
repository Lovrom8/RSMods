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
D3DLOCKED_RECT pLockedRect;

LPDIRECT3DVERTEXBUFFER9 Stream_Data;
UINT Offset = 0;

LPDIRECT3DBASETEXTURE9 pBaseTexture;
UINT vertexBufferSize;
unsigned long crc;

LPDIRECT3DTEXTURE9 pCurrTexture;

LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow;
LPDIRECT3DTEXTURE9 gradientTextureNormal, gradientTextureSeven, nonexistentTexture, additiveNoteTexture, ourTexture;

int currIdx = 0, selectedIdx = 0, counter = 0;
INT currStride, currNumVertices, currPrimCount, currStartIndex, currStartRegister, currPrimType, currDeclType, currVectorCount, currNumElements;
bool cbEnabled, generateTexture = false;
const char* comboStringsItems[] = { "0", "1", "2", "3", "4", "5" };
static int selectedString = 0;
static int strR, strG, strB;

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
		if (currentMesh == mesh)
			return true;

	return false;
}

bool IsToBeRemoved(std::vector<Mesh> list, Mesh mesh) {
	for (auto currentMesh : list)
		if (currentMesh == mesh)
			return true;

	return false;
}

//  {8, 10, 8}, //  Massive UI (Artwork, preview skylines, loading screens, etc)

//std::vector<Mesh> skyline{
//	{16, 66, 132}, //
//	{16, 38, 76}, // 
//	{16, 2, 4}, //
//	{16, 104, 208}, //
//};

std::vector<Mesh> skylineLesson{
	{16, 66, 132},
	{16, 38, 76},
	{16, 2, 4},
	{16, 104, 208},
	{20, 2, 4},
	{16, 18, 16},
	{8, 20, 16} 
};

std::vector<Mesh> sevenstring{
	{36, 40, 76}, // Note Heads
	{36, 40, 50}, // Open Strings
	{32, 8, 10}, // Indicators
	{36, 100, 200}, // Note Head Symbols
	{32, 104, 187}, // Highlighted Note Head
	{34, 40, 76} // "Some Note Head Stuff" - Lovro
};

std::vector<Mesh> noteHighway{ { 32, 2, 4} };

std::vector<ThiccMesh> headstockThicc{
	// General
		// { 24, 2, 4, 0, 0, 4, 2, 8, 4 }, Tuner (Disabled to make it easier on the end user on tuning)
		//{ 32, 2, 4, 0, 0, 4, 2, 12, 4 }, // Check tuningLetters
		{ 32, 2, 4, 0, 0, 4, 2, 4, 2 },

		// Guitar
			// 3+3
				{ 60, 392, 198, 0, 0, 4, 2, 9, 7 }, // Machine Heads
				{ 60, 68, 52, 0, 0, 4, 2, 9, 7 }, // Shaft of Machine Heads
				{ 44, 538, 311, 0, 0, 4, 2, 8, 3 }, { 44, 538, 311, 0, 0, 4, 2, 9, 4 }, { 44, 538, 406, 0, 0, 4, 2, 9, 4}, // Strings Past Nut (3rd vector is for Lefties)
				{ 76, 3284, 1787, 0, 0, 4, 2, 8, 3 }, { 76, 3284, 1787, 0, 0, 4, 2, 9, 7 }, // Headstock Texture
				{ 68, 2760, 1890, 0, 0, 4, 2, 8, 3 }, { 68, 4123, 2983, 0, 0, 4, 2, 8, 3 }, { 68, 2760, 1890, 0, 0, 4, 2, 9, 7 },  { 68, 4123, 2983, 0, 0, 4, 2, 9, 7 },  // Tuning Peg (2nd vector is the block on the back of the peg, 3rd and 4th for Lefties)

			// 6-inline
				{ 60, 588, 316, 0, 0, 4, 2, 9, 7 }, // Machine Heads
				{ 76, 67, 76, 0, 0, 4, 2, 9, 7 }, { 76, 4062, 3165, 0, 0, 4, 2, 9, 7 },  // Truss Rod Cover (2nd vector is the Screws)
				{ 44, 1074, 618, 0, 0, 4, 2, 8, 3 }, { 44, 1074, 618, 0, 0, 4, 2, 9, 4 }, { 44, 538, 416, 0, 0, 4, 2, 8, 3 }, { 44, 538, 416, 0, 0, 4, 2, 9, 4 }, // Strings Past Nut (3rd and 4th vectors are for Lefties)
				{ 76, 2311, 1268, 0, 0, 4, 2, 9, 7 }, // Headstock Texture
				{ 76, 2142, 1458, 0, 0, 4, 2, 8, 3 }, { 76, 2142, 1458, 0, 0, 4, 2, 9, 7 }, // Tuning Peg

		// Bass
			// 2+2
				{ 60, 992, 604, 0, 0, 4, 2, 9, 7 }, // Machine Heads
				{ 44, 522, 303, 0, 0, 4, 2, 9, 4 }, { 44, 522, 303, 0, 0, 4, 2, 8, 3 },  // Strings Past Nut (D, & G are this texture. E & A are {44, 538, 311}, which is applied in 3+3 guitar)
				{ 84, 1232, 699, 0, 0, 4, 2, 9, 7 },  { 84, 1232, 699, 0, 0, 4, 2, 8, 3 }, // Headstock Texture
				{ 76, 2560, 2140, 0, 0, 4, 2, 9, 7 }, { 76, 2560, 2842, 0, 0, 4, 2, 9, 7 },{ 76, 6304, 4880, 0, 0, 4, 2, 9, 7 }, { 76, 2560, 2842, 0, 0, 4, 2, 9, 7 }, // Tuning Peg (2nd vector is the block on the back of the peg, 3rd and 4th for Lefties)

			// 4-inline
				{ 60, 306, 218, 0, 0, 4, 2, 9, 7 }, // Machine Heads
				{ 84, 237, 150, 0, 0, 4, 2, 9, 7 }, { 68, 237, 150, 0, 0, 4, 2, 9, 7 },  // Truss Rod Cover (2nd vector is for Lefties)
				{ 44, 538, 522, 0, 0, 4, 2, 8, 3 }, { 44, 538, 522, 0, 0, 4, 2, 9, 4 }, // Strings Past Nut
				{ 84, 1067, 869, 0, 0, 4, 2, 9, 7 }, { 68, 1067, 612, 0, 0, 4, 2, 9, 7}, // Headstock Texture (2nd vector is for Lefties)
				{ 68, 1340, 1120, 0, 0, 4, 2, 9, 7 }, { 68, 2216, 1852, 0, 0, 4, 2, 9, 7 }, { 56, 1340, 1120, 0, 0, 4, 2, 10, 6}, { 56, 2216, 1854, 0, 0, 4, 2, 10, 6}, // Tuning Peg (2nd vector is the block on the back of the peg, 3rd and 4th for Lefties)
};
std::vector<ThiccMesh> inlays{
	{ 32, 8, 9, 0, 0, 4, 2, 12, 4 }, { 32, 16, 18, 0, 0, 4, 2, 12, 4 }, // Standard Dot (2nd for 12th/24th fret)
};

std::vector<ThiccMesh> tunerHighlight{
	{ 60, 392, 198, 0, 0, 4, 2, 4, 2 }, { 60, 68, 52, 0, 0, 4, 2, 4, 2 }, // 3+3
	{ 60, 588, 316, 0, 0, 4, 2, 4, 2 }, // 6-inline
	{ 60, 992, 604, 0, 0, 4, 2, 4, 2 }, // 2+2
	{ 60, 306, 218, 0, 0, 4, 2, 4, 2 }, // 4-inline
};

std::vector<ThiccMesh> leftyFix{
	{ 60, 392, 198, 0, 0, 4, 2, 9, 7 }, // Machine Heads 3+3
	{ 44, 538, 406, 0, 0, 4, 2, 9, 4}, // Strings past tuner 3+3
};

// All the single ladies
std::vector<ThiccMesh> laneMarkers{ { 32, 6, 8, 0, 0, 4, 2, 4, 7 } };
std::vector<ThiccMesh> fretless{ { 56, 264, 273, 0, 0, 4, 2, 13, 6 } };
std::vector<ThiccMesh> tuningLetters{ { 32, 2, 4, 0, 0, 4, 2, 12, 4 } };
std::vector<ThiccMesh> greenScreenWallMesh{ { 92, 2, 6, 0, 0, 4, 2, 8, 9 } };
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

std::string tuningMenus[17] = { // previously known as getRidOfTuningLettersOnTheseMenus
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
	(std::string)"Tuner",
	(std::string)"Duet_PreSongTuner",
	(std::string)"H2H_PreSongTuner",
	(std::string)"GETuner",
	(std::string)"PreGame_GETuner"
};

std::string songModes[8] = {
	(std::string)"LearnASong_Game",
	(std::string)"NonStopPlay_Game",
	(std::string)"ScoreAttack_Game",
	(std::string)"LearnASong_Pause",
	(std::string)"NonStopPlay_Pause",
	(std::string)"ScoreAttack_Pause",
};

std::string lessonModes[2] = {
	(std::string)"GuidedExperience_Game",
	(std::string)"GuidedExperience_Pause"
};

std::string dontAutoEnter[16] = {
	// First time Player
	(std::string)"TextEntryDialog", // Prompts to enter profile name / uplay name / etc.
	(std::string)"PlayedRS1Select", // Did you play the original Rocksmith?
	(std::string)"ExperienceSelect", // How good at guitar do you think you are?
	(std::string)"PathSelect", // What path do you want to play? (Lead, Rhythm, Bass)
	(std::string)"HandSelect", // Are you left handed or right handed? (Left, Right)
	(std::string)"HeadstockSelect", // Select between (3+3 or 6-inline) (2+2 or 4-inline).
	(std::string)"FE_InputSelect", // What cable method do you have? (RTC, Mic, Disconnected)
	(std::string)"FECalibrationMeter", // First time calibrating.
	(std::string)"VideoPlayer", // Intro videos on how to calibrate, tune, play the game.
	(std::string)"FETuner", // First time tuning.
	(std::string)"FirstEncounter_Game", // Intro to game, teaches basic UI elements.
	(std::string)"SelectionListDialog", // Occasional Yes/ No Prompts.

	// Played RS1 before
	(std::string)"RefresherSelect", // User has played Rocksmith before, but do they need some touchup on their knowledge.

	// Disconnected Mode
	(std::string)"ImageDialog", // Disconnected Mode Pictures.

	// Uplay section
	(std::string)"UplayLoginDialog", // User needs to login to uPlay.
	(std::string)"UplayAccountCreationDialog" // User needs to create a uPlay account.
};

/*------------------------ CRC Calculation --------------------------------------- */
std::vector<LPDIRECT3DTEXTURE9> headstockTexturePointers;
std::vector<LPDIRECT3DTEXTURE9> skylineTexturePointers;
std::vector<LPDIRECT3DTEXTURE9> notewayTexturePointers; //stems & accents

LPDIRECT3DBASETEXTURE9 pBaseTextures[3];
LPDIRECT3DTEXTURE9 pCurrTextures[3];

void AddToTextureList(std::vector<LPDIRECT3DTEXTURE9>& textureList, LPDIRECT3DTEXTURE9 newTexture) {
	if (std::find(std::begin(textureList), std::end(textureList), newTexture) == std::end(textureList))
		textureList.push_back(newTexture);
}

DWORD QuickCheckSum(DWORD* BufferData, size_t Size)
{
	if (!BufferData) return 0x0;
	DWORD Temp = 0, Sum = *BufferData;

	for (size_t I = 1; I < (Size / 4); ++I)
	{
		Temp = BufferData[I];
		Temp = (DWORD)(Sum >> 29) + Temp;
		Temp = (DWORD)(Sum >> 17) + Temp;
		Sum = (DWORD)(Sum << 3) ^ Temp;
	}
	return Sum;
}

bool CRCForTexture(LPDIRECT3DTEXTURE9 texture, DWORD& crc) {
	D3DLOCKED_RECT lockedRect;

	if (texture->LockRect(0, &lockedRect, NULL, D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY) == D3D_OK) {
		crc = 0;
		DWORD* pData = (DWORD*)lockedRect.pBits;
		if (pData != NULL) {
			crc = QuickCheckSum(pData, 20);

			texture->UnlockRect(0);
			return true;
		}
	}
	return false;
}

/*-------------------------------------------------------------------------*/