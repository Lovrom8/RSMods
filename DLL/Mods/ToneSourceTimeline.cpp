#include "../stdafx.h"
#include "ToneSourceTimeline.hpp"

namespace QCAutomation::ToneSourceTimeline {
	std::optional<std::string> ResolveToneNameFromToneId(
		const ArrangementToneTimeline& timeline,
		int toneId,
		bool& outAmbiguousToneIdZero) {
		outAmbiguousToneIdZero = false;

		switch (toneId) {
			case 0:
				// Compiled SNG timeline only gives toneId. In RS2014 assets toneId=0 can map to ToneBase or ToneA.
				if (!timeline.toneA.empty() && !timeline.toneBase.empty() && timeline.toneA != timeline.toneBase) {
					outAmbiguousToneIdZero = true;
					return std::nullopt;
				}
				if (!timeline.toneA.empty()) return timeline.toneA;
				if (!timeline.toneBase.empty()) return timeline.toneBase;
				return std::nullopt;
			case 1:
				if (!timeline.toneB.empty()) return timeline.toneB;
				return std::nullopt;
			case 2:
				if (!timeline.toneC.empty()) return timeline.toneC;
				return std::nullopt;
			case 3:
				if (!timeline.toneD.empty()) return timeline.toneD;
				return std::nullopt;
			default:
				return std::nullopt;
		}
	}

	ResolveResult ResolveCurrentAuthoredToneName(const ArrangementToneTimeline& timeline, float playbackTimeSec) {
		ResolveResult result;

		const ToneEvent* selectedEvent = nullptr;
		for (const ToneEvent& toneEvent : timeline.toneEvents) {
			if (toneEvent.timeSec <= playbackTimeSec) {
				selectedEvent = &toneEvent;
			}
			else {
				break;
			}
		}

		if (!selectedEvent) {
			result.source = "tonebase";
			if (!timeline.toneBase.empty()) {
				result.authoredToneName = timeline.toneBase;
			}
			return result;
		}

		result.source = "timeline";
		if (selectedEvent->authoredToneName && !selectedEvent->authoredToneName->empty()) {
			result.authoredToneName = *selectedEvent->authoredToneName;
			return result;
		}

		if (selectedEvent->toneIdDebug) {
			result.unresolvedToneId = *selectedEvent->toneIdDebug;
		}
		return result;
	}

	std::string BuildToneSlotDump(const ArrangementToneTimeline& timeline) {
		std::ostringstream oss;
		oss << "Tone slots: toneBase='" << timeline.toneBase
			<< "' toneA='" << timeline.toneA
			<< "' toneB='" << timeline.toneB
			<< "' toneC='" << timeline.toneC
			<< "' toneD='" << timeline.toneD << "'";
		return oss.str();
	}
}
