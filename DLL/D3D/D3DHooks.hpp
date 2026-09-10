#pragma once
#include <atomic>
#include <format>

#include "../Mods/ExtendedRangeMode.hpp"

namespace D3DHooks {
	inline UINT StartRegister;
	inline UINT VectorCount;

	inline UINT Stride;
	inline D3DVERTEXBUFFER_DESC vdesc;

	inline IDirect3DVertexDeclaration9* pDecl;
	inline D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
	inline UINT NumElements;

	inline IDirect3DVertexShader9* vShader;
	inline UINT vSize;

	inline IDirect3DPixelShader9* pShader;
	inline UINT pSize;

	inline IDirect3DTexture9* texture;
	inline D3DSURFACE_DESC sDesc;
	inline D3DLOCKED_RECT pLockedRect;

	inline LPDIRECT3DVERTEXBUFFER9 Stream_Data;
	inline UINT Offset = 0;
	inline UINT vertexBufferSize;

	inline bool debug = true;

	inline std::atomic_bool regenerateUserDefinedTexture = false;
	
	inline HWND hThisWnd = NULL;
	inline WNDPROC oWndProc = NULL;

	inline bool cachedIsInSong = false;

	HRESULT APIENTRY Hook_SetVertexDeclaration(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexDeclaration9* pdecl);
	HRESULT APIENTRY Hook_DP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT StartIndex, UINT PrimCount);
	HRESULT APIENTRY Hook_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, const float* pConstantData, UINT Vector4fCount);
	HRESULT APIENTRY Hook_SetVertexShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexShader9* veShader);
	HRESULT APIENTRY Hook_SetPixelShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DPixelShader9* piShader);
	HRESULT APIENTRY Hook_SetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride);
	HRESULT APIENTRY Hook_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
	HRESULT APIENTRY Hook_DIP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimCount);
	HRESULT APIENTRY Hook_EndScene(IDirect3DDevice9* pDevice);

	// Mod Settings
	inline bool resetHeadstockCache = true; // Do we want to reset the headstock cache? Triggers when opening tuning menu
	inline bool toggleSkyline = false; // Do we want to toggle the skyline right now? Triggers to false when turned on/ off
	inline int EnumSliderVal = 10000; // Sleep every X ms for enumeration (1000 ms = 1s)
	inline bool SkylineOff = false; // Is the skyline disabled right now? Toggles when skyline turns off (True - No Skyline, False - Skyline)
	inline bool DrawSkylineInMenu = false; // If the user is in "Song" mode of Toggle Skyline, should we draw the skyline in this menu (True - Skyline, False - No Skyline)
	
	inline std::atomic<bool> RemoveHeadstockInThisMenu = false; // If true, the headstock of the guitar / bass will be disabled in this menu. (True - No Headstock, False - Keep Headstock)
	inline bool DiscoModeEnabled = false; // If true, we do the trippy effects that disco mode is known for (True - Disco, False - Normal).
	inline std::map<IDirect3DDevice9*, std::pair<DWORD, DWORD>> DiscoModeInitialSetting; // List of all the pDevices that have been affected by Disco Mode
	inline bool ToggleOffLoftWhenDoneWithMod = false; // If true, we save this until after the mod is done and re-enable it.
	inline bool PrideMode = false; // If true, the background will be cycle through colors.
	inline bool RainbowNotes = false; // If true, the notes will turn rainbow along with the stems

	inline std::atomic_bool RecreateTextures = true; // Initialized to true so textures generate on frame 1. Set to true whenever settings update.

	inline const int randomTextureCount = 10;
	inline LPDIRECT3DTEXTURE9 twitchUserDefinedTexture = nullptr;
	inline std::vector<LPDIRECT3DTEXTURE9> randomTextures(randomTextureCount);
	inline ColorList randomTextureColors(randomTextureCount);
	inline int currentRandomTexture = 0;

	// Dev Functions
	inline bool startLogging = false; // Should we log what's happening in Hook_DIP? Logs to log.txt in your RS2014 directory

	void CheckRecreateTextures(IDirect3DDevice9* pDevice);
	void GenerateRandomTextures(IDirect3DDevice9* pDevice);
	void RegenerateTwitchNoteColors(IDirect3DDevice9* pDevice);
	void InitializeCrcProvider();

	// Refreshes the headstock texture cache based on the current/previous menu. Call once per menu tick.
	void UpdateHeadstockCacheForMenu();

	inline HWND cachedGameHwnd = nullptr;
	inline HWND GetGameWindow() {
		if (cachedGameHwnd) {
			return cachedGameHwnd;
		}

		cachedGameHwnd = FindWindowA(nullptr, "Rocksmith 2014");

		return cachedGameHwnd;
	}
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef HRESULT(WINAPI* tDrawIndexedPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
inline tDrawIndexedPrimitive oDrawIndexedPrimitive;

typedef HRESULT(WINAPI* tBeginScene)(IDirect3DDevice9*);
inline tBeginScene oBeginScene;

typedef HRESULT(WINAPI* tDrawPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
inline tDrawPrimitive oDrawPrimitive;

typedef HRESULT(__stdcall* tEndScene)(IDirect3DDevice9*);
inline tEndScene oEndScene;

typedef HRESULT(APIENTRY* tReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
inline tReset oReset;

typedef HRESULT(APIENTRY* tSetStreamSource)(IDirect3DDevice9*, UINT, IDirect3DVertexBuffer9*, UINT, UINT);
inline tSetStreamSource oSetStreamSource;

typedef HRESULT(APIENTRY* tSetVertexDeclaration)(IDirect3DDevice9*, IDirect3DVertexDeclaration9*);
inline tSetVertexDeclaration oSetVertexDeclaration;

typedef HRESULT(APIENTRY* tSetVertexShaderConstantF)(IDirect3DDevice9*, UINT, const float*, UINT);
inline tSetVertexShaderConstantF oSetVertexShaderConstantF;

typedef HRESULT(APIENTRY* tSetVertexShader)(IDirect3DDevice9*, IDirect3DVertexShader9*);
inline tSetVertexShader oSetVertexShader;

typedef HRESULT(APIENTRY* tSetPixelShader)(IDirect3DDevice9*, IDirect3DPixelShader9*);;
inline tSetPixelShader oSetPixelShader;

typedef HRESULT(APIENTRY* tSetTexture)(IDirect3DDevice9*, DWORD, IDirect3DBaseTexture9*);
inline tSetTexture oSetTexture;

typedef HRESULT(APIENTRY* tPresent) (IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
inline tPresent oPresent;

/// <summary>
/// Dump stages 0-7 of texture. https://docs.microsoft.com/en-us/windows/win32/direct3d9/texture-blending
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="file_prefix"> - Prefix for file name. "_stage#.png" is appended to this to end up with the file name.</param>
inline void DumpTextureStages(IDirect3DDevice9* pDevice, std::string file_prefix) {
	LPDIRECT3DBASETEXTURE9 baseTexture;
	D3DCAPS9 deviceCaps;
	pDevice->GetDeviceCaps(&deviceCaps);

	file_prefix += "_stage";

	// The reason we get Device Caps is because some devices don't support 8 stages (0-7), so we have to account for that.

	for (DWORD i = 0; i < deviceCaps.MaxTextureBlendStages; i++) {
		std::string filename = std::string(file_prefix + std::to_string(i) + ".png").c_str();
		pDevice->GetTexture(i, &baseTexture);
		D3DXSaveTextureToFileA(filename.c_str(), D3DXIFF_PNG, baseTexture, NULL);
	}
}
