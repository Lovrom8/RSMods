#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>

namespace Framework {
	// Keeps overlay clicks away from the game. Rocksmith reads the mouse only through DirectInput, so
	// swallowing WM_ messages isn't enough: while an overlay owns the mouse, the game's DirectInput mouse
	// reads must come back empty. The host patches dinput8 (DirectInputCapture) and routes every read
	// through the filters below; the render thread sets the flag once per ImGui frame.
	class InputCapture {
	public:
		// DIMOUSESTATE / DIMOUSESTATE2: three LONG axes, then 4 or 8 button bytes.
		static constexpr uint32_t kMouseStateSize = 16;
		static constexpr uint32_t kMouseState2Size = 20;

		void SetMouseCaptured(bool captured) { mouseCaptured.store(captured, std::memory_order_relaxed); }
		[[nodiscard]] bool IsMouseCaptured() const { return mouseCaptured.load(std::memory_order_relaxed); }

		// GetDeviceState. The keyboard and joysticks share the patched vtable, hence isMouse.
		// Wheel is blanked too: scrolling a panel used to scroll the song list behind it.
		bool FilterDeviceState(bool isMouse, uint32_t size, void* state) const {
			if (!isMouse || state == nullptr || !IsMouseCaptured())
				return false;

			if (size != kMouseStateSize && size != kMouseState2Size)
				return false;

			std::memset(state, 0, size);
			return true;
		}

		// GetDeviceData. Every buffered event goes, so a press and its release never split between overlay and game.
		bool FilterDeviceData(bool isMouse, uint32_t* itemCount) const {
			if (!isMouse || itemCount == nullptr || !IsMouseCaptured())
				return false;

			*itemCount = 0;
			return true;
		}

	private:
		std::atomic<bool> mouseCaptured{ false };
	};

	inline InputCapture& Input() {
		static InputCapture instance;
		return instance;
	}
}
