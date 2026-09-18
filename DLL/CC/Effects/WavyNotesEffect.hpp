#pragma once

namespace CrowdControl::Effects {
	struct Vec3 {
		float z;
		float x;
		float y;
	};

	inline Vec3* vec;
	inline bool wavyNotesEnabled = false;

	void NotePositionHook();

	class WavyNotesEffect : public CCEffect
	{
	public:
		explicit WavyNotesEffect(int64_t durationMilliseconds)
		{
			duration_ms = durationMilliseconds;

			MemUtil::PlaceHook(Offsets::ptr_wavyNotesHook, NotePositionHook, 41);
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "note-visuals" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;
	};
}
