#pragma once

#include "../Framework/Framework.hpp"

class ShowCurrentNoteMod : public Framework::IMod {
public:
	MOD_ID(ShowCurrentNoteMod)
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnSongTick(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;

private:
	void Publish(Framework::ModContext& c, bool inSong) const;
};
