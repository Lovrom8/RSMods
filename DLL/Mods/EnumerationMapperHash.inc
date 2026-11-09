namespace EnumerationMapperHash {
    uintptr_t native = 0;
    uint32_t __declspec(naked) __cdecl Native(const EngineString*) {
        __asm { push edi }
        __asm { mov edi, [esp + 8] }
        __asm { call dword ptr [native] }
        __asm { pop edi }
        __asm { ret }
    }
    uint32_t __cdecl Hash(const EngineString* text) {
        const auto* data = text->storage == &text->finish ? text->data.local : text->data.heap;
        const auto begin = reinterpret_cast<uintptr_t>(data);
        const auto end = reinterpret_cast<uintptr_t>(text->finish);
        uint32_t result = 0;
        bool fast = data && end >= begin && end - begin <= 4096
            && Ascii(data, end - begin, result);
        if (!fast) return Native(text);
        return result;
    }
    void __declspec(naked) Hook() {
        __asm { push edi }
        __asm { call Hash }
        __asm { add esp, 4 }
        __asm { ret }
    }
}
