// DON'T TOUCH THIS FILE!!!!!! IT IS NEEDED FOR ROCKSMITH TO RUN!
#include "windows.h"

namespace Proxy {
	bool Init();
    void Shutdown();
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
