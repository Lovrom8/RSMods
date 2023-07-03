#pragma once
#include "windows.h"
#include "d3d9.h"
#include <vector>
#include <iostream>
#include <random>
#include <string>
#include <gdiplus.h>

#include "../Log.hpp"
#include "../Structs.hpp"
#include "D3DHelper.hpp"

// #pragma intrinsic(_ReturnAddress) Not actually declared
#pragma comment (lib, "gdiplus.lib")
#pragma once

namespace D3D {
	enum TextureType {
		Random = 0,
		Random_Solid = 1,
		Rainbow = 2,
		Strings = 3,
		Notes = 4,
		Noteway = 5,
		Gutter = 6,
		FretNums = 7,
	};

	ColorMap GetCustomColors(int strIdx, bool CB);
	void SetCustomColors();
	HRESULT GenerateSolidTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppD3Dtex, DWORD colour32);
	void GenerateTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, ColorList colorSet, UINT in_width = 256, UINT in_height = 128, int in_lineHeight = 8, int howManyLines = 16);
	void GenerateTextures(IDirect3DDevice9* pDevice, TextureType type);
};

inline Gdiplus::GdiplusStartupInput inp;
inline Gdiplus::GdiplusStartupOutput outp;
inline ULONG_PTR token_;

inline static std::random_device rd;
inline static std::mt19937 rng(rd());

inline LPDIRECT3DBASETEXTURE9 pBaseTexture, pBaseRainbowTexture, pBaseNotewayTexture, pBaseGutterTexture, pBaseFretNumTexture;

inline unsigned long crc;
inline unsigned long crcNotewayFretNumbers = 0x00090000, crcNoteLanes = 0x005a00b9, crcIncomingLanes = 0x35193, crcNotewayGutters = 0x00004a4a, crcStemsAccents = 0x02a50002, crcBendSlideIndicators = 0x4922065c;
inline unsigned long crcSkylinePurple = 0x65b846aa, crcSkylineOrange = 0xbad9e064, crcSkylineBackground = 0xc605fbd2, crcSkylineShadow = 0xff1c61ff;
inline unsigned long crcHeadstock0 = 0x008d5439, crcHeadstock1 = 0x000d4439, crcHeadstock2 = 0x00000000, crcHeadstock3 = 0xa55470f6, crcHeadstock4 = 0x008f4039;
inline unsigned long crcLyrics = 0x00000000;

inline float rainbowSpeed = 2.f;
inline const int randomTextureCount = 10;
inline LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow;
inline LPDIRECT3DTEXTURE9 pCurrTexture, pCurrRainbowTexture, pCurrNotewayTexture;
inline LPDIRECT3DTEXTURE9 gradientTextureNormal, gradientTextureSeven, nonexistentTexture, additiveNoteTexture, customStringColorTexture, customNoteColorTexture, twitchUserDefinedTexture, notewayTexture, gutterTexture, fretNumTexture;
inline LPDIRECT3DTEXTURE9 customHeadstockTexture;
inline LPDIRECT3DTEXTURE9 customGreenScreenWall_Stage0, customGreenScreenWall_Stage1, customGreenScreenWall_Stage2, customGreenScreenWall_Stage3, customGreenScreenWall_Stage4, customGreenScreenWall_Stage5, customGreenScreenWall_Stage6;
inline std::vector<LPDIRECT3DTEXTURE9> randomTextures(randomTextureCount);
inline std::vector<LPDIRECT3DTEXTURE9> rainbowTextures((const unsigned int)(360.0f / rainbowSpeed));
inline ColorList randomTextureColors;

inline int selectedIdx = 0, counter = 0, currentRandomTexture = 0;
inline unsigned int currIdx = 0;
inline INT currStride, currNumVertices, currPrimCount, currStartIndex, currStartRegister, currPrimType, currDeclType, currVectorCount, currNumElements;
inline bool cbEnabled, generateTexture = false;
inline const char* comboStringsItems[] = { "0", "1", "2", "3", "4", "5" };
inline static int selectedString = 0, selectedDevice = 0;
inline static int strR, strG, strB;

inline int StringChangeInterval = 50; // In ms
inline bool RandomTexturesEnabled = false;
inline int currentRandTexture = 0;

inline float realSongSpeed;

//  {8, 10, 8}, //  Massive UI (Artwork, preview skylines, loading screens, etc)

inline  std::vector<Mesh> sevenstring{
	{36, 40, 76}, // Note Heads
	{36, 40, 50}, // Open Strings
	{32, 8, 10}, // Indicators
	{36, 100, 200}, // Note Head Symbols
	{32, 104, 187}, // Highlighted Note Head
	{34, 40, 76} // "Some Note Head Stuff" - Lovro
};

inline std::vector<ThiccMesh> headstockThicc{
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

inline std::vector<ThiccMesh> inlays{
	{ 32, 8, 9, 0, 0, 4, 2, 12, 4 }, { 32, 16, 18, 0, 0, 4, 2, 12, 4 }, // Standard Dot (2nd for 12th/24th fret)
};

inline std::vector<ThiccMesh> noteModifiers{ // Note assets that need to be changed with string colors or they give a flickering effect.
	{ 36, 180, 296, 0, 0, 4, 2, 4, 7 }, // Open Tap
	{ 36, 27, 57, 0, 0, 4, 2, 4, 7 }, // Open Slap / Open Chord Pull-off / Open Chord Pop
	{ 36, 40, 78, 0, 0, 4, 2, 4, 7 }, // Note Ho-Po / Note Harmonic
	{ 36, 44, 92, 0, 0, 4, 2, 4, 7 }, // Far Away Taps
	{ 36, 205, 313, 0, 0, 4, 2, 4, 7 }, // Open Chord Slap
	{ 32, 48, 72, 0, 0, 4, 2, 4, 7 }, // Open Arpeggio
	{ 32, 12, 24, 0, 0, 4, 2, 4, 7 }, // Arpeggio
	{ 36, 450, 482, 0, 0, 4, 2, 4, 7 }, // Pinch Harmonic ~ 1 measure ahead
};

inline std::vector<ThiccMesh> tunerHighlight{
	{ 60, 392, 198, 0, 0, 4, 2, 4, 2 }, { 60, 68, 52, 0, 0, 4, 2, 4, 2 }, // 3+3
	{ 60, 588, 316, 0, 0, 4, 2, 4, 2 }, // 6-inline
	{ 60, 992, 604, 0, 0, 4, 2, 4, 2 }, // 2+2
	{ 60, 306, 218, 0, 0, 4, 2, 4, 2 }, // 4-inline
};

inline std::vector<ThiccMesh> leftyFix{
	{ 60, 392, 198, 0, 0, 4, 2, 9, 7 }, // Machine Heads 3+3
	{ 44, 538, 406, 0, 0, 4, 2, 9, 4}, // Strings past tuner 3+3
};

inline std::vector<ThiccMesh> headstonks{

	{ 76, 3284, 1787, 0, 0, 4, 2, 8, 3 }, // 3+3
	{ 76, 2311, 1268, 0, 0, 4, 2, 9, 7 }, // 6-inline
	{ 84, 1232, 699, 0, 0, 4, 2, 9, 7 }, // 2+2
	{ 84, 1067, 869, 0, 0, 4, 2, 9, 7 }, // 4-inline

	{ 76, 3284, 1787, 0, 0, 4, 2, 9, 7 }, // 3+3 LEFTY
	{ 84, 1232, 699, 0, 0, 4, 2, 8, 3 }, // 2+2 LEFTY
	{ 68, 1067, 612, 0, 0, 4, 2, 9, 7}, // 4-inline LEFTY
};

// All the single (thicc) ladies
inline std::vector<ThiccMesh> laneMarkers{ { 32, 6, 8, 0, 0, 4, 2, 4, 7 } };
inline std::vector<ThiccMesh> lyrics{ { 12, 2, 4, 0, 0, 4, 2, 1, 2 } };
inline std::vector<ThiccMesh> fretless{ { 56, 264, 273, 0, 0, 4, 2, 13, 6 } };
inline std::vector<ThiccMesh> tuningLetters{ { 32, 2, 4, 0, 0, 4, 2, 12, 4 } };
inline std::vector<ThiccMesh> greenScreenWallMesh{ { 92, 2, 6, 0, 0, 4, 2, 8, 9 } };
inline std::vector<ThiccMesh> nostrings{ { 12, 1536, 1199, 0, 0, 4, 2, 9, 4 } };
inline std::vector<ThiccMesh> drunkMode{ { 12, 2, 4, 0, 0, 4, 2, 12, 2 } };
inline std::vector<ThiccMesh> preSongTunerFog{ { 12, 2, 4, 0, 0, 4, 2, 4, 2 } };

// All the single (thin) ladies
inline std::vector<Mesh> noteHighway{ { 32, 2, 4} };

inline std::vector<ThiccMesh> allMeshes;
inline std::vector<ThiccMesh> removedMeshes;

// Misc
#define NOTE_STEMS (Stride == 32 && PrimCount == 2 && NumVertices == 4)
#define CHORD_OUTLINE_THINGY (Stride == 34 && PrimCount == 48 && NumVertices == 80)
#define CHORD_OUTLINE_CORNERS (Stride == 24 && PrimCount == 108 && NumVertices == 208)
#define PAUSE_MENU_BGD (Stride == 16 && PrimCount == 18 && NumVertices == 16)
#define BUTTONS_BOTTOM_1 (Stride == 8 && PrimCount == 20 && NumVertices == 16)
#define VERTICAL_LINES (Stride == 32 && PrimCount == 6 && NumVertices == 8)
#define CHORD_NAMES (Stride == 24 && PrimCount == 2 && NumVertices == 4)
#define FHP (Stride == 12 && PrimCount == 6 && NumVertices == 8)
#define SLIDERS_AND_BUTTONS (Stride == 8 && PrimCount == 20 && NumVertices == 16)
#define OPEN_NOTE_ACCENTS (Stride == 32 && PrimCount == 4 && NumVertices == 6)
#define NOTE_TAILS (Stride == 12)
#define POSSIBLE_HEADSTOCKS (Stride == 44 || Stride == 56 || Stride == 60 || Stride == 68 || Stride == 76 || Stride == 84)
#define POSSIBLE_SKYLINE (Stride == 16)
#define SHOW_TEXTURE oDrawIndexedPrimitive(pDevice, PrimType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount)
#define REMOVE_TEXTURE (D3D_OK)

/*------------------------ CRC Calculation --------------------------------------- */
inline std::vector<LPDIRECT3DTEXTURE9> headstockTexturePointers; // the guitar / bass headstock
inline std::vector<LPDIRECT3DTEXTURE9> skylineTexturePointers; // the dynamic difficulty bars
inline std::vector<LPDIRECT3DTEXTURE9> notewayTexturePointers; // stems & accents

inline LPDIRECT3DBASETEXTURE9 pBaseTextures[3];
inline LPDIRECT3DTEXTURE9 pCurrTextures[3];

inline void AddToTextureList(std::vector<LPDIRECT3DTEXTURE9>& textureList, LPDIRECT3DTEXTURE9 newTexture) {
	if (std::find(std::begin(textureList), std::end(textureList), newTexture) == std::end(textureList))
		textureList.push_back(newTexture);
}

inline DWORD QuickCheckSum(DWORD* BufferData, size_t Size) // Yes, we are aware it's not exactly CRC anymore, but it does the job :)
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

inline bool CRCForTexture(LPDIRECT3DTEXTURE9 texture, IDirect3DDevice9* pDevice, DWORD& o_crc) {
	_LOG_INIT;

	_LOG_SETLEVEL(LogLevel::Error);

	D3DLOCKED_RECT lockedRect;

	if (texture->LockRect(0, &lockedRect, NULL, D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY) == D3D_OK) {
		o_crc = 0;
		DWORD* pData = (DWORD*)lockedRect.pBits;
		if (pData != NULL) {
			o_crc = QuickCheckSum(pData, 20);

			texture->UnlockRect(0);
			return true;
		}
		else {
			_LOG("CRCForTexture: FAILED. lockedRect.pBits == NULL" << std::endl);
			return false;
		}
	}
	else {
		_LOG("CRCForTexture: FAILED. LockRect == D3DERR_INVALIDCALL" << std::endl);

		D3DCAPS9 pDeviceCaps;
		IDirect3DSurface9* pRenderTarget, *newRenderTarget;
		D3DSURFACE_DESC surfaceDesc;

		pDevice->GetDeviceCaps(&pDeviceCaps);

		for (int i = 0; i < pDeviceCaps.NumSimultaneousRTs - 1; i++) {
			_LOG_SETLEVEL(LogLevel::Info);
			_LOG("CRCForTexture: Trying RenderTarget(" << i << ")" << std::endl);

			HRESULT rRenderTarget = pDevice->GetRenderTarget(i, &pRenderTarget);
			
			if (rRenderTarget == D3D_OK) {
				pRenderTarget->GetDesc(&surfaceDesc);
				std::string poolType;

				short pool = surfaceDesc.Pool;

				switch (pool) {
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

				_LOG("CRCForTexture: RenderTarget(" << i << ")->Pool == " << poolType << std::endl);
			}
			else if (rRenderTarget == D3DERR_NOTFOUND)
			{
				_LOG_SETLEVEL(LogLevel::Error);
				_LOG("CRCForTexture: No Render Target At Index: " << i << std::endl);
			}
			else
			{
				_LOG_SETLEVEL(LogLevel::Error);
				_LOG("CRCForTexure: pDevice->GetRenderTarget(" << i << ") has an invalid argument" << std::endl);
			}
			

			if (pRenderTarget != nullptr)
				pRenderTarget->Release(); // Gotta release it or it'll leak everywhere.
		}
		_LOG_SETLEVEL(LogLevel::Info);
		_LOG("CRCForTexture: END" << std::endl);
		return false;
	}
}

