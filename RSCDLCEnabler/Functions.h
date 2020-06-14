#include "windows.h"
#include "DirectX/d3d9.h"
#include "DirectX/d3dx9.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

typedef void(__thiscall * _tForceEnumeration)(byte *rs_dlc_service_flags); //maybe in the future
_tForceEnumeration forceEnumeration;


void __fastcall tForceEnumeration(byte *rs_dlc_service_flags) {
	return forceEnumeration(rs_dlc_service_flags);
}

typedef HRESULT(APIENTRY* tReset)(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS*);
tReset oReset;

typedef HRESULT(APIENTRY* tEndScene)(IDirect3DDevice9* pDevice);
tEndScene oEndScene = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef HRESULT(WINAPI* tDrawIndexedPrimitive)(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE PrimType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
tDrawIndexedPrimitive oDrawIndexedPrimitive;

typedef HRESULT(WINAPI* tBeginScene)(IDirect3DDevice9* pDevice);
tBeginScene oBeginScene;