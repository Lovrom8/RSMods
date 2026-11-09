// Hash slash-prefixed paths with FNV-1a, omitting supported platform directories.
// Unsupported path shapes retain native hashing.
namespace EnumerationLlid {
    using EnumerationMapperHash::EngineString;
    uintptr_t native = 0;
    using NativeFn = uint32_t(__cdecl*)(const EngineString*);


    bool StartsWith(const char* text, size_t size, const char* prefix, size_t prefixSize) {
        return size >= prefixSize && memcmp(text, prefix, prefixSize) == 0;
    }

    void Mix(uint32_t& hash, const char* text, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            hash ^= static_cast<unsigned char>(text[i]);
            hash *= 0x01000193u;
        }
    }

    bool Compute(const char* path, size_t size, uint32_t& out) {
        if (!size || size > 1024)
            return false;
        for (size_t i = 0; i < size; ++i) {
            const unsigned char c = static_cast<unsigned char>(path[i]);
            if (c < 0x20 || c >= 0x80 || c == '\\' || c == ':' || (c >= 'A' && c <= 'Z'))
                return false;
        }
        size_t keep = 0, skip = 0;
        if (StartsWith(path, size, "songs/bin/generic/", 18)) { keep = 10; skip = 8; }
        else if (StartsWith(path, size, "audio/windows/", 14)) { keep = 6; skip = 8; }
        else if (!StartsWith(path, size, "manifests/", 10) && !StartsWith(path, size, "gfxassets/", 10)
            && !StartsWith(path, size, "songs/arr/", 10) && memchr(path, '/', size) != nullptr)
            return false;
        uint32_t hash = 0x811C9DC5u;
        Mix(hash, "/", 1);
        Mix(hash, path, keep);
        Mix(hash, path + keep + skip, size - keep - skip);
        out = hash;
        return true;
    }

    uint32_t __cdecl Hook(const EngineString* text) {
        const auto* data = reinterpret_cast<const char*>(text->storage == &text->finish ? text->data.local : text->data.heap);
        const auto begin = reinterpret_cast<uintptr_t>(data);
        const auto end = reinterpret_cast<uintptr_t>(text->finish);
        uint32_t result = 0;
        if (data && end >= begin && Compute(data, end - begin, result)) {
            return result;
        }
        return reinterpret_cast<NativeFn>(native)(text);
    }

    // AddReference and SearchFileByLLID push string*; the caller pops it.
    bool AddPatches(std::vector<Patch>& patches, bool includeMapperSite) {
        native = Offsets::func_enumLlid;
        if (!native || !Offsets::enumLlidSearchCall || (includeMapperSite && !Offsets::enumInner49DAB2))
            return false;
        if (includeMapperSite)
            patches.push_back(Branch(Offsets::enumInner49DAB2, native, &Hook, 0xE8));
        patches.push_back(Branch(Offsets::enumLlidSearchCall, native, &Hook, 0xE8));
        return true;
    }

    void Log() {
    }
}
