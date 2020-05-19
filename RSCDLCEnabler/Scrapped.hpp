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