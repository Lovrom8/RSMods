#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace QCAutomation::ToneSourceTimeline {
	struct ToneEvent {
		float timeSec = 0.0f;
		std::optional<std::string> authoredToneName;
		std::optional<int> toneIdDebug;
		bool toneIdZeroAmbiguous = false;
	};

	struct ArrangementToneTimeline {
		std::string songKey;
		std::string arrangementID;
		std::string persistentID;
		std::string arrangementIDNormalized;
		std::string persistentIDNormalized;

		std::string toneBase;
		std::string toneA;
		std::string toneB;
		std::string toneC;
		std::string toneD;

		std::vector<ToneEvent> toneEvents;
		std::size_t ignoredOutOfRangeToneIdCount = 0;
		std::size_t unresolvedToneNameCount = 0;
		std::size_t ambiguousToneIdZeroCount = 0;
	};

	struct ResolveResult {
		std::optional<std::string> authoredToneName;
		std::string source = "tonebase";
		std::optional<int> unresolvedToneId;
	};

	std::optional<std::string> ResolveToneNameFromToneId(
		const ArrangementToneTimeline& timeline,
		int toneId,
		bool& outAmbiguousToneIdZero);
	ResolveResult ResolveCurrentAuthoredToneName(
		const ArrangementToneTimeline& timeline,
		float playbackTimeSec);
	std::string BuildToneSlotDump(const ArrangementToneTimeline& timeline);
}
