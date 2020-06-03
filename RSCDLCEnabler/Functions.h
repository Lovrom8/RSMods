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

typedef HRESULT(__stdcall *f_EndScene)(IDirect3DDevice9 *pDevice);
f_EndScene oEndScene; // Original Endscene

typedef HRESULT(__stdcall* f_Reset)(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS*);
f_Reset oReset;
