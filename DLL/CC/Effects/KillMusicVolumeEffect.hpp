#pragma once

namespace CrowdControl::Effects {
	class KillMusicVolumeEffect : public CCEffect
	{
	public:
		explicit KillMusicVolumeEffect(int64_t durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "player-volume" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;
		void Run() override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;

	private:
		float oldVolume = 100.0f;
		bool ending = false;
	};
}
