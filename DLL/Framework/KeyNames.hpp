#pragma once

#include <string_view>

namespace Framework {
	// The Windows virtual-key code for a name RSMods.ini stores a key bind as ("A", "VK_F9"), 0 if it isn't one.
	// Key binds use these codes on every platform; a host maps its own key codes onto them.
	unsigned int VirtualKeyFromName(std::string_view name);
}
