#pragma once

#include "ToneSourceTimeline.hpp"

#include <filesystem>
#include <string>

namespace QCAutomation::ToneSourceAssetLoader {
	// Internal ToneSource module contract:
	// - Callers must serialize access (ToneSource currently does this via toneSourceMutex).
	// - Functions here may mutate process-lifetime caches and are not thread-safe on their own.
	struct ManifestArrangementDescriptor {
		std::filesystem::path psarcPath;
		std::string songKey;
		std::string arrangementID;
		std::string persistentID;
		std::string arrangementName;
		std::string arrangementType;
		std::string songXmlPath;
		std::string arrangementIDNormalized;
		std::string persistentIDNormalized;
		std::string toneBase;
		std::string toneA;
		std::string toneB;
		std::string toneC;
		std::string toneD;
		std::string sngPath;
	};

	bool ResolveDescriptorForCurrentSong(
		const std::string& songKey,
		const std::string& arrangementHash,
		ManifestArrangementDescriptor& outDescriptor,
		std::string& outMatchedBy,
		std::string& outFailureReason);

	bool BuildToneBaseFallbackTimelineFromDescriptor(
		const ManifestArrangementDescriptor& descriptor,
		ToneSourceTimeline::ArrangementToneTimeline& outTimeline);

	bool BuildTimelineFromDescriptor(
		const ManifestArrangementDescriptor& descriptor,
		ToneSourceTimeline::ArrangementToneTimeline& outTimeline,
		std::string& outError);
}
