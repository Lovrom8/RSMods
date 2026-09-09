#pragma once

#include "../Framework/Framework.hpp"

class SongAccuracyMod : public Framework::IMod {
public:
	MOD_ID(SongAccuracyMod)
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnSongTick(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;

private:
	static float ReadAccuracy();
	void Publish(Framework::ModContext& c) const;
};
