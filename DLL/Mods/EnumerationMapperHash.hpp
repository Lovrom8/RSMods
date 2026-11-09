#pragma once
#include <cstddef>
#include <cstdint>

namespace EnumerationMapperHash {
    // Native FNV uses the signed first byte of each character; non-ASCII retains native hashing.
    inline bool Ascii(const unsigned char* data, size_t size, uint32_t& hash) {
        hash = 0x811C9DC5u;
        for (size_t i = 0; i < size; ++i) {
            if (data[i] == 0 || data[i] >= 0x80) return false;
            hash = (hash ^ data[i]) * 0x01000193u;
        }
        return true;
    }
    struct EngineString {
        union { unsigned char local[16]; const unsigned char* heap; } data;
        const unsigned char* finish;
        const void* storage;
    };
    static_assert(sizeof(EngineString) == 0x18);
    static_assert(offsetof(EngineString, finish) == 0x10);
    static_assert(offsetof(EngineString, storage) == 0x14);
}
