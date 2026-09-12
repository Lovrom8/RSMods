#pragma once

#include "../Framework/Framework.hpp"

class UltrawideMod : public Framework::IMod {
public:
	MOD_ID(UltrawideMod)
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;

	static void SetDisplayAspect(double aspect);

private:
	void SetPatched(bool enable);

	uintptr_t operandAddress = 0;
	uint32_t originalOperand = 0;
	bool located = false;
	bool patched = false;
};
