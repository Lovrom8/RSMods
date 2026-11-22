#pragma once

#include <optional>
#include <string>

namespace GameNotification {
	// Numeric message types are part of Rocksmith's UI protocol, and title/body semantics and queue
	// styles differ per type. Types seen on screen:
	//   0x01 = left-side LAS song-title plate (title and body).
	//   0x10 = lower-left subtitle strip (body only, the title is not drawn).
	//   0x11 = upper-right news panel with an icon (free header and body, queued one after another).
	struct Handle {
		std::string instanceKey;
		int messageType{};

		explicit operator bool() const noexcept { return !instanceKey.empty(); }
	};

	// UI/main thread only. A positive duration is required; Rocksmith interprets
	// zero as effectively permanent. Unsupported executables and unavailable UI
	// state fail closed.
	std::optional<Handle> Show(const char* title, const char* body,
		float seconds, int messageType, int playerIndex = 0);

	// Dismiss only zeroes the remaining-duration timer, so the notification plays its normal expiry
	// animation and stays active for a few frames afterwards. IsActive reports whether the game still
	// holds it.
	bool Dismiss(const Handle& handle);
	std::optional<bool> IsActive(const Handle& handle);

	inline std::optional<Handle> Show(const char* body, float seconds,
		int messageType, int playerIndex = 0) {
		return Show("", body, seconds, messageType, playerIndex);
	}
}
