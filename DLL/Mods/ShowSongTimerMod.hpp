#pragma once

#include "../Framework/Framework.hpp"

class ShowSongTimerMod : public Framework::IMod {
public:
	MOD_ID(ShowSongTimerMod)
	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnInitialize(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;
	void OnSongEnter(Framework::ModContext& c) override;
	void OnSongExit(Framework::ModContext& c) override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;

private:
	bool shown = false;     // is the timer currently up (toggled by the key, driven by song edges in automatic mode)
	bool automatic = false; // last observed ShowSongTimerWhen == Automatic

	static bool IsAutomatic(const Framework::ModContext& c);
	void Publish(Framework::ModContext& c) const;
};
