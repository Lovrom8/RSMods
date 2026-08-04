#pragma once

#include "CaptureFormat.h"
#include "IInputProcessor.hpp"

#include <cstddef>

// Intercepts the guitar signal underneath RS_ASIO, on the ASIO driver's own buffers.
//
// RS_ASIO does not create the driver through COM. It reads InprocServer32 for the driver's
// CLSID and loads that DLL directly, then goes through the module's class factory. Nothing
// in that path calls CoCreateInstance, which is why the WASAPI side probe never saw it:
// RS_ASIO's log shows "Creating AsioSharedHost - dll: ...MAudioMTrackSoloDuo_Asio.dll" with
// no corresponding creation in ours.
//
// So we get there first. The driver name comes from RS_ASIO.ini, the CLSID from
// HKLM\SOFTWARE\ASIO, and the module path from InprocServer32. Loading that module early
// means RS_ASIO later gets the same already-loaded module, and calls a DllGetClassObject we
// have already detoured. From there: class factory -> IASIO -> createBuffers, which hands
// over the ASIOCallbacks RS_ASIO registered. Wrapping bufferSwitch puts us on the driver's
// input buffers before RS_ASIO copies them anywhere.
//
// Two things about IASIO differ from the WASAPI interfaces. It is a plain C++ class with
// virtual methods rather than a COM interface, so on x86 its methods are __thiscall, not
// __stdcall, and the hooks are declared __fastcall with a dummy EDX argument. And its
// buffers are per channel and non-interleaved, so a processor here sees one mono channel
// rather than an interleaved block.
	namespace Audio::AsioHook
{
	constexpr size_t INPUT_ROUTE_COUNT = 2;

	void Install();

	// Called from the game loop. Enables processing once the driver has built its buffers
	// and a processor plus input channel are in place; createBuffers happens ~40s after
	// Install, so readiness can only be observed by polling.
	void Poll();

	// Ownership stays with the caller, which must keep both processors alive for as long as
	// the ASIO stream runs. Route 0 is [Asio.Input.0], route 1 is [Asio.Input.1].
	void SetProcessor(size_t routeIndex, IInputProcessor* inputProcessor);

	void SetProcessingEnabled(bool enabled);
	bool IsProcessingEnabled();
	bool IsInputConfigured(size_t routeIndex);
	bool IsInputReady(size_t routeIndex);
}
