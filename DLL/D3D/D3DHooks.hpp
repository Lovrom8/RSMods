#pragma once
#include <atomic>
#include <format>
#include <unordered_map>


namespace D3DHooks {
	inline UINT StartRegister;
	inline UINT VectorCount;

	inline UINT Stride;
	inline D3DVERTEXBUFFER_DESC vdesc;

	inline IDirect3DVertexDeclaration9* pDecl;
	inline D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
	inline UINT NumElements;

	inline IDirect3DVertexShader9* vShader;

	inline IDirect3DPixelShader9* pShader;

	inline IDirect3DTexture9* texture;
	inline D3DSURFACE_DESC sDesc;
	inline D3DLOCKED_RECT pLockedRect;

	inline LPDIRECT3DVERTEXBUFFER9 Stream_Data;
	inline UINT Offset = 0;

	inline bool debug = true;

	inline HWND hThisWnd = NULL;
	inline WNDPROC oWndProc = NULL;

	inline bool cachedIsInSong = false;

	// Ultrawide aspect correction.
	inline std::atomic<bool> ultrawideActive = false;   // The setting is on and the backbuffer is wider than 16:9: corrections apply.

	// True while a Guitarcade minigame is being played or paused. Evaluated once
	// per frame in UpdateUltrawideState because the underlying GameState query is
	// a string comparison and the draw path reads this per draw.
	inline std::atomic<bool> ultrawideInGuitarcade = false;
	inline std::atomic<bool> ultrawideInVideoPlayer = false; // The video player composites at its own aspect.
	inline std::atomic<bool> ultrawideSettingOn = false; // Published by UltrawideMod (mod thread) from the Ultrawide setting. The source of the flag above.
	inline float ultrawideClipXScale = 1.0f; // (16/9) / the display aspect: how far the interface has to be squeezed back in.
	inline std::unordered_map<void*, std::pair<UINT, UINT>> ultrawideRenderTargetTextures; // Render thread only: every texture that has been a render target, with its size. Scene-aspect ones mark post-processing draws; off-aspect ones (text strips) mark 16:9 UI.
	inline bool ultrawideBackBufferValid = false;
	inline UINT ultrawideBackBufferWidth = 0;
	inline UINT ultrawideBackBufferHeight = 0;
	inline bool ultrawideRenderTargetIsScene = false;
	inline UINT ultrawideRenderTargetWidth = 0;
	inline UINT ultrawideRenderTargetHeight = 0;

	void UpdateUltrawideState(IDirect3DDevice9* pDevice);

	HRESULT APIENTRY Hook_SetVertexDeclaration(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexDeclaration9* pdecl);
	HRESULT APIENTRY Hook_DP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT StartIndex, UINT PrimCount);
	HRESULT APIENTRY Hook_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 pDevice, UINT StartRegister, const float* pConstantData, UINT Vector4fCount);
	HRESULT APIENTRY Hook_SetVertexShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DVertexShader9* veShader);
	HRESULT APIENTRY Hook_SetPixelShader(LPDIRECT3DDEVICE9 pDevice, IDirect3DPixelShader9* piShader);
	HRESULT APIENTRY Hook_SetStreamSource(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride);
	HRESULT APIENTRY Hook_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
	HRESULT APIENTRY Hook_StretchRect(LPDIRECT3DDEVICE9 pDevice, IDirect3DSurface9* pSourceSurface, const RECT* pSourceRect, IDirect3DSurface9* pDestSurface, const RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);
	HRESULT APIENTRY Hook_SetRenderTarget(LPDIRECT3DDEVICE9 pDevice, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget);
	HRESULT APIENTRY Hook_SetTexture(LPDIRECT3DDEVICE9 pDevice, DWORD Stage, IDirect3DBaseTexture9* pTexture);
	HRESULT APIENTRY Hook_DIP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimCount);
	HRESULT APIENTRY Hook_DrawPrimitiveUP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT PrimCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
	HRESULT APIENTRY Hook_DrawIndexedPrimitiveUP(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, UINT MinVertexIndex, UINT NumVertices, UINT PrimCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
	HRESULT APIENTRY Hook_EndScene(IDirect3DDevice9* pDevice);

	inline std::atomic_bool RecreateTextures = true; // Initialized to true so textures generate on frame 1. Set to true whenever settings update.

	// Dev Functions
	inline bool startLogging = false; // Should we log what's happening in Hook_DIP? Logs to log.txt in your RS2014 directory

	void CheckRecreateTextures(IDirect3DDevice9* pDevice);
	void InitializeCrcProvider();

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

typedef HRESULT(APIENTRY* tDrawPrimitiveUP)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, const void*, UINT);
inline tDrawPrimitiveUP oDrawPrimitiveUP;

typedef HRESULT(APIENTRY* tDrawIndexedPrimitiveUP)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT, UINT, const void*, D3DFORMAT, const void*, UINT);
inline tDrawIndexedPrimitiveUP oDrawIndexedPrimitiveUP;

typedef HRESULT(__stdcall* tEndScene)(IDirect3DDevice9*);
inline tEndScene oEndScene;

typedef HRESULT(APIENTRY* tReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
inline tReset oReset;

typedef HRESULT(APIENTRY* tStretchRect)(IDirect3DDevice9*, IDirect3DSurface9*, const RECT*, IDirect3DSurface9*, const RECT*, D3DTEXTUREFILTERTYPE);
inline tStretchRect oStretchRect;

typedef HRESULT(APIENTRY* tSetRenderTarget)(IDirect3DDevice9*, DWORD, IDirect3DSurface9*);
inline tSetRenderTarget oSetRenderTarget;

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
