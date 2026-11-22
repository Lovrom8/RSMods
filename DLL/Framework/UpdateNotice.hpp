#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CacheFile.hpp"

// Pure parts of the update notice: no windows.h, no game memory, so they build in the portable tests.
namespace Framework::UpdateNotice {
	using Version = std::array<unsigned, 4>;

	namespace detail {
		inline bool IsDigit(char c) { return c >= '0' && c <= '9'; }
		inline bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
	}

	// Reads the first dotted run of 2 to 4 numbers, so "RSModsInstaller-v1.2.8.4_OnCommit" gives 1.2.8.4 and
	// "v0.7.5" gives 0.7.5. A tag with no such run, or a component over 9 digits, gives nothing.
	inline std::optional<Version> ParseVersion(std::string_view text) {
		std::size_t start = 0;
		while (start < text.size()) {
			if (!detail::IsDigit(text[start])) {
				++start;
				continue;
			}

			Version version{};
			std::size_t parts = 0;
			std::size_t pos = start;
			while (parts < version.size()) {
				const std::size_t begin = pos;
				unsigned value = 0;
				while (pos < text.size() && detail::IsDigit(text[pos])) {
					if (pos - begin >= 9)
						return std::nullopt;
					value = value * 10 + static_cast<unsigned>(text[pos] - '0');
					++pos;
				}
				version[parts++] = value;
				if (pos + 1 < text.size() && text[pos] == '.' && detail::IsDigit(text[pos + 1]))
					++pos;
				else
					break;
			}
			if (parts >= 2)
				return version;
			// A lone number is skipped whole; a later run may still be a version.
			start = pos;
		}
		return std::nullopt;
	}

	// Numeric, component by component. Only a strictly newer remote counts, so a newer local build never matches.
	inline bool IsNewer(const Version& remote, const Version& local) {
		for (std::size_t i = 0; i < remote.size(); ++i) {
			if (remote[i] != local[i])
				return remote[i] > local[i];
		}
		return false;
	}

	inline std::string FormatVersion(const Version& version) {
		std::size_t count = version.size();
		while (count > 2 && version[count - 1] == 0)
			--count;
		std::string text;
		for (std::size_t i = 0; i < count; ++i) {
			if (i)
				text += '.';
			text += std::to_string(version[i]);
		}
		return text;
	}

	// Value of the first "key": "value" pair in a GitHub release body. Gives nothing for an escaped value,
	// which a release tag never has.
	inline std::optional<std::string> ExtractJsonString(std::string_view json, std::string_view key) {
		const std::string needle = "\"" + std::string(key) + "\"";
		for (std::size_t at = json.find(needle); at != std::string_view::npos; at = json.find(needle, at + 1)) {
			std::size_t pos = at + needle.size();
			while (pos < json.size() && detail::IsSpace(json[pos]))
				++pos;
			if (pos >= json.size() || json[pos] != ':')
				continue;
			++pos;
			while (pos < json.size() && detail::IsSpace(json[pos]))
				++pos;
			if (pos >= json.size() || json[pos] != '"')
				continue;
			const std::size_t end = json.find('"', pos + 1);
			if (end == std::string_view::npos)
				return std::nullopt;
			const std::string_view value = json.substr(pos + 1, end - pos - 1);
			if (value.find('\\') != std::string_view::npos)
				return std::nullopt;
			return std::string(value);
		}
		return std::nullopt;
	}

	// One component's installed and latest versions, as shown to the player.
	struct Candidate {
		std::string name;
		Version local{};
		Version remote{};
	};

	// Gives the candidate if the remote tag is strictly newer than the local version, else nothing.
	inline std::optional<Candidate> CheckTag(std::string name, const Version& local, std::string_view tag) {
		const auto remote = ParseVersion(tag);
		if (!remote || !IsNewer(*remote, local))
			return std::nullopt;
		return Candidate{ std::move(name), local, *remote };
	}

	// The notification body, without a header: the components that have an update and where to get it.
	// Nothing installs by itself, so it says the player downloads it. Empty when nothing has an update.
	template <typename Range>
	inline std::string BuildBody(const Range& updates) {
		std::string text;
		std::string firstName;
		std::size_t count = 0;
		for (const Candidate& update : updates) {
			if (count++)
				text += ", ";
			else
				firstName = update.name;
			text += update.name + " " + FormatVersion(update.remote) + " (installed " + FormatVersion(update.local) + ")";
		}
		if (count == 0)
			return text;
		// No URL: it would not fit the panel, and the page name is enough to find it. Naming the
		// releases page already says the download is the player's to do.
		if (count == 1)
			return text + ". Get it from the " + firstName + " releases page on GitHub.";
		return text + ". Get them from their GitHub releases pages.";
	}

	// GitHub is asked about once a month. The per-install jitter spreads installs out, so a release
	// day does not send them all at the same moment.
	constexpr uint64_t kCheckIntervalSeconds = 30ull * 24 * 60 * 60;
	constexpr uint32_t kMaxJitterSeconds = 4u * 24 * 60 * 60;
	constexpr uint32_t kMaxTagBytes = 256;

	// What the last successful check left in the cache. The tags are kept, never a finished message:
	// they are compared with the versions installed at that moment, so a player who has since updated
	// is not told about an update they already have.
	struct CheckRecord {
		uint64_t checkedAt = 0;
		uint32_t jitter = 0;
		std::string modsTag;
		std::string asioTag;
	};

	inline uint32_t JitterFromSeed(uint32_t seed) { return seed % kMaxJitterSeconds; }

	inline std::vector<unsigned char> EncodeRecord(const CheckRecord& record) {
		std::vector<unsigned char> out;
		CacheFile::Put64(out, record.checkedAt);
		CacheFile::Put32(out, record.jitter);
		CacheFile::PutBytes(out, record.modsTag.data(), record.modsTag.size());
		CacheFile::PutBytes(out, record.asioTag.data(), record.asioTag.size());
		return out;
	}

	// Gives nothing for a short, oversized or trailing-garbage payload.
	inline std::optional<CheckRecord> DecodeRecord(const std::vector<unsigned char>& bytes) {
		CacheFile::Reader in{ bytes.data(), bytes.size() };
		CheckRecord record;
		if (!in.Get64(record.checkedAt) || !in.Get32(record.jitter) ||
			!in.GetBytes(record.modsTag, kMaxTagBytes) || !in.GetBytes(record.asioTag, kMaxTagBytes) ||
			in.Left() != 0 || record.jitter >= kMaxJitterSeconds)
			return std::nullopt;
		return record;
	}

	// True while the stored check is younger than the interval plus this install's jitter. A clock set
	// back before the check counts as stale, so a wrong clock cannot silence the check for years.
	inline bool IsFresh(const CheckRecord& record, uint64_t now) {
		if (now < record.checkedAt)
			return false;
		return now - record.checkedAt < kCheckIntervalSeconds + record.jitter;
	}

	inline uint64_t AgeDays(const CheckRecord& record, uint64_t now) {
		return now > record.checkedAt ? (now - record.checkedAt) / (24 * 60 * 60) : 0;
	}

	// Candidates for the tags against the installed versions. An absent version or an empty tag gives none.
	inline std::vector<Candidate> FindUpdates(const std::optional<Version>& localMods, std::string_view modsTag,
		const std::optional<Version>& localAsio, std::string_view asioTag) {
		std::vector<Candidate> updates;
		if (localMods && !modsTag.empty()) {
			if (auto update = CheckTag("RSMods", *localMods, modsTag))
				updates.push_back(std::move(*update));
		}
		if (localAsio && !asioTag.empty()) {
			if (auto update = CheckTag("RS_ASIO", *localAsio, asioTag))
				updates.push_back(std::move(*update));
		}
		return updates;
	}
}
