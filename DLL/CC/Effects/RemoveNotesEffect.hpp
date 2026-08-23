#pragma once

namespace CrowdControl::Effects {
	// Prevents the game from drawing note head meshes
	class RemoveNotesEffect : public CCEffect
	{
	public:
		explicit RemoveNotesEffect(unsigned int durationMilliseconds) {
			duration_ms = durationMilliseconds;
		}

		std::vector<std::string> ClaimsExclusive() const override
		{
			return { "note-visuals" };
		}

		Enums::EffectStatus Test(const Structs::Request& request) override;

	protected:
		Enums::EffectStatus OnStart(const Structs::Request& request) override;
		Enums::EffectStatus OnStop() override;

	private:
		static void ScaleNotes(float scale);
	};
}
