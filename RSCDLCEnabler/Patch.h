#include <windows.h>

#pragma once
class Patch
{
public:
	bool PatchAdr(LPVOID dst, LPVOID src, size_t len);
	bool PlaceHook(void * hookSpot, void * ourFunct, int len);
};