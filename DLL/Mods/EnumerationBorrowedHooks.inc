	uintptr_t g_nativeIntern = 0, g_internFallback = 0;
	void __declspec(naked) BorrowInternHook() {
		__asm { pushfd }
		__asm { pushad }
		__asm { mov eax, [esp + 40] }
		__asm { push [ebp - 2Ch] }
		__asm { push eax }
		__asm { push ecx }
		__asm { call BorrowName }
		__asm { add esp, 12 }
		__asm { mov [esp + 28], eax }
		__asm { popad }
		__asm { popfd }
		__asm { test eax, eax }
		__asm { jz native }
		__asm { ret }
	 native:
		__asm { jmp dword ptr [g_internFallback] }
	}
	void __declspec(naked) ReleaseStage() {
		__asm { pushfd }
		__asm { pushad }
		__asm { push edi }
		__asm { call IsBorrowedName }
		__asm { add esp, 4 }
		__asm { test eax, eax }
		__asm { jnz borrowed }
		__asm { popad }
		__asm { popfd }
		__asm { test edi, edi }
		__asm { jz done }
		__asm { mov esi, [g_internLock] }
		__asm { mov esi, [esi] }
		__asm { jmp dword ptr [g_releaseBody] }
	 borrowed:
		__asm { popad }
		__asm { popfd }
	 done:
		__asm { jmp dword ptr [g_releaseDone] }
	}
