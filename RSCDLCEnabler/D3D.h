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
LPDIRECT3DTEXTURE9 gradientTextureNormal, gradientTextureSeven, nonexistentTexture, additiveNoteTexture, normalBMP;
LPDIRECT3DTEXTURE9 stemTexture;

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

/*------------------------ CRC Calculation --------------------------------------- */
unsigned long crctable[256] = {
		 0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
		 0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
		 0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
		 0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
		 0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
		 0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
		 0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
		 0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
		 0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
		 0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
		 0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
		 0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
		 0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
		 0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
		 0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
		 0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
		 0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
		 0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
		 0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
		 0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
		 0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
		 0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
		 0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
		 0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
		 0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
		 0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
		 0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
		 0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
		 0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
		 0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
		 0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
		 0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
		 0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
		 0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
		 0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
		 0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
		 0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
		 0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
		 0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
		 0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
		 0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
		 0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
		 0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
		 0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
		 0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
		 0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
		 0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
		 0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
		 0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
		 0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
		 0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
		 0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
		 0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
		 0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
		 0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
		 0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
		 0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
		 0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
		 0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
		 0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
		 0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
		 0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
		 0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
		 0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};

unsigned long crc32(unsigned char* block, unsigned long len)
{
	unsigned long crc = 0xFFFFFFFF;

	while (len--)
		crc = crctable[(crc ^ *block++) & 0xFFL] ^ (crc >> 8);

	return crc ^ 0xFFFFFFFF;
}

/*-------------------------------------------------------------------------*/