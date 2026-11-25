#pragma once

#include "../Framework/Framework.hpp"

// Groups custom songs by package author inside the stock SOURCE sort, with a native heading per author.
// The author comes from each custom package's toolkit.version; official songs are grouped by source.
class SongSourceAuthorGroupsMod : public Framework::IMod {
public:
	MOD_ID(SongSourceAuthorGroupsMod)
	Framework::SettingDefs Settings() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;

private:
	bool installed = false;
};
