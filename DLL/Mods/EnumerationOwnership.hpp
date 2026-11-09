#pragma once
#include <cstdint>

// Answer scan-time ownership calls locally; song loading retains its native ownership check.
namespace EnumerationOwnership {
    unsigned int Answered();
    void Log();
}
