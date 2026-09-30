#include "../stdafx.h"
#include "MetronomeBeats.hpp"

#include <charconv>
#include <optional>

using Metronome::Beat;
using Metronome::BeatMap;
using Metronome::BeatMapSource;

namespace {
	constexpr wchar_t kExtractBeatsCommand[] = L"--extract-beats";
	constexpr char kBeatsExtension[] = ".beats";
	constexpr char kHeaderMarker = '#';

	// The key goes into a file name and a command line. Song keys are letters, digits, underscores and the odd
	// hyphen, so anything else is refused rather than escaped.
	bool IsUsableSongKey(const std::string& songKey) {
		auto isKeyCharacter = [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '-'; };
		return !songKey.empty() && std::ranges::all_of(songKey, isKeyCharacter);
	}

	std::wstring Quoted(const std::wstring& text) {
		return L"\"" + text + L"\"";
	}

	std::filesystem::path GameFolder() {
		wchar_t exePath[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		return std::filesystem::path(exePath).parent_path();
	}

	// "<seconds> <1 if the beat starts a measure, else 0>", as the GUI's BeatMapFile writes it.
	std::optional<Beat> ParseBeat(std::string_view line) {
		const char* end = line.data() + line.size();

		Beat beat;
		auto [afterSeconds, secondsError] = std::from_chars(line.data(), end, beat.seconds);
		if (secondsError != std::errc() || afterSeconds == end) return std::nullopt;

		int measureFlag = 0;
		auto [afterFlag, flagError] = std::from_chars(afterSeconds + 1, end, measureFlag);
		if (flagError != std::errc()) return std::nullopt;

		beat.startsMeasure = measureFlag == 1;
		return beat;
	}
}

BeatMapSource::BeatMapSource()
	: gameFolder(GameFolder()), rsModsFolder(gameFolder / "RSMods") {}

void BeatMapSource::Request(const std::string& songKey) {
	if (!IsUsableSongKey(songKey)) return;

	const bool firstRequest = requestedSongKeys.insert(songKey).second;
	if (firstRequest) StartExtraction(songKey);
}

BeatMap BeatMapSource::Load(const std::string& songKey) const {
	if (!IsUsableSongKey(songKey)) return {};

	BeatMap beats;
	std::ifstream file(BeatsFile(songKey));
	for (std::string line; std::getline(file, line);) {
		if (line.empty() || line.front() == kHeaderMarker) continue;
		if (auto beat = ParseBeat(line)) beats.push_back(*beat);
	}
	return beats;
}

std::filesystem::path BeatMapSource::BeatsFile(const std::string& songKey) const {
	return rsModsFolder / "MetronomeBeats" / (songKey + kBeatsExtension);
}

void BeatMapSource::StartExtraction(const std::string& songKey) const {
	const std::filesystem::path helper = rsModsFolder / "RSMods.exe";
	const std::wstring wideSongKey(songKey.begin(), songKey.end()); // ASCII only, see IsUsableSongKey.

	std::wstring commandLine = Quoted(helper.wstring()) + L" " + kExtractBeatsCommand
		+ L" " + Quoted(gameFolder.wstring())
		+ L" " + Quoted(wideSongKey)
		+ L" " + Quoted(BeatsFile(songKey).wstring());

	STARTUPINFOW startupInfo = { sizeof(startupInfo) };
	PROCESS_INFORMATION process = {};
	if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &process)) {
		LOG_ERROR("(Metronome) Couldn't start " << helper.string() << " to extract the beats of " << songKey
			<< " (error " << GetLastError() << ")" << std::endl);
		return;
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
}
