#pragma once
#include "d3d9.h"
#include "../MemHelpers.hpp"
#include "D3D.hpp"
#include <intrin.h>
#include "../Mods/ExtendedRangeMode.hpp"


#include "../Lib/ImGUI/imgui.h"
#include "../Lib/ImGUI/imgui_impl_dx9.h"
#include "../Lib/ImGUI/imgui_impl_win32.h"
#include "../Lib/ImGUI/RobotoFont.cpp"

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

#ifdef _DEBUG
	inline bool debug = true;
#else
	inline bool debug = false;
#endif

	inline bool regenerateUserDefinedTexture = false;
	
	inline HWND hThisWnd = NULL;
	inline WNDPROC oWndProc = NULL;

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
	inline bool LoftOff = false; // Is the loft disabled right now? Toggles when loft turns off (True - No Loft, False - Loft)
	inline bool SkylineOff = false; // Is the skyline disabled right now? Toggles when skyline turns off (True - No Skyline, False - Skyline)
	inline bool DrawSkylineInMenu = false; // If the user is in "Song" mode of Toggle Skyline, should we draw the skyline in this menu (True - Skyline, False - No Skyline)
	inline bool GreenScreenWall = false; // If true, set the Greenscreen wall up. This helps call it in Lesson mode for No-Loft users (True - Black wall, False - Loft)
	inline bool GuitarSpeakPresent = false; // If true, read the notes inputted and press the key combo provided. (True - On, False - Off)
	inline bool RemoveLyrics = false; // If true, remove the lyrics from Learn A Song & Non-stop Play. (True - No Lyrics, False - Keep Lyrics)
	inline bool RemoveHeadstockInThisMenu = false; // If true, the headstock of the guitar / bass will be disabled in this menu. (True - No Headstock, False - Keep Headstock)
	inline bool showSongTimerOnScreen = false; // If true, the current song timer will be shown in the top-right of the screen. This will only work when inside a song. (True - Show, False - Hide)
	inline bool DiscoModeEnabled = false; // If true, we do the trippy effects that disco mode is known for (True - Disco, False - Normal).
	inline std::map<IDirect3DDevice9*, std::pair<DWORD, DWORD>> DiscoModeInitialSetting; // List of all the pDevices that have been affected by Disco Mode
	inline bool ToggleOffLoftWhenDoneWithMod = false; // If true, we save this until after the mod is done and re-enable it.
	inline bool PrideMode = false; // If true, the background will be cycle through colors.
	inline bool RainbowNotes = false; // If true, the notes will turn rainbow along with the stems.
	inline bool AutomatedSongTimer = false; // If true, we will always show the song timer.
	inline bool AutomatedSelectedVolume = false; // If true, we will always show the selected volume.
	inline bool AttemptedERInThisSong = false; // If true, then we are safe to look for the ER variable.
	inline bool UseEROrColorsInThisSong = false; // If true, ER Mode will toggle on (includes custom colors).
	inline bool UseERExclusivelyInThisSong = false; // If true, ER Mode will toggle on.
	inline bool AttemptedERInTuner = false; // If true, then we are safe to look for the ER In Tuner variable.
	inline bool UseERInTuner = false; // If true, ER Mode will toggle on in the tuner.

	// Dev Functions
	inline bool startLogging = false; // Should we log what's happening in Hook_DIP?

	// ImGUI Settings
	inline bool menuEnabled = false; // Do we show the user the ImGUI settings menu?
	inline bool enableColorBlindCheckboxGUI = false; // Do we allow the user to change Colorblind mode in the ImGUI menu?

	// Menus
	inline std::string currentMenu; // Current Menu Name
	inline std::string previousMenu; // Last Menu Name
	inline bool GameLoaded = false; // Has the game gotten to the main menu where you can pick the gamemodes?
	inline bool LessonMode = false; // Is the user in LessonMode?

	// Misc
	inline bool lowPerformancePC = false; // Does your game lag with all of our mods? Toggle on to disable us running mods for an "original DLL" experience.
	inline bool setAllToNoteGradientTexture = false; // Should we override the 6-string note textures with the 7-string note textures?
	inline bool GameClosing = false; // Inform the threads that their work is done just before the game closes

	inline const char* windowName = "Rocksmith 2014"; // Console Window Title
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