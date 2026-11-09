// Replaces call edx: ECX = apps interface, EDX = native query, appid on stack.
namespace EnumerationOwnership {
    namespace {
        std::atomic<unsigned int> answered = 0;
    }
    bool __stdcall Owned(void*, uintptr_t, uint32_t) {
        ++answered;
        return true;
    }
    unsigned int Answered() { return answered.load(); }
    #ifndef ENUMERATION_OWNERSHIP_TEST
    void Log() {
        LOG_INFO("(ENUMERATION) Scan-time ownership: queries=0, answered=" << answered.load() << std::endl);
    }
    #endif
    void __declspec(naked) Dispatch() {
        __asm { push [esp + 4] }
        __asm { push edx }
        __asm { push ecx }
        __asm { call Owned }
        __asm { ret 4 }
    }
}
