// DON'T TOUCH THIS FILE!!!!!! IT IS NEEDED FOR ROCKSMITH TO RUN!

#include <mutex>
#include <utility>

namespace Proxy {
	namespace detail {
		template <typename Loader>
		bool InitializeOnce(std::once_flag& once, bool& initialized, Loader&& loader) {
			std::call_once(once, [&] { initialized = static_cast<bool>(std::forward<Loader>(loader)()); });
			return initialized;
		}
	}

	bool Init();
}

struct XINPUT_BATTERY_INFORMATION {
	BYTE BatteryType;
	BYTE BatteryLevel;
};

struct XINPUT_GAMEPAD {
	WORD  wButtons;
	BYTE  bLeftTrigger;
	BYTE  bRightTrigger;
	SHORT sThumbLX;
	SHORT sThumbLY;
	SHORT sThumbRX;
	SHORT sThumbRY;
};

struct XINPUT_VIBRATION {
	WORD wLeftMotorSpeed;
	WORD wRightMotorSpeed;
};

struct XINPUT_CAPABILITIES {
	BYTE             Type;
	BYTE             SubType;
	WORD             Flags;
	XINPUT_GAMEPAD   Gamepad;
	XINPUT_VIBRATION Vibration;
};

struct XINPUT_KEYSTROKE {
	WORD  VirtualKey;
	WCHAR Unicode;
	WORD  Flags;
	BYTE  UserIndex;
	BYTE  HidCode;
};

struct XINPUT_STATE {
	DWORD          dwPacketNumber;
	XINPUT_GAMEPAD Gamepad;
};
