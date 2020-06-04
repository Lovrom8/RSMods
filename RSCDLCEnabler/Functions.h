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

typedef HRESULT(APIENTRY* f_Reset)(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS*);
f_Reset oReset;

typedef HRESULT(APIENTRY* tEndScene)(IDirect3DDevice9* pDevice);
tEndScene oEndScene = nullptr;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
