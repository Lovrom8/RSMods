#pragma once

#include "../Framework/Framework.hpp"

class UltrawideMod : public Framework::IMod {
public:
	MOD_ID(UltrawideMod)
	std::vector<std::string_view> ClaimsExclusive() const override {
		return { "loft-post-effects-state" };
	}

	void OnInitialize(Framework::ModContext& c) override;
	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnTick(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

private:
	// Patches the game's aspect load once; never undone, so no toggle rewrites live code.
	void EnsurePatched();

	uintptr_t operandAddress = 0;
	bool patched = false;
};
