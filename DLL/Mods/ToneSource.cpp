#include "../stdafx.h"
#include "ToneSource.hpp"
#include "ToneSourceAssetLoader.hpp"
#include "ToneSourceTimeline.hpp"

#include <optional>

namespace {
	using QCAutomation::ToneSourceAssetLoader::BuildTimelineFromDescriptor;
	using QCAutomation::ToneSourceAssetLoader::BuildToneBaseFallbackTimelineFromDescriptor;
	using QCAutomation::ToneSourceAssetLoader::ManifestArrangementDescriptor;
	using QCAutomation::ToneSourceAssetLoader::ResolveDescriptorForCurrentSong;
	using QCAutomation::ToneSourceTimeline::ArrangementToneTimeline;
	using QCAutomation::ToneSourceTimeline::BuildToneSlotDump;
	using QCAutomation::ToneSourceTimeline::ResolveCurrentAuthoredToneName;
	using QCAutomation::ToneSourceTimeline::ResolveResult;

	constexpr const char* kLogPrefix = "[QCAutomation] ";
	constexpr int kIdleDefaultSlot = 2;
	constexpr float kSongRestartMinPreviousTimeSec = 8.0f;
	constexpr float kSongRestartMaxCurrentTimeSec = 1.0f;
	constexpr float kSongRestartMinBackwardJumpSec = 5.0f;

	enum class EffectiveToneIntentSource {
		None,
		Auto,
		Manual,
		Idle
	};

	struct ToneSourceRuntimeState {
		bool monitoringCurrentSong_ = false;
		std::string currentSongKey_;
		std::string currentArrangementHash_;
		bool hasActiveTimeline_ = false;
		ArrangementToneTimeline activeTimeline_;

		std::optional<std::string> currentBaselineAuthoredToneName_;
		std::optional<std::string> lastLoggedBaselineAuthoredToneName_;
		std::optional<int> lastLoggedUnresolvedToneId_;
		std::optional<float> previousPlaybackTimeSec_;

		bool manualOverrideActive_ = false;
		int manualOverrideSlot_ = 0;
		std::string manualOverrideSource_;
		bool idleDefaultActive_ = false;

		EffectiveToneIntentSource lastLoggedEffectiveToneIntentSource_ = EffectiveToneIntentSource::None;
		std::string lastLoggedEffectiveToneIntentValue_;

		bool loggedMissingTimelineForCurrentArrangement_ = false;
		bool loggedTimelineLoadForCurrentArrangement_ = false;
		bool loggedIdentifierProbeForCurrentArrangement_ = false;
	};

	std::mutex toneSourceMutex;
	std::mutex toneSourceLoaderMutex;
	ToneSourceRuntimeState runtimeState;

	std::optional<int> TryMapManualSlotFromHotkey(WPARAM keyPressed) {
		switch (keyPressed) {
			case '1':
			case VK_NUMPAD1:
				return 1;
			case '2':
			case VK_NUMPAD2:
				return 2;
			case '3':
			case VK_NUMPAD3:
				return 3;
			case '4':
			case VK_NUMPAD4:
				return 4;
			default:
				return std::nullopt;
		}
	}

	const char* EffectiveToneIntentSourceToString(EffectiveToneIntentSource source) {
		switch (source) {
			case EffectiveToneIntentSource::Auto:
				return "auto";
			case EffectiveToneIntentSource::Manual:
				return "manual";
			case EffectiveToneIntentSource::Idle:
				return "idle";
			case EffectiveToneIntentSource::None:
			default:
				return "none";
		}
	}

	void MaybeLogEffectiveToneIntentChangeUnlocked(std::vector<std::string>& infoLogs) {
		EffectiveToneIntentSource source = EffectiveToneIntentSource::None;
		std::string value;

		// Effective state model:
		// 1) IdleDefaultSlot(2) when out of song
		// 2) ManualSlot(2|3|4) when in-song manual override is active
		// 3) AutoAuthored(name) from authored timeline baseline
		if (runtimeState.idleDefaultActive_) {
			source = EffectiveToneIntentSource::Idle;
			value = std::to_string(kIdleDefaultSlot);
		}
		else if (runtimeState.manualOverrideActive_) {
			source = EffectiveToneIntentSource::Manual;
			value = std::to_string(runtimeState.manualOverrideSlot_);
		}
		else if (runtimeState.currentBaselineAuthoredToneName_ && !runtimeState.currentBaselineAuthoredToneName_->empty()) {
			source = EffectiveToneIntentSource::Auto;
			value = *runtimeState.currentBaselineAuthoredToneName_;
		}

		if (source == runtimeState.lastLoggedEffectiveToneIntentSource_ && value == runtimeState.lastLoggedEffectiveToneIntentValue_) {
			return;
		}

		runtimeState.lastLoggedEffectiveToneIntentSource_ = source;
		runtimeState.lastLoggedEffectiveToneIntentValue_ = value;

		if (source == EffectiveToneIntentSource::None) {
			return;
		}

		std::ostringstream effectiveLine;
		effectiveLine << "effectiveToneIntent source=" << EffectiveToneIntentSourceToString(source)
			<< " value='" << value << "'";
		infoLogs.push_back(effectiveLine.str());
	}

	void SetManualOverrideSlotUnlocked(int slot, const char* source, std::vector<std::string>& infoLogs) {
		if (slot < 2 || slot > 4) {
			return;
		}

		if (runtimeState.manualOverrideActive_ && runtimeState.manualOverrideSlot_ == slot && runtimeState.manualOverrideSource_ == source) {
			return;
		}

		runtimeState.manualOverrideActive_ = true;
		runtimeState.manualOverrideSlot_ = slot;
		runtimeState.manualOverrideSource_ = source;
		runtimeState.idleDefaultActive_ = false;

		std::ostringstream manualLine;
		manualLine << "manualOverride slot=" << slot << " source=" << source;
		infoLogs.push_back(manualLine.str());

		MaybeLogEffectiveToneIntentChangeUnlocked(infoLogs);
	}

	void ClearManualOverrideUnlocked(const char* reason, std::vector<std::string>& infoLogs) {
		if (!runtimeState.manualOverrideActive_) {
			return;
		}

		runtimeState.manualOverrideActive_ = false;
		runtimeState.manualOverrideSlot_ = 0;
		runtimeState.manualOverrideSource_.clear();

		std::ostringstream clearLine;
		clearLine << "manualOverride cleared reason=" << reason;
		infoLogs.push_back(clearLine.str());

		MaybeLogEffectiveToneIntentChangeUnlocked(infoLogs);
	}

	bool IsLikelySongRestart(float previousPlayback, float currentPlayback) {
		return previousPlayback >= kSongRestartMinPreviousTimeSec &&
			currentPlayback >= 0.0f &&
			currentPlayback <= kSongRestartMaxCurrentTimeSec &&
			(previousPlayback - currentPlayback) >= kSongRestartMinBackwardJumpSec;
	}

	void ResetSongContextUnlocked() {
		runtimeState.currentSongKey_.clear();
		runtimeState.currentArrangementHash_.clear();
		runtimeState.hasActiveTimeline_ = false;
		runtimeState.activeTimeline_ = {};

		runtimeState.currentBaselineAuthoredToneName_.reset();
		runtimeState.lastLoggedBaselineAuthoredToneName_.reset();
		runtimeState.lastLoggedUnresolvedToneId_.reset();
		runtimeState.previousPlaybackTimeSec_.reset();

		runtimeState.loggedMissingTimelineForCurrentArrangement_ = false;
		runtimeState.loggedTimelineLoadForCurrentArrangement_ = false;
		runtimeState.loggedIdentifierProbeForCurrentArrangement_ = false;
	}

	void ResetSongTrackingUnlocked() {
		ResetSongContextUnlocked();

		runtimeState.manualOverrideActive_ = false;
		runtimeState.manualOverrideSlot_ = 0;
		runtimeState.manualOverrideSource_.clear();
		runtimeState.idleDefaultActive_ = false;

		runtimeState.lastLoggedEffectiveToneIntentSource_ = EffectiveToneIntentSource::None;
		runtimeState.lastLoggedEffectiveToneIntentValue_.clear();
	}

}

namespace QCAutomation::ToneSource {
	void Initialize() {
		// Installed asset metadata is loaded lazily on song/arrangement transitions.
	}

	void NotifyManualToneSlotHotkey(WPARAM keyPressed) {
		const auto mappedSlot = TryMapManualSlotFromHotkey(keyPressed);
		if (!mappedSlot.has_value()) {
			return;
		}

		if (Settings::ReturnSettingValue("QCAutomationEnabled") != "on") {
			return;
		}

		std::vector<std::string> infoLogs;
		{
			std::lock_guard<std::mutex> lock(toneSourceMutex);
			if (!runtimeState.monitoringCurrentSong_ || runtimeState.currentSongKey_.empty() || runtimeState.currentArrangementHash_.empty()) {
				return;
			}

			if (*mappedSlot == 1) {
				ClearManualOverrideUnlocked("slot1_return_to_auto", infoLogs);
			}
			else {
				SetManualOverrideSlotUnlocked(*mappedSlot, "hook", infoLogs);
			}
		}

		for (const std::string& infoLine : infoLogs) {
			LOG_INFO(kLogPrefix << infoLine << std::endl);
		}
	}

	void HandleInSongState() {
		const std::string songKey = GameState::GetSongKey();
		const std::string arrangementHash = GameState::GetArrangementID();
		const float playbackTimeSec = SongTimer::SongTimer();

		std::vector<std::string> infoLogs;
		std::vector<std::string> warningLogs;
		struct PendingLoadSnapshot {
			bool required = false;
			std::string songKey;
			std::string arrangementHash;
		};
		struct PendingLoadResult {
			bool descriptorResolved = false;
			ManifestArrangementDescriptor descriptor;
			std::string matchedBy;
			std::string resolveFailureReason;
			bool timelineLoaded = false;
			ArrangementToneTimeline timeline;
			std::string loadError;
			bool fallbackTimelineBuilt = false;
			ArrangementToneTimeline fallbackTimeline;
		};

		PendingLoadSnapshot pendingLoad;
		PendingLoadResult pendingLoadResult;

		auto updateRuntimeForCurrentTickUnlocked = [&]() {
			if (!pendingLoad.required &&
				runtimeState.previousPlaybackTimeSec_.has_value() &&
				IsLikelySongRestart(*runtimeState.previousPlaybackTimeSec_, playbackTimeSec)) {
				ClearManualOverrideUnlocked("song_restart", infoLogs);
			}

			if (runtimeState.hasActiveTimeline_) {
				const ResolveResult resolvedTone = ResolveCurrentAuthoredToneName(runtimeState.activeTimeline_, playbackTimeSec);
				if (resolvedTone.authoredToneName && !resolvedTone.authoredToneName->empty()) {
					runtimeState.currentBaselineAuthoredToneName_ = *resolvedTone.authoredToneName;

					if (!runtimeState.lastLoggedBaselineAuthoredToneName_ ||
						*runtimeState.lastLoggedBaselineAuthoredToneName_ != *resolvedTone.authoredToneName) {
						std::ostringstream baselineLine;
						baselineLine << "baselineAuthored t="
							<< std::fixed << std::setprecision(3) << playbackTimeSec
							<< " tone='" << *resolvedTone.authoredToneName << "'";
						infoLogs.push_back(baselineLine.str());
						runtimeState.lastLoggedBaselineAuthoredToneName_ = resolvedTone.authoredToneName;
					}

					runtimeState.lastLoggedUnresolvedToneId_.reset();
				}
				else if (resolvedTone.unresolvedToneId) {
					if (!runtimeState.lastLoggedUnresolvedToneId_ || *runtimeState.lastLoggedUnresolvedToneId_ != *resolvedTone.unresolvedToneId) {
						std::ostringstream unresolved;
						unresolved << "baselineAuthored unresolved t=" << std::fixed << std::setprecision(3) << playbackTimeSec
							<< " unresolved toneId=" << *resolvedTone.unresolvedToneId
							<< " (compiled SNG toneId does not map unambiguously to authored tone name)";
						warningLogs.push_back(unresolved.str());
						runtimeState.lastLoggedUnresolvedToneId_ = resolvedTone.unresolvedToneId;
					}
				}
			}

			MaybeLogEffectiveToneIntentChangeUnlocked(infoLogs);
			runtimeState.previousPlaybackTimeSec_ = playbackTimeSec;
		};

		{
			std::lock_guard<std::mutex> lock(toneSourceMutex);

			if (!runtimeState.monitoringCurrentSong_) {
				runtimeState.monitoringCurrentSong_ = true;
				ResetSongTrackingUnlocked();
			}

			if (songKey.empty() || arrangementHash.empty()) {
				return;
			}

			const bool changedSongContext = (songKey != runtimeState.currentSongKey_) || (arrangementHash != runtimeState.currentArrangementHash_);
			if (changedSongContext) {
				runtimeState.idleDefaultActive_ = false;
				if (!runtimeState.currentSongKey_.empty() || !runtimeState.currentArrangementHash_.empty()) {
					runtimeState.currentBaselineAuthoredToneName_.reset();
					runtimeState.lastLoggedBaselineAuthoredToneName_.reset();
					runtimeState.lastLoggedUnresolvedToneId_.reset();

					const bool arrangementChanged = (songKey == runtimeState.currentSongKey_) && (arrangementHash != runtimeState.currentArrangementHash_);
					ClearManualOverrideUnlocked(arrangementChanged ? "arrangement_change" : "reenter", infoLogs);
				}

				runtimeState.currentSongKey_ = songKey;
				runtimeState.currentArrangementHash_ = arrangementHash;
				runtimeState.hasActiveTimeline_ = false;
				runtimeState.activeTimeline_ = {};

				runtimeState.currentBaselineAuthoredToneName_.reset();
				runtimeState.lastLoggedBaselineAuthoredToneName_.reset();
				runtimeState.lastLoggedUnresolvedToneId_.reset();
				runtimeState.previousPlaybackTimeSec_.reset();

				runtimeState.lastLoggedEffectiveToneIntentSource_ = EffectiveToneIntentSource::None;
				runtimeState.lastLoggedEffectiveToneIntentValue_.clear();

				runtimeState.loggedMissingTimelineForCurrentArrangement_ = false;
				runtimeState.loggedTimelineLoadForCurrentArrangement_ = false;
				runtimeState.loggedIdentifierProbeForCurrentArrangement_ = false;

				if (!runtimeState.loggedIdentifierProbeForCurrentArrangement_) {
					std::ostringstream probe;
					probe << "Identifier probe: song=" << runtimeState.currentSongKey_
						<< " arrangementHash='" << runtimeState.currentArrangementHash_ << "'";
						infoLogs.push_back(probe.str());
					runtimeState.loggedIdentifierProbeForCurrentArrangement_ = true;
				}

				pendingLoad.required = true;
				pendingLoad.songKey = runtimeState.currentSongKey_;
				pendingLoad.arrangementHash = runtimeState.currentArrangementHash_;
			}
			else {
				updateRuntimeForCurrentTickUnlocked();
			}
		}

		if (pendingLoad.required) {
			{
				std::lock_guard<std::mutex> loaderLock(toneSourceLoaderMutex);
				if (!ResolveDescriptorForCurrentSong(
					pendingLoad.songKey,
					pendingLoad.arrangementHash,
					pendingLoadResult.descriptor,
					pendingLoadResult.matchedBy,
					pendingLoadResult.resolveFailureReason)) {
					pendingLoadResult.descriptorResolved = false;
				}
				else {
					pendingLoadResult.descriptorResolved = true;
					if (BuildTimelineFromDescriptor(pendingLoadResult.descriptor, pendingLoadResult.timeline, pendingLoadResult.loadError)) {
						pendingLoadResult.timelineLoaded = true;
					}
					else {
						pendingLoadResult.timelineLoaded = false;
						pendingLoadResult.fallbackTimelineBuilt = BuildToneBaseFallbackTimelineFromDescriptor(
							pendingLoadResult.descriptor,
							pendingLoadResult.fallbackTimeline);
					}
				}
			}

				{
					std::lock_guard<std::mutex> lock(toneSourceMutex);
					if (runtimeState.currentSongKey_ != pendingLoad.songKey ||
						runtimeState.currentArrangementHash_ != pendingLoad.arrangementHash) {
						// Song/arrangement changed while load work was in flight; discard stale result and
						// suppress pre-load context logs gathered by this call for the now-stale context.
						warningLogs.clear();
						infoLogs.clear();
					}
					else {
					if (!pendingLoadResult.descriptorResolved) {
						if (!runtimeState.loggedMissingTimelineForCurrentArrangement_) {
							warningLogs.push_back("No installed arrangement tone metadata: " + pendingLoadResult.resolveFailureReason);
							runtimeState.loggedMissingTimelineForCurrentArrangement_ = true;
						}
					}
					else if (!pendingLoadResult.timelineLoaded) {
						if (!runtimeState.loggedMissingTimelineForCurrentArrangement_) {
							std::ostringstream loadFailed;
							loadFailed << "Failed to load arrangement tone timeline from installed assets: song=" << runtimeState.currentSongKey_
								<< " arrangementHash='" << runtimeState.currentArrangementHash_
								<< "' matchedBy=" << pendingLoadResult.matchedBy
								<< " psarc='" << pendingLoadResult.descriptor.psarcPath.string()
								<< "' sngPath='" << pendingLoadResult.descriptor.sngPath
								<< "' reason=" << pendingLoadResult.loadError;
							warningLogs.push_back(loadFailed.str());
							runtimeState.loggedMissingTimelineForCurrentArrangement_ = true;
						}

						if (pendingLoadResult.fallbackTimelineBuilt) {
							runtimeState.activeTimeline_ = std::move(pendingLoadResult.fallbackTimeline);
							runtimeState.hasActiveTimeline_ = true;

							if (!runtimeState.loggedTimelineLoadForCurrentArrangement_) {
								std::ostringstream fallbackLine;
								fallbackLine << "Using toneBase-only fallback (no timeline events): song=" << runtimeState.currentSongKey_
									<< " arrangementHash='" << runtimeState.currentArrangementHash_
									<< "' matchedBy=" << pendingLoadResult.matchedBy;
								infoLogs.push_back(fallbackLine.str());
								infoLogs.push_back(BuildToneSlotDump(runtimeState.activeTimeline_));
								runtimeState.loggedTimelineLoadForCurrentArrangement_ = true;
							}
						}
					}
					else {
						runtimeState.activeTimeline_ = std::move(pendingLoadResult.timeline);
						runtimeState.hasActiveTimeline_ = true;

						if (!runtimeState.loggedTimelineLoadForCurrentArrangement_) {
							std::ostringstream loadOk;
							loadOk << "Loaded arrangement tone timeline from installed assets: song=" << runtimeState.currentSongKey_
								<< " arrangementHash='" << runtimeState.currentArrangementHash_
								<< "' matchedBy=" << pendingLoadResult.matchedBy
								<< " metadataArrangementID='" << runtimeState.activeTimeline_.arrangementID
								<< "' metadataPersistentID='" << runtimeState.activeTimeline_.persistentID
								<< "' psarc='" << pendingLoadResult.descriptor.psarcPath.string()
								<< "' sngPath='" << pendingLoadResult.descriptor.sngPath
								<< "' events=" << runtimeState.activeTimeline_.toneEvents.size();
							infoLogs.push_back(loadOk.str());
							infoLogs.push_back(BuildToneSlotDump(runtimeState.activeTimeline_));

							if (runtimeState.activeTimeline_.ambiguousToneIdZeroCount > 0) {
								std::ostringstream ambiguity;
								ambiguity << "Compiled SNG fidelity limit: toneId=0 is ambiguous between ToneBase and ToneA for this arrangement"
									<< " (events=" << runtimeState.activeTimeline_.ambiguousToneIdZeroCount << ").";
								warningLogs.push_back(ambiguity.str());
							}

							if (runtimeState.activeTimeline_.ignoredOutOfRangeToneIdCount > 0) {
								std::ostringstream ignored;
								ignored << "Ignored out-of-range toneId events while parsing SNG timeline: "
									<< runtimeState.activeTimeline_.ignoredOutOfRangeToneIdCount;
								warningLogs.push_back(ignored.str());
							}

							runtimeState.loggedTimelineLoadForCurrentArrangement_ = true;
						}
					}

					updateRuntimeForCurrentTickUnlocked();
				}
			}
		}

		for (const std::string& warningLine : warningLogs) {
			LOG_WARNING(kLogPrefix << warningLine << std::endl);
		}
		for (const std::string& infoLine : infoLogs) {
			LOG_INFO(kLogPrefix << infoLine << std::endl);
		}
	}

	void HandleOutOfSongState() {
		std::vector<std::string> infoLogs;
		{
			std::lock_guard<std::mutex> lock(toneSourceMutex);
			// Clear authored baseline before removing manual override so song-exit transitions do not emit an intermediate
			// effective auto intent from stale in-song data.
			runtimeState.currentBaselineAuthoredToneName_.reset();
			runtimeState.lastLoggedBaselineAuthoredToneName_.reset();
			runtimeState.lastLoggedUnresolvedToneId_.reset();

			if (runtimeState.monitoringCurrentSong_) {
				ClearManualOverrideUnlocked("reenter", infoLogs);
			}

			runtimeState.monitoringCurrentSong_ = false;
			ResetSongContextUnlocked();
			runtimeState.idleDefaultActive_ = true;
			MaybeLogEffectiveToneIntentChangeUnlocked(infoLogs);
		}

		for (const std::string& infoLine : infoLogs) {
			LOG_INFO(kLogPrefix << infoLine << std::endl);
		}
	}

	bool TryGetCurrentToneSlot(int& outToneSlot) {
		std::lock_guard<std::mutex> lock(toneSourceMutex);
		if (!runtimeState.manualOverrideActive_) {
			return false;
		}

		outToneSlot = runtimeState.manualOverrideSlot_;
		return true;
	}

	std::optional<std::string> GetCurrentAuthoredToneName(float playbackTimeSec) {
		std::lock_guard<std::mutex> lock(toneSourceMutex);
		if (!runtimeState.hasActiveTimeline_) {
			return std::nullopt;
		}

		return ResolveCurrentAuthoredToneName(runtimeState.activeTimeline_, playbackTimeSec).authoredToneName;
	}

	bool TryGetCurrentAuthoredToneName(std::string& outToneName) {
		const auto currentToneName = GetCurrentAuthoredToneName(SongTimer::SongTimer());
		if (!currentToneName) {
			return false;
		}

		outToneName = *currentToneName;
		return true;
	}
}
