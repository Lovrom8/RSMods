#pragma once

#include "../Framework/Framework.hpp"

// Startup speedups are enabled together; setting changes apply on the next launch.
class UltrafastLoadMod : public Framework::IMod {
public:
	MOD_ID(UltrafastLoadMod)
	bool IsEnabled(const Framework::ModContext&) const override { return false; }
	Framework::SettingDefs Settings() const override;
};

namespace UltrafastLoad {
	// Read once at DLL attach, before the settings store is available.
	bool Enabled();
}
