#pragma once

// Standalone stand-in for the DLL's stdafx.h, found first via /I . so the test
// build never sees the real one. The shifter only needs the standard headers the
// project headers assume a precompiled header has already provided; the real
// stdafx.h drags in Detours lib pragmas and a MIDI pedal table with dynamic
// initializers into every translation unit that includes it.
#include <cstdint>
#include <string>
