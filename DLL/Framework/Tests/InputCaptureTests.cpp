#include "../InputCapture.hpp"

#include <cstdint>
#include <iostream>
#include <string>

using Framework::InputCapture;

namespace {
	int g_failures = 0;

	void Check(const std::string& name, bool ok) {
		if (ok) {
			std::cout << "[PASS] " << name << "\n";
		}
		else {
			std::cout << "[FAIL] " << name << "\n";
			++g_failures;
		}
	}

	// DIMOUSESTATE2 layout, without pulling dinput.h into a host-agnostic test.
	struct MouseState2 {
		int32_t x, y, z;
		uint8_t buttons[8];
	};
	static_assert(sizeof(MouseState2) == InputCapture::kMouseState2Size);

	MouseState2 Moving() { return { 5, -3, 120, { 0x80, 0, 0x80, 0, 0, 0, 0, 0x80 } }; }

	bool IsBlank(const MouseState2& s) {
		if (s.x || s.y || s.z) return false;
		for (uint8_t b : s.buttons) if (b) return false;
		return true;
	}

	void NotCapturedPassesThrough() {
		InputCapture ic;
		MouseState2 s = Moving();
		uint32_t count = 4;
		Check("uncaptured state untouched", !ic.FilterDeviceState(true, sizeof(s), &s) && s.z == 120);
		Check("uncaptured events untouched", !ic.FilterDeviceData(true, &count) && count == 4);
	}

	void CapturedBlanksMouse() {
		InputCapture ic;
		ic.SetMouseCaptured(true);
		MouseState2 s = Moving();
		Check("captured DIMOUSESTATE2 blanked", ic.FilterDeviceState(true, sizeof(s), &s) && IsBlank(s));

		MouseState2 small = Moving();
		Check("captured DIMOUSESTATE blanked", ic.FilterDeviceState(true, InputCapture::kMouseStateSize, &small)
			&& small.x == 0 && small.z == 0 && small.buttons[3] == 0);
		Check("DIMOUSESTATE leaves bytes past its size", small.buttons[7] == 0x80);

		uint32_t count = 4;
		Check("captured events dropped", ic.FilterDeviceData(true, &count) && count == 0);
	}

	void OtherDevicesUntouched() {
		InputCapture ic;
		ic.SetMouseCaptured(true);
		MouseState2 s = Moving();
		uint32_t count = 4;
		Check("keyboard state untouched", !ic.FilterDeviceState(false, sizeof(s), &s) && s.x == 5);
		Check("keyboard events untouched", !ic.FilterDeviceData(false, &count) && count == 4);

		uint8_t joystick[80] = { 1 };   // DIJOYSTATE: not a mouse format even if the device claims to be one
		Check("unknown format untouched", !ic.FilterDeviceState(true, sizeof(joystick), joystick) && joystick[0] == 1);
	}

	void NullBuffersIgnored() {
		InputCapture ic;
		ic.SetMouseCaptured(true);
		Check("null state ignored", !ic.FilterDeviceState(true, InputCapture::kMouseStateSize, nullptr));
		Check("null count ignored", !ic.FilterDeviceData(true, nullptr));
	}

	void ReleaseRestoresInput() {
		InputCapture ic;
		ic.SetMouseCaptured(true);
		ic.SetMouseCaptured(false);
		MouseState2 s = Moving();
		Check("released -> state passes", !ic.FilterDeviceState(true, sizeof(s), &s) && s.x == 5);
	}
}

int main() {
	NotCapturedPassesThrough();
	CapturedBlanksMouse();
	OtherDevicesUntouched();
	NullBuffersIgnored();
	ReleaseRestoresInput();

	std::cout << (g_failures ? "FAILED" : "ALL PASSED") << " (" << g_failures << " failures)\n";
	return g_failures ? 1 : 0;
}
