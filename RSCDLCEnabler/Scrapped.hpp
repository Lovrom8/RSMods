/*#include "windows.h"

DWORD hookAddr_Enumeration = 0x017B60AF;
DWORD hookBackAddr_Enumeration;
void __declspec(naked) hook_FakeCurrentSection() {
	__asm {
		lea esi, [eax + 0x8C]
		pushad
		mov ebx, [esi]
		mov byte ptr[ebx], 0x53
		mov byte ptr[ebx + 0x1], 0x68
		mov byte ptr[ebx + 0x2], 0x6F
		mov byte ptr[ebx + 0x3], 0x70
		//mov byte ptr[ebx+0x4], bl
		popad
		jmp[hookBackAddr_Enumeration]
	}
}
*/

/* NOTE TO SELF: don't hook just before a conditional jump :D
//DWORD hookAddrCustom = 0x013957DD;
void __declspec(naked) songListCustomHook() {
	__asm { //ECX = "$[4696x]SONG LIST"
		mov esi, DWORD PTR[esi]
		cmp DWORD PTR[ebp + 0xC], ecx //OG part

		pushad
		//pushfd

		//cmp byte ptr [ecx + 0x2], 0x34
		//JNZ GTFO //CMP compares chars by division, so if diff > 0 -> not equal
		nop

		GTFO:
		popad
		jmp[hookBackAddr2]
	}
}
*/


//At 0x01529F61, into EAX is saved pointer to: either the clean string (if index exists) or a string without the $[] part (eg. either it becomes SONG LIST or #46967#SONG LIST
//so prolly a good place to hook - check if it contains # and check the last digit to determine which index to put out

/* My version for custom song list names, by using the 0x01529F61 method 

void __declspec(naked) hook_basicCustomTitles() {
	__asm {
		lea eax, dword ptr ss : [ebp - 0x80] // ebp-0x80 = pointer to the "clean" string address
		pushad

		mov ebx, [eax] //dereference it first
		cmp byte ptr[ebx], '#'
		jne GTFO

		mov byte ptr[ebx + 3], 0x35 //very basic, not as developed as Koko's
		mov byte ptr[ebx + 7], 0x44

		GTFO:
		popad
			jmp[hookBackAddr_CustomNames]
	}

	void RandomMemStuff::HookSongLists() {
	SetFakeListNames();

	len = 6;
	hookBackAddr_CustomNames = hookAddr_ModifyCleanString + len;

	MemUtil.PlaceHook((void*)hookAddr_ModifyCleanString, hook_basicCustomTitles, len);
}
}*/

/*-----------------------DX CRAP ----------------------*/

/* //std::string  out((char*)D3DX9.D3DCreate);
	//std::string out((char*)NULL);

	char winpath[MAX_PATH];
	HINSTANCE hL;
	BYTE* D3DCreate;
	GetSystemDirectoryA(winpath, sizeof(winpath));
	strcat_s(winpath, "\\d3d9.dll");

	hL = LoadLibraryA(winpath);

	if(!hL)
		MessageBoxA(NULL, "y", NULL, NULL);

	D3DCreate = (BYTE*)GetProcAddress(hL, "Direct3DCreate9");



	//oDirect3DCreate9 = (tDirect3DCreate9)DetourFunction(D3DCreate, (PBYTE)hkDirect3DCreate9);
	oDirect3DCreate9 = (tDirect3DCreate9)MemUtil.TrampHook(D3DCreate, (PBYTE)hkDirect3DCreate9, 7);

	while (g_Device == NULL) Sleep(100);

	if (g_Device == NULL)
		MessageBoxA(NULL, "z", NULL, NULL);

	DWORD* pVTable = (DWORD*)g_Device;
	pVTable = (DWORD*)pVTable[0];

	if (pVTable == NULL)
		MessageBoxA(NULL, "bb", NULL, NULL);
		*/

		//PBYTE pVtable = (BYTE*)0x135dd50;

		//oEndScene = (tEndScene)MemUtil.TrampHook((PBYTE) (void*)0x135dd50 + 42, (PBYTE)Hook_EndScene, 5);
		//oEndScene = (tEndScene)DetourFunction((PBYTE)(*(DWORD*)0x135dd50 + 42), (PBYTE)Hook_EndScene);
		//oEndScene = (tEndScene)MemUtil.TrampHook((PBYTE)(*(DWORD*)0x135dd50 + 42), (PBYTE)Hook_EndScene, 5);

/*--------------------------------------------------------------------*/

/*

void showptr() {
	//char temp_buffer[64];
//sprintf(temp_buffer, "%p", (void*)D3DCreate);
//MessageBoxA(0, temp_buffer, "Pointer value", MB_OK);
}

//	DWORD* dVtable = (DWORD*)0x135dd50;
//	dVtable = (DWORD*)dVtable[0];

*/