#pragma once

#include "../Framework/Framework.hpp"

class UltrawideMod : public Framework::IMod {
public:
	MOD_ID(UltrawideMod)
	Framework::SettingDefs Settings() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;
	std::vector<std::string_view> ClaimsExclusive() const override {
		return { "loft-post-effects-state" };
	}

	void OnInitialize(Framework::ModContext& c) override;
	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnTick(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

private:
	void SetPatched(bool enable);

	uintptr_t operandAddress = 0;
	uint32_t originalOperand = 0;
	bool located = false;
	bool patched = false;
};
