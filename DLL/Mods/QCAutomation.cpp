#include "../stdafx.h"
#include "QCAutomation.hpp"
#include "Midi.hpp"
#include "ToneSource.hpp"
#include "../Settings.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <optional>
#include <string_view>
#include <vector>

namespace {
	constexpr const char* kLogPrefix = "[QCAutomation] ";
	constexpr int kQcMyPresetsSetlistCc32 = 1; // Quad Cortex: CC#32=1 selects My Presets setlist.
	constexpr int kDefaultSceneCc = 43;
	constexpr int kMinSongTransposeSemitone = -7;
	constexpr int kMaxSongTransposeSemitone = 0;
	constexpr float kSongRestartMinPreviousTimeSec = 8.0f;
	constexpr float kSongRestartMaxCurrentTimeSec = 1.0f;
	constexpr float kSongRestartMinBackwardJumpSec = 5.0f;
	constexpr float kDeferredSceneMinPlaybackTimeSec = 0.150f;

	enum class AutoToneBucket {
		Clean,
		Od,
		Dist,
		Mod,
		Solo,
		BassIgnore
	};

	enum class EffectiveIntentSource {
		Auto,
		Manual,
		Idle
	};

	enum class QcTarget {
		AutoClean,
		AutoOd,
		AutoDist,
		AutoMod,
		AutoSolo,
		Manual2,
		Manual3,
		Manual4,
		Idle
	};

	struct QcTargetInfo {
		QcTarget target;
		const char* roleLabel;
		const char* settingKey;
		const char* defaultTarget;
	};

	struct MidiRecallTarget {
		std::string targetSpec;
		int cc32 = kQcMyPresetsSetlistCc32;
		int cc0 = 0;
		int program = 0;
	};

	struct TargetDecision {
		QcTarget target = QcTarget::AutoClean;
		EffectiveIntentSource source = EffectiveIntentSource::Auto;
		std::string sourceValue;
		std::optional<AutoToneBucket> classifierBucket;
		std::string matchedKeyword;
		std::string authoredToneName;
	};

	enum class SongTransposeResult {
		Resolved,
		Retry,
		Unsupported
	};

	struct SongTransposeDecision {
		SongTransposeResult result = SongTransposeResult::Retry;
		int semitones = 0;
		std::string detail;
	};

	constexpr std::array<QcTargetInfo, 9> kQcTargets{
		QcTargetInfo{ QcTarget::AutoClean, "AUTO_CLEAN", "QCAutomationAutoCleanTarget", "MyPresets:32A" },
		QcTargetInfo{ QcTarget::AutoOd, "AUTO_OD", "QCAutomationAutoODTarget", "MyPresets:32B" },
		QcTargetInfo{ QcTarget::AutoDist, "AUTO_DIST", "QCAutomationAutoDistTarget", "MyPresets:32C" },
		QcTargetInfo{ QcTarget::AutoMod, "AUTO_MOD", "QCAutomationAutoModTarget", "MyPresets:32D" },
		QcTargetInfo{ QcTarget::AutoSolo, "AUTO_SOLO", "QCAutomationAutoSoloTarget", "MyPresets:32E" },
		QcTargetInfo{ QcTarget::Manual2, "MANUAL_2", "QCAutomationManual2Target", "MyPresets:32F" },
		QcTargetInfo{ QcTarget::Manual3, "MANUAL_3", "QCAutomationManual3Target", "MyPresets:32G" },
		QcTargetInfo{ QcTarget::Manual4, "MANUAL_4", "QCAutomationManual4Target", "MyPresets:32H" },
		QcTargetInfo{ QcTarget::Idle, "IDLE", "QCAutomationIdleTarget", "MyPresets:32F" }
	};

	bool loggedMissingMidiDevice = false;
	bool loggedConfiguredMidiDeviceNotFound = false;
	std::string lastSelectedMidiOutDevice;
	bool lastSelectedMidiOutDeviceResolved = false;

	std::optional<AutoToneBucket> lastLoggedClassifierBucket;
	std::string lastLoggedClassifierKeyword;
	std::string lastLoggedClassifierToneName;
	std::optional<QcTarget> lastLoggedTarget;
	std::string lastLoggedTargetSpec;
	std::optional<QcTarget> lastSentTarget;
	std::string lastSentTargetSpec;
	std::array<std::string, kQcTargets.size()> lastInvalidTargetSpecByIndex;
	std::optional<int> lastSentSceneValue;
	std::optional<int> activeSongSceneValue;
	bool queuedSongSceneSend = false;
	bool deferredSongSceneSent = false;
	bool retrySongSceneSendPending = false;
	bool retrySongSceneSendAttempted = false;
	std::string queuedSongSceneReason;
	std::string activeSongKey;
	std::string activeArrangementId;
	bool transposeNeedsResolve = false;
	std::string transposeResolveReason;
	bool loggedPendingTransposeResolve = false;
	std::string lastInvalidIdleSceneSetting;
	std::string lastInvalidOutOfRangeBehaviorSetting;
	bool transposeStateWasInSong = false;
	std::optional<float> previousInSongPlaybackTimeSec;

	int ClampMidi7Bit(int value) {
		if (value < 0) return 0;
		if (value > 127) return 127;
		return value;
	}

	int ClampMidiChannel(int channel) {
		if (channel < 0) return 0;
		if (channel > 15) return 15;
		return channel;
	}

	const QcTargetInfo& GetQcTargetInfo(QcTarget target) {
		for (const QcTargetInfo& info : kQcTargets) {
			if (info.target == target) {
				return info;
			}
		}

		return kQcTargets[0];
	}

	const char* ToString(AutoToneBucket bucket) {
		switch (bucket) {
			case AutoToneBucket::Clean:
				return "CLEAN";
			case AutoToneBucket::Od:
				return "OD";
			case AutoToneBucket::Dist:
				return "DIST";
			case AutoToneBucket::Mod:
				return "MOD";
			case AutoToneBucket::Solo:
				return "SOLO";
			case AutoToneBucket::BassIgnore:
				return "BASS_IGNORE";
			default:
				return "UNKNOWN";
		}
	}

	const char* ToString(EffectiveIntentSource source) {
		switch (source) {
			case EffectiveIntentSource::Auto:
				return "auto";
			case EffectiveIntentSource::Manual:
				return "manual";
			case EffectiveIntentSource::Idle:
				return "idle";
			default:
				return "unknown";
		}
	}

	char SceneValueToLetter(int sceneValue) {
		const int clamped = std::clamp(sceneValue, 0, 7);
		return static_cast<char>('A' + clamped);
	}

	bool TryParseSceneValueFromLetter(std::string_view rawValue, int& outSceneValue) {
		size_t begin = 0;
		size_t end = rawValue.size();
		while (begin < end && std::isspace(static_cast<unsigned char>(rawValue[begin]))) {
			++begin;
		}
		while (end > begin && std::isspace(static_cast<unsigned char>(rawValue[end - 1]))) {
			--end;
		}
		if (begin >= end) {
			return false;
		}

		const char sceneLetter = static_cast<char>(std::toupper(static_cast<unsigned char>(rawValue[begin])));
		if (sceneLetter < 'A' || sceneLetter > 'H') {
			return false;
		}

		outSceneValue = sceneLetter - 'A';
		return true;
	}

	std::string ToLowerAscii(std::string_view input) {
		std::string lowered;
		lowered.reserve(input.size());

		for (const char c : input) {
			lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		}

		return lowered;
	}

	std::string TrimAscii(std::string_view input) {
		size_t begin = 0;
		size_t end = input.size();
		while (begin < end && std::isspace(static_cast<unsigned char>(input[begin]))) {
			++begin;
		}
		while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
			--end;
		}

		return std::string(input.substr(begin, end - begin));
	}

	bool IsLikelySongRestart(float previousPlayback, float currentPlayback) {
		return previousPlayback >= kSongRestartMinPreviousTimeSec &&
			currentPlayback >= 0.0f &&
			currentPlayback <= kSongRestartMaxCurrentTimeSec &&
			(previousPlayback - currentPlayback) >= kSongRestartMinBackwardJumpSec;
	}

	int GetSceneCcNumber() {
		const int configured = Settings::GetModSetting("QCAutomationSceneCC");
		if (configured == 0) {
			return kDefaultSceneCc;
		}
		return ClampMidi7Bit(configured);
	}

	std::string GetOutOfRangeBehavior() {
		std::string configured = ToLowerAscii(TrimAscii(Settings::ReturnSettingValue("QCAutomationTransposeOutOfRange")));
		if (configured != "clamp" && configured != "skip") {
			if (lastInvalidOutOfRangeBehaviorSetting != configured) {
				lastInvalidOutOfRangeBehaviorSetting = configured;
				LOG_WARNING(kLogPrefix << "Invalid QCAutomationTransposeOutOfRange='" << configured
					<< "'. Falling back to 'clamp'." << std::endl);
			}
			return "clamp";
		}

		lastInvalidOutOfRangeBehaviorSetting.clear();
		return configured;
	}

	int GetIdleSceneValue() {
		const std::string configured = Settings::ReturnSettingValue("QCAutomationIdleScene");
		int parsedSceneValue = 0;
		if (TryParseSceneValueFromLetter(configured, parsedSceneValue)) {
			lastInvalidIdleSceneSetting.clear();
			return parsedSceneValue;
		}

		if (lastInvalidIdleSceneSetting != configured) {
			lastInvalidIdleSceneSetting = configured;
			LOG_WARNING(kLogPrefix << "Invalid QCAutomationIdleScene='" << configured
				<< "'. Falling back to 'A'." << std::endl);
		}
		return 0;
	}

	std::vector<std::string> ParseCsvKeywords(const std::string& csv) {
		std::vector<std::string> parsed;
		size_t start = 0;
		while (start <= csv.size()) {
			const size_t comma = csv.find(',', start);
			const size_t end = comma == std::string::npos ? csv.size() : comma;
			std::string token = TrimAscii(std::string_view(csv).substr(start, end - start));
			token = ToLowerAscii(token);
			if (!token.empty()) {
				parsed.push_back(std::move(token));
			}

			if (comma == std::string::npos) {
				break;
			}

			start = comma + 1;
		}

		return parsed;
	}

	const std::vector<std::string>& GetKeywordList(
		const char* settingKey,
		const char* defaultCsv,
		std::string& ioCachedRaw,
		std::vector<std::string>& ioCachedParsed) {
		std::string csv = Settings::ReturnSettingValue(settingKey);
		if (csv.empty()) {
			csv = defaultCsv;
		}

		if (csv != ioCachedRaw) {
			ioCachedRaw = csv;
			ioCachedParsed = ParseCsvKeywords(csv);
		}

		return ioCachedParsed;
	}

	bool MatchKeywordGroup(
		std::string_view normalizedToneName,
		const std::vector<std::string>& keywords,
		std::string& outMatchedKeyword) {
		for (const std::string& keywordLower : keywords) {
			if (keywordLower.empty()) {
				continue;
			}

			size_t searchOffset = 0;
			while (searchOffset < normalizedToneName.size()) {
				const size_t pos = normalizedToneName.find(keywordLower, searchOffset);
				if (pos == std::string_view::npos) {
					break;
				}

				// Prevent short OD token from stealing obvious MOD-like names (e.g. "mod", "leadmod").
				// Keep broad substring behavior for all other cases.
				if (keywordLower == "od" && pos > 0 && normalizedToneName[pos - 1] == 'm') {
					searchOffset = pos + 1;
					continue;
				}

				outMatchedKeyword = keywordLower;
				return true;
			}
		}

		return false;
	}

	constexpr const char* kDefaultSoloKeywords = "lead,solo";
	constexpr const char* kDefaultDistKeywords = "dist,distortion,fuzz,gain,higain,highgain,dis";
	constexpr const char* kDefaultOdKeywords = "overdrive,od,drive,crunch,dirty,breakup,over";
	constexpr const char* kDefaultCleanKeywords = "clean,acoustic,acous,acc,twang,chime,sparkle";
	constexpr const char* kDefaultModKeywords = "wah,chorus,verb,reverb,delay,echo,trem,tremolo,phase,phaser,flange,flanger,filter,mod,fx,ambient,synth,8va,oct,octave,sitar";
	constexpr int kMinClassifierPriority = 1;
	constexpr int kMaxClassifierPriority = 5;

	std::string cachedSoloKeywordsRaw;
	std::string cachedDistKeywordsRaw;
	std::string cachedOdKeywordsRaw;
	std::string cachedCleanKeywordsRaw;
	std::string cachedModKeywordsRaw;
	std::string cachedSoloPriorityRaw;
	std::string cachedDistPriorityRaw;
	std::string cachedOdPriorityRaw;
	std::string cachedCleanPriorityRaw;
	std::string cachedModPriorityRaw;
	std::vector<std::string> cachedSoloKeywords;
	std::vector<std::string> cachedDistKeywords;
	std::vector<std::string> cachedOdKeywords;
	std::vector<std::string> cachedCleanKeywords;
	std::vector<std::string> cachedModKeywords;
	int cachedSoloPriority = 5;
	int cachedDistPriority = 2;
	int cachedOdPriority = 1;
	int cachedCleanPriority = 3;
	int cachedModPriority = 4;

	struct ClassifierMatchRule {
		AutoToneBucket bucket = AutoToneBucket::Clean;
		const std::vector<std::string>* keywords = nullptr;
		int priority = kMaxClassifierPriority;
		int fallbackOrder = 0;
	};

	int ParseClassifierPriority(const std::string& rawValue, int defaultPriority) {
		int parsedPriority = defaultPriority;
		try {
			const std::string trimmed = TrimAscii(rawValue);
			if (!trimmed.empty()) {
				size_t parsedLength = 0;
				parsedPriority = std::stoi(trimmed, &parsedLength, 10);
				if (parsedLength != trimmed.size()) {
					parsedPriority = defaultPriority;
				}
			}
		}
		catch (...) {
			parsedPriority = defaultPriority;
		}

		return std::clamp(parsedPriority, kMinClassifierPriority, kMaxClassifierPriority);
	}

	int GetClassifierPriority(
		const char* settingKey,
		int defaultPriority,
		std::string& ioCachedRaw,
		int& ioCachedParsed) {
		std::string configured = Settings::ReturnSettingValue(settingKey);
		if (configured.empty()) {
			configured = std::to_string(defaultPriority);
		}

		if (configured != ioCachedRaw) {
			ioCachedRaw = configured;
			ioCachedParsed = ParseClassifierPriority(configured, defaultPriority);
		}

		return ioCachedParsed;
	}

	AutoToneBucket ClassifyAutoToneBucket(const std::string& authoredToneName, std::string& outMatchedKeyword) {
		// Lean substring classifier:
		// - full-name lowercase normalization
		// - case-insensitive substring matching
		// - configurable priority order (1 = highest).
		const std::string normalized = ToLowerAscii(authoredToneName);

		const bool ignoreBass = Settings::ReturnSettingValue("QCAutomationIgnoreBass") != "off";
		if (ignoreBass && normalized.find("bass") != std::string::npos) {
			outMatchedKeyword = "bass";
			return AutoToneBucket::BassIgnore;
		}

		const std::vector<std::string>& soloKeywords = GetKeywordList(
			"QCAutomationSoloKeywords",
			kDefaultSoloKeywords,
			cachedSoloKeywordsRaw,
			cachedSoloKeywords
		);
		const std::vector<std::string>& distKeywords = GetKeywordList(
			"QCAutomationDistKeywords",
			kDefaultDistKeywords,
			cachedDistKeywordsRaw,
			cachedDistKeywords
		);
		const std::vector<std::string>& odKeywords = GetKeywordList(
			"QCAutomationODKeywords",
			kDefaultOdKeywords,
			cachedOdKeywordsRaw,
			cachedOdKeywords
		);
		const std::vector<std::string>& cleanKeywords = GetKeywordList(
			"QCAutomationCleanKeywords",
			kDefaultCleanKeywords,
			cachedCleanKeywordsRaw,
			cachedCleanKeywords
		);
		const std::vector<std::string>& modKeywords = GetKeywordList(
			"QCAutomationModKeywords",
			kDefaultModKeywords,
			cachedModKeywordsRaw,
			cachedModKeywords
		);
		const int soloPriority = GetClassifierPriority(
			"QCAutomationSoloPriority",
			5,
			cachedSoloPriorityRaw,
			cachedSoloPriority
		);
		const int distPriority = GetClassifierPriority(
			"QCAutomationDistPriority",
			2,
			cachedDistPriorityRaw,
			cachedDistPriority
		);
		const int odPriority = GetClassifierPriority(
			"QCAutomationODPriority",
			1,
			cachedOdPriorityRaw,
			cachedOdPriority
		);
		const int cleanPriority = GetClassifierPriority(
			"QCAutomationCleanPriority",
			3,
			cachedCleanPriorityRaw,
			cachedCleanPriority
		);
		const int modPriority = GetClassifierPriority(
			"QCAutomationModPriority",
			4,
			cachedModPriorityRaw,
			cachedModPriority
		);

		std::array<ClassifierMatchRule, 5> rules{
			ClassifierMatchRule{ AutoToneBucket::Od, &odKeywords, odPriority, 0 },
			ClassifierMatchRule{ AutoToneBucket::Dist, &distKeywords, distPriority, 1 },
			ClassifierMatchRule{ AutoToneBucket::Clean, &cleanKeywords, cleanPriority, 2 },
			ClassifierMatchRule{ AutoToneBucket::Mod, &modKeywords, modPriority, 3 },
			ClassifierMatchRule{ AutoToneBucket::Solo, &soloKeywords, soloPriority, 4 }
		};

		std::stable_sort(
			rules.begin(),
			rules.end(),
			[](const ClassifierMatchRule& lhs, const ClassifierMatchRule& rhs) {
				if (lhs.priority != rhs.priority) {
					return lhs.priority < rhs.priority;
				}

				return lhs.fallbackOrder < rhs.fallbackOrder;
			}
		);

		for (const ClassifierMatchRule& rule : rules) {
			if (rule.keywords == nullptr) {
				continue;
			}

			if (MatchKeywordGroup(normalized, *rule.keywords, outMatchedKeyword)) {
				return rule.bucket;
			}
		}

		outMatchedKeyword = "fallback_clean";
		return AutoToneBucket::Clean;
	}

	QcTarget MapAutoBucketToTarget(AutoToneBucket bucket) {
		switch (bucket) {
			case AutoToneBucket::Solo:
				return QcTarget::AutoSolo;
			case AutoToneBucket::Dist:
				return QcTarget::AutoDist;
			case AutoToneBucket::Od:
				return QcTarget::AutoOd;
			case AutoToneBucket::Mod:
				return QcTarget::AutoMod;
			case AutoToneBucket::BassIgnore:
				return QcTarget::AutoClean;
			case AutoToneBucket::Clean:
			default:
				return QcTarget::AutoClean;
		}
	}

	size_t ToIndex(QcTarget target) {
		for (size_t idx = 0; idx < kQcTargets.size(); ++idx) {
			if (kQcTargets[idx].target == target) {
				return idx;
			}
		}

		return 0;
	}

	std::string GetConfiguredTargetSpec(const QcTargetInfo& targetInfo) {
		std::string configured = Settings::ReturnSettingValue(targetInfo.settingKey);
		if (configured.empty()) {
			configured = targetInfo.defaultTarget;
		}

		return configured;
	}

	bool TryParseMyPresetsTarget(std::string_view targetSpec, MidiRecallTarget& outTarget) {
		const std::string trimmed = TrimAscii(targetSpec);
		const std::string lowered = ToLowerAscii(trimmed);
		constexpr std::string_view kPrefix = "mypresets:";
		if (lowered.rfind(kPrefix, 0) != 0) {
			return false;
		}

		const std::string bankAndSlot = TrimAscii(std::string_view(trimmed).substr(kPrefix.size()));
		if (bankAndSlot.size() < 2) {
			return false;
		}

		const char slotLetter = static_cast<char>(std::toupper(static_cast<unsigned char>(bankAndSlot.back())));
		if (slotLetter < 'A' || slotLetter > 'H') {
			return false;
		}

		const std::string bankString = TrimAscii(std::string_view(bankAndSlot).substr(0, bankAndSlot.size() - 1));
		if (bankString.empty()) {
			return false;
		}

		int bankNumber = 0;
		try {
			bankNumber = std::stoi(bankString);
		}
		catch (...) {
			return false;
		}

		if (bankNumber < 1) {
			return false;
		}

		const int slotIndex = slotLetter - 'A';
		const int presetIndex = (bankNumber - 1) * 8 + slotIndex;
		outTarget.targetSpec = trimmed;
		outTarget.cc32 = kQcMyPresetsSetlistCc32;
		outTarget.cc0 = ClampMidi7Bit(presetIndex / 128);
		outTarget.program = ClampMidi7Bit(presetIndex % 128);
		return true;
	}

	bool ResolveMidiRecallTarget(const QcTargetInfo& targetInfo, MidiRecallTarget& outTarget) {
		const std::string configuredSpec = GetConfiguredTargetSpec(targetInfo);
		if (TryParseMyPresetsTarget(configuredSpec, outTarget)) {
			lastInvalidTargetSpecByIndex[ToIndex(targetInfo.target)].clear();
			return true;
		}

		const size_t index = ToIndex(targetInfo.target);
		if (lastInvalidTargetSpecByIndex[index] != configuredSpec) {
			lastInvalidTargetSpecByIndex[index] = configuredSpec;
			LOG_WARNING(kLogPrefix << "Invalid " << targetInfo.settingKey << "='" << configuredSpec
				<< "'. Falling back to default '" << targetInfo.defaultTarget << "'." << std::endl);
		}

		if (!TryParseMyPresetsTarget(targetInfo.defaultTarget, outTarget)) {
			LOG_ERROR(kLogPrefix << "Failed to parse default QC target '" << targetInfo.defaultTarget
				<< "' for " << targetInfo.roleLabel << "." << std::endl);
			return false;
		}

		return true;
	}

	void MarkTransposeResolveNeeded(const std::string& reason) {
		transposeNeedsResolve = true;
		transposeResolveReason = reason;
		loggedPendingTransposeResolve = false;
		activeSongSceneValue.reset();
		queuedSongSceneSend = false;
		deferredSongSceneSent = false;
		retrySongSceneSendPending = false;
		retrySongSceneSendAttempted = false;
		queuedSongSceneReason.clear();
	}

	SongTransposeDecision BuildSongTransposeDecision() {
		SongTransposeDecision decision;

		const std::array<byte, 6> tuningSnapshot = SongTuning::GetCurrentTuning();
		if (tuningSnapshot == std::array<byte, 6>{}) {
			decision.result = SongTransposeResult::Retry;
			decision.detail = "tuning not available yet";
			return decision;
		}

		const Tuning tuning(
			tuningSnapshot[0],
			tuningSnapshot[1],
			tuningSnapshot[2],
			tuningSnapshot[3],
			tuningSnapshot[4],
			tuningSnapshot[5]
		);

		const std::array<int, 2> highestLowest = SongTuning::GetHighestLowestString(tuning);
		const int highestTuning = highestLowest[0];
		const int lowestTuning = highestLowest[1];
		if (highestTuning == 666 && lowestTuning == 666) {
			decision.result = SongTransposeResult::Retry;
			decision.detail = "tuning not available yet";
			return decision;
		}

		// v1 transpose automation supports one global semitone offset.
		// For non-uniform drop tunings, assume player baseline is Drop D and
		// resolve from the non-low-string semitone (highestTuning).
		// Some charts are effectively drop-style without matching a strict string-by-string
		// pattern at runtime, so allow a drop-like 2-semitone spread as fallback.
		if (highestTuning != lowestTuning) {
			const bool isStrictDrop = SongTuning::IsSongInDrop(tuning);
			const bool isDropLikeSpread =
				highestTuning <= 0 &&
				lowestTuning <= 0 &&
				(highestTuning - lowestTuning) == 2;
			if (isStrictDrop || isDropLikeSpread) {
				decision.result = SongTransposeResult::Resolved;
				decision.semitones = highestTuning;
				if (isDropLikeSpread && !isStrictDrop) {
					decision.detail = "drop-like fallback (2-semitone spread)";
				}
				return decision;
			}

			std::ostringstream unsupported;
			unsupported << "non-uniform tuning (highest=" << highestTuning
				<< ", lowest=" << lowestTuning << ")";
			decision.result = SongTransposeResult::Unsupported;
			decision.detail = unsupported.str();
			return decision;
		}

		decision.result = SongTransposeResult::Resolved;
		decision.semitones = highestTuning;
		return decision;
	}

	void ResolveSongTransposeForCurrentContext() {
		if (!transposeNeedsResolve) {
			return;
		}

		const SongTransposeDecision decision = BuildSongTransposeDecision();
		if (decision.result == SongTransposeResult::Retry) {
			if (!loggedPendingTransposeResolve) {
				LOG_INFO(kLogPrefix << "transpose pending reason=" << transposeResolveReason
					<< " detail='" << decision.detail << "'" << std::endl);
				loggedPendingTransposeResolve = true;
			}
			return;
		}

		if (decision.result == SongTransposeResult::Unsupported) {
			LOG_WARNING(kLogPrefix << "transpose unsupported reason='" << transposeResolveReason
				<< "' detail='" << decision.detail
				<< "'. Scene automation skipped for this song context." << std::endl);
			activeSongSceneValue.reset();
			queuedSongSceneSend = false;
			deferredSongSceneSent = false;
			retrySongSceneSendPending = false;
			retrySongSceneSendAttempted = false;
			queuedSongSceneReason.clear();
			transposeNeedsResolve = false;
			loggedPendingTransposeResolve = false;
			return;
		}

		const int semitones = decision.semitones;
		int resolvedSemitones = semitones;
		const bool inSupportedRange = semitones >= kMinSongTransposeSemitone && semitones <= kMaxSongTransposeSemitone;
		if (!inSupportedRange) {
			const std::string outOfRangeBehavior = GetOutOfRangeBehavior();
			if (outOfRangeBehavior == "skip") {
				LOG_WARNING(kLogPrefix << "transpose out_of_range semitones=" << semitones
					<< " behavior='skip'" << std::endl);
				activeSongSceneValue.reset();
				queuedSongSceneSend = false;
				deferredSongSceneSent = false;
				retrySongSceneSendPending = false;
				retrySongSceneSendAttempted = false;
				queuedSongSceneReason.clear();
				transposeNeedsResolve = false;
				loggedPendingTransposeResolve = false;
				return;
			}

			resolvedSemitones = std::clamp(semitones, kMinSongTransposeSemitone, kMaxSongTransposeSemitone);
			LOG_WARNING(kLogPrefix << "transpose out_of_range semitones=" << semitones
				<< " behavior='clamp' scene='" << SceneValueToLetter(-resolvedSemitones) << "'" << std::endl);
		}

		const int sceneValue = ClampMidi7Bit(-resolvedSemitones);
		activeSongSceneValue = sceneValue;
		transposeNeedsResolve = false;
		loggedPendingTransposeResolve = false;
		queuedSongSceneSend = true;
		deferredSongSceneSent = false;
		retrySongSceneSendPending = false;
		retrySongSceneSendAttempted = false;
		queuedSongSceneReason = transposeResolveReason;

		if (!decision.detail.empty()) {
			LOG_INFO(kLogPrefix << "transpose resolved via fallback detail='" << decision.detail << "'" << std::endl);
		}
		LOG_INFO(kLogPrefix << "transpose computed source=" << transposeResolveReason
			<< " semitones=" << semitones
			<< " scene='" << SceneValueToLetter(sceneValue) << "'" << std::endl);
		LOG_INFO(kLogPrefix << "transpose scene queued reason=" << queuedSongSceneReason
			<< " scene='" << SceneValueToLetter(sceneValue) << "'" << std::endl);
	}

	bool EnsureConfiguredMidiOutDeviceSelected() {
		const std::string configuredDevice = Settings::ReturnSettingValue("QCAutomationDevice");
		if (configuredDevice.empty()) {
			if (!loggedMissingMidiDevice) {
				LOG_WARNING(kLogPrefix << "QCAutomationDevice is empty. MIDI sends are skipped." << std::endl);
				loggedMissingMidiDevice = true;
			}
			return false;
		}

		loggedMissingMidiDevice = false;

		if (configuredDevice != lastSelectedMidiOutDevice || !lastSelectedMidiOutDeviceResolved) {
			lastSelectedMidiOutDeviceResolved = Midi::RefreshMidiOutDevice(configuredDevice);
			lastSelectedMidiOutDevice = configuredDevice;
		}

		if (!lastSelectedMidiOutDeviceResolved) {
			if (!loggedConfiguredMidiDeviceNotFound) {
				LOG_WARNING(kLogPrefix << "QCAutomationDevice was not found among MIDI OUT devices. MIDI sends are skipped." << std::endl);
				loggedConfiguredMidiDeviceNotFound = true;
			}
			return false;
		}

		loggedConfiguredMidiDeviceNotFound = false;
		return true;
	}

	void MaybeLogClassifierResultChange(const TargetDecision& decision) {
		if (!decision.classifierBucket.has_value()) {
			return;
		}

		const bool sameBucket = lastLoggedClassifierBucket.has_value() &&
			*lastLoggedClassifierBucket == *decision.classifierBucket;
		const bool sameKeyword = lastLoggedClassifierKeyword == decision.matchedKeyword;
		const bool sameToneName = lastLoggedClassifierToneName == decision.authoredToneName;
		if (sameBucket && sameKeyword && sameToneName) {
			return;
		}

		lastLoggedClassifierBucket = *decision.classifierBucket;
		lastLoggedClassifierKeyword = decision.matchedKeyword;
		lastLoggedClassifierToneName = decision.authoredToneName;
		LOG_INFO(kLogPrefix << "classifier bucket=" << ToString(*decision.classifierBucket)
			<< " match='" << decision.matchedKeyword
			<< "' authoredToneName='" << decision.authoredToneName << "'" << std::endl);
	}

	void MaybeLogEffectiveTargetChange(const TargetDecision& decision) {
		const QcTargetInfo& targetInfo = GetQcTargetInfo(decision.target);
		const std::string configuredTargetSpec = GetConfiguredTargetSpec(targetInfo);
		if (lastLoggedTarget.has_value() &&
			*lastLoggedTarget == decision.target &&
			lastLoggedTargetSpec == configuredTargetSpec) {
			return;
		}

		lastLoggedTarget = decision.target;
		lastLoggedTargetSpec = configuredTargetSpec;
		LOG_INFO(kLogPrefix << "effectiveQCTarget source=" << ToString(decision.source)
			<< " value='" << decision.sourceValue
			<< "' targetRole='" << targetInfo.roleLabel
			<< "' target='" << configuredTargetSpec << "'" << std::endl);
	}

	bool SendMidiForTarget(const TargetDecision& decision) {
		if (!EnsureConfiguredMidiOutDeviceSelected()) {
			return false;
		}

		const QcTargetInfo& targetInfo = GetQcTargetInfo(decision.target);
		MidiRecallTarget recallTarget;
		if (!ResolveMidiRecallTarget(targetInfo, recallTarget)) {
			return false;
		}
		const char channel = static_cast<char>(ClampMidiChannel(Settings::GetModSetting("QCAutomationMidiChannel")));
		const char setlistCc32 = static_cast<char>(ClampMidi7Bit(recallTarget.cc32));
		const char presetGroupCc0 = static_cast<char>(ClampMidi7Bit(recallTarget.cc0));
		const char program = static_cast<char>(ClampMidi7Bit(recallTarget.program));

		// Quad Cortex preset recall:
		// CC#32 = setlist (My Presets is 1), CC#0 = preset group (1 => 128..255), then PC.
		const bool sentCc32 = Midi::SendRawControlChange(32, setlistCc32, channel);
		const bool sentCc0 = sentCc32 && Midi::SendRawControlChange(0, presetGroupCc0, channel);
		const bool sentProgram = sentCc0 && Midi::SendRawProgramChange(program, channel);
		if (!sentProgram) {
			return false;
		}

		LOG_INFO(kLogPrefix << "midiSend targetRole='" << targetInfo.roleLabel
			<< "' target='" << recallTarget.targetSpec
			<< "' CC#32=" << static_cast<int>(setlistCc32)
			<< " CC#0=" << static_cast<int>(presetGroupCc0)
			<< " PC=" << static_cast<int>(program)
			<< " channel=" << static_cast<int>(channel) << std::endl);
		return true;
	}

	bool SendSceneChangeIfNeeded(int sceneValue, bool forceSend = false) {
		const int clampedSceneValue = ClampMidi7Bit(sceneValue);
		if (!forceSend && lastSentSceneValue.has_value() && *lastSentSceneValue == clampedSceneValue) {
			return false;
		}

		if (!EnsureConfiguredMidiOutDeviceSelected()) {
			return false;
		}

		const char channel = static_cast<char>(ClampMidiChannel(Settings::GetModSetting("QCAutomationMidiChannel")));
		const int sceneCc = GetSceneCcNumber();
		if (!Midi::SendRawControlChange(
			static_cast<char>(sceneCc),
			static_cast<char>(clampedSceneValue),
			channel)) {
			return false;
		}

		lastSentSceneValue = clampedSceneValue;
		LOG_INFO(kLogPrefix << "sceneSend CC#" << sceneCc
			<< "=" << clampedSceneValue
			<< " scene='" << SceneValueToLetter(clampedSceneValue)
			<< "' channel=" << static_cast<int>(channel) << std::endl);
		return true;
	}

	bool ApplyTargetDecision(const TargetDecision& decision) {
		MaybeLogClassifierResultChange(decision);
		MaybeLogEffectiveTargetChange(decision);

		const QcTargetInfo& targetInfo = GetQcTargetInfo(decision.target);
		const std::string configuredTargetSpec = GetConfiguredTargetSpec(targetInfo);
		if (lastSentTarget.has_value() &&
			*lastSentTarget == decision.target &&
			lastSentTargetSpec == configuredTargetSpec) {
			return false;
		}

		if (SendMidiForTarget(decision)) {
			lastSentTarget = decision.target;
			lastSentTargetSpec = configuredTargetSpec;
			return true;
		}

		return false;
	}

	std::optional<TargetDecision> ResolveInSongTargetDecision() {
		int manualSlot = 0;
		if (QCAutomation::ToneSource::TryGetCurrentToneSlot(manualSlot)) {
			TargetDecision decision;
			decision.source = EffectiveIntentSource::Manual;
			decision.sourceValue = std::to_string(manualSlot);

			switch (manualSlot) {
				case 2:
					decision.target = QcTarget::Manual2;
					return decision;
				case 3:
					decision.target = QcTarget::Manual3;
					return decision;
				case 4:
					decision.target = QcTarget::Manual4;
					return decision;
				default:
					return std::nullopt;
			}
		}

		std::string authoredToneName;
		if (!QCAutomation::ToneSource::TryGetCurrentAuthoredToneName(authoredToneName)) {
			return std::nullopt;
		}

		TargetDecision decision;
		decision.source = EffectiveIntentSource::Auto;
		decision.sourceValue = authoredToneName;
		decision.authoredToneName = authoredToneName;

		std::string matchedKeyword;
		const AutoToneBucket bucket = ClassifyAutoToneBucket(authoredToneName, matchedKeyword);
		decision.classifierBucket = bucket;
		decision.matchedKeyword = matchedKeyword;
		if (bucket == AutoToneBucket::BassIgnore) {
			MaybeLogClassifierResultChange(decision);
			return std::nullopt;
		}
		decision.target = MapAutoBucketToTarget(bucket);
		return decision;
	}

	TargetDecision BuildIdleTargetDecision() {
		TargetDecision decision;
		decision.target = QcTarget::Idle;
		decision.source = EffectiveIntentSource::Idle;
		decision.sourceValue = "2";
		return decision;
	}

	void UpdateInSongTransposeContext() {
		const std::string songKey = GameState::GetSongKey();
		const std::string arrangementId = GameState::GetArrangementID();
		const float playbackTimeSec = SongTimer::SongTimer();

		if (!transposeStateWasInSong) {
			transposeStateWasInSong = true;
			activeSongKey.clear();
			activeArrangementId.clear();
			previousInSongPlaybackTimeSec.reset();
			MarkTransposeResolveNeeded("song_start");
			LOG_INFO(kLogPrefix << "transpose recomputed reason=song_start" << std::endl);
		}

		if (!songKey.empty() && !arrangementId.empty() &&
			(songKey != activeSongKey || arrangementId != activeArrangementId)) {
			const std::string reason = activeSongKey.empty() ? "song_start" : ((songKey == activeSongKey) ? "arrangement_change" : "reenter");
			activeSongKey = songKey;
			activeArrangementId = arrangementId;
			previousInSongPlaybackTimeSec.reset();
			MarkTransposeResolveNeeded(reason);
			LOG_INFO(kLogPrefix << "transpose recomputed reason=" << reason << std::endl);
		}
		else if (previousInSongPlaybackTimeSec.has_value() &&
			IsLikelySongRestart(*previousInSongPlaybackTimeSec, playbackTimeSec)) {
			MarkTransposeResolveNeeded("song_restart");
			LOG_INFO(kLogPrefix << "transpose recomputed reason=song_restart" << std::endl);
		}

		previousInSongPlaybackTimeSec = playbackTimeSec;

		if (Settings::ReturnSettingValue("QCAutomationTransposeEnabled") != "on") {
			transposeNeedsResolve = false;
			activeSongSceneValue.reset();
			queuedSongSceneSend = false;
			deferredSongSceneSent = false;
			retrySongSceneSendPending = false;
			retrySongSceneSendAttempted = false;
			queuedSongSceneReason.clear();
			return;
		}

		ResolveSongTransposeForCurrentContext();
	}

	void ApplyInSongSceneIfNeeded(bool presetChanged, float playbackTimeSec) {
		if (Settings::ReturnSettingValue("QCAutomationTransposeEnabled") != "on") {
			return;
		}

		if (!activeSongSceneValue.has_value()) {
			return;
		}

		const int desiredSceneValue = *activeSongSceneValue;
		if (queuedSongSceneSend && !deferredSongSceneSent) {
			if (!std::isfinite(playbackTimeSec) || playbackTimeSec < kDeferredSceneMinPlaybackTimeSec) {
				return;
			}

			queuedSongSceneSend = false;
			deferredSongSceneSent = true;
			retrySongSceneSendPending = true;
			retrySongSceneSendAttempted = false;

			const bool deferredSendSuccess = SendSceneChangeIfNeeded(desiredSceneValue, true);
			LOG_INFO(kLogPrefix << "transpose scene deferred send reason=" << queuedSongSceneReason
				<< " t=" << std::fixed << std::setprecision(3) << playbackTimeSec
				<< " scene='" << SceneValueToLetter(desiredSceneValue)
				<< "' sent=" << (deferredSendSuccess ? "true" : "false") << std::endl);
			return;
		}

		if (retrySongSceneSendPending && !retrySongSceneSendAttempted && presetChanged) {
			retrySongSceneSendAttempted = true;
			retrySongSceneSendPending = false;
			const bool retrySendSuccess = SendSceneChangeIfNeeded(desiredSceneValue, true);
			LOG_INFO(kLogPrefix << "transpose scene retry/resend reason=first_preset_recall"
				<< " scene='" << SceneValueToLetter(desiredSceneValue)
				<< "' sent=" << (retrySendSuccess ? "true" : "false") << std::endl);
			return;
		}

		const bool sceneSent = SendSceneChangeIfNeeded(desiredSceneValue);
		if (!sceneSent &&
			presetChanged &&
			lastSentSceneValue.has_value() &&
			*lastSentSceneValue == desiredSceneValue) {
			const bool reapplySendSuccess = SendSceneChangeIfNeeded(desiredSceneValue, true);
			LOG_INFO(kLogPrefix << "transpose scene reapply reason=preset_change"
				<< " scene='" << SceneValueToLetter(desiredSceneValue)
				<< "' sent=" << (reapplySendSuccess ? "true" : "false") << std::endl);
		}
	}

	void ApplyIdleSceneIfNeeded() {
		if (Settings::ReturnSettingValue("QCAutomationTransposeEnabled") != "on") {
			return;
		}

		const int idleSceneValue = GetIdleSceneValue();
		(void)SendSceneChangeIfNeeded(idleSceneValue);
	}

	void ResetAutomationState() {
		loggedMissingMidiDevice = false;
		loggedConfiguredMidiDeviceNotFound = false;
		lastSelectedMidiOutDevice.clear();
		lastSelectedMidiOutDeviceResolved = false;
		lastLoggedClassifierBucket.reset();
		lastLoggedClassifierKeyword.clear();
		lastLoggedClassifierToneName.clear();
		lastLoggedTarget.reset();
		lastLoggedTargetSpec.clear();
		lastSentTarget.reset();
		lastSentTargetSpec.clear();
		lastSentSceneValue.reset();
		activeSongSceneValue.reset();
		queuedSongSceneSend = false;
		deferredSongSceneSent = false;
		retrySongSceneSendPending = false;
		retrySongSceneSendAttempted = false;
		queuedSongSceneReason.clear();
		activeSongKey.clear();
		activeArrangementId.clear();
		transposeNeedsResolve = false;
		transposeResolveReason.clear();
		loggedPendingTransposeResolve = false;
		lastInvalidIdleSceneSetting.clear();
		lastInvalidOutOfRangeBehaviorSetting.clear();
		transposeStateWasInSong = false;
		previousInSongPlaybackTimeSec.reset();
		for (std::string& lastInvalidSpec : lastInvalidTargetSpecByIndex) {
			lastInvalidSpec.clear();
		}
	}
}

namespace QCAutomation {
	void Initialize() {
		ToneSource::Initialize();
	}

	void HandleInSongState() {
		if (Settings::ReturnSettingValue("QCAutomationEnabled") != "on") {
			return;
		}

		const float playbackTimeSec = SongTimer::SongTimer();
		UpdateInSongTransposeContext();
		ApplyInSongSceneIfNeeded(false, playbackTimeSec);
		ToneSource::HandleInSongState();

		const auto decision = ResolveInSongTargetDecision();
		if (!decision.has_value()) {
			return;
		}

		const bool presetChanged = ApplyTargetDecision(*decision);
		ApplyInSongSceneIfNeeded(presetChanged, playbackTimeSec);
	}

	void HandleOutOfSongState() {
		ToneSource::HandleOutOfSongState();
		transposeStateWasInSong = false;
		activeSongKey.clear();
		activeArrangementId.clear();
		previousInSongPlaybackTimeSec.reset();
		MarkTransposeResolveNeeded("reenter");

		if (Settings::ReturnSettingValue("QCAutomationEnabled") != "on") {
			ResetAutomationState();
			return;
		}

		(void)ApplyTargetDecision(BuildIdleTargetDecision());
		ApplyIdleSceneIfNeeded();
	}
}
