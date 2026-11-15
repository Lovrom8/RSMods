#pragma once

#include "../Framework/Framework.hpp"

#include <cstdint>
#include <vector>

// Fills in the release date the song lists sort and label by, when the game has none.
// Official DLC gets its US release date and custom songs the date their file was last changed.
// The game only gets official dates from Ubisoft's servers, and shows a song without a date as "NEW RELEASE".
class SongDatesMod : public Framework::IMod {
public:
	MOD_ID(SongDatesMod)
	Framework::SettingDefs Settings() const override;
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;

private:
	struct PlacedHook {
		uintptr_t address = 0;
		std::vector<unsigned char> original;
	};

	bool PlaceHooks();
	bool PlaceHook(uintptr_t address, void* hook, const unsigned char* bytes, size_t size);
	void RemoveHooks();

	std::vector<PlacedHook> placed;
};

