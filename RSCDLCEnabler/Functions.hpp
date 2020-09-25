#pragma once
#include "windows.h"

/*----------------------- FORCE ENUMERATION -----------------------*/

typedef void(__thiscall* _tForceEnumeration)(byte* rs_dlc_service_flags); //maybe in the future
_tForceEnumeration forceEnumeration;


void __fastcall tForceEnumeration(byte* rs_dlc_service_flags) {
	return forceEnumeration(rs_dlc_service_flags);
}