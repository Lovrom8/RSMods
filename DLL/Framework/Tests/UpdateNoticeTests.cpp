#include "../UpdateNotice.hpp"

#include <iostream>
#include <vector>

namespace {
	int failures = 0;

	void Check(bool condition, const char* what) {
		if (!condition) {
			++failures;
			std::cerr << "FAIL: " << what << "\n";
		}
	}

	using namespace Framework::UpdateNotice;

	bool Newer(const char* remote, const char* local) {
		const auto r = ParseVersion(remote);
		const auto l = ParseVersion(local);
		return r && l && IsNewer(*r, *l);
	}
}

int main() {
	Check(ParseVersion("RSModsInstaller-v1.2.8.4_OnCommit") == Version{ 1, 2, 8, 4 }, "installer tag");
	Check(ParseVersion("v0.7.5") == Version{ 0, 7, 5, 0 }, "asio tag");
	Check(ParseVersion("1.2.8.4-abc1234") == Version{ 1, 2, 8, 4 }, "version with sha");
	Check(ParseVersion("v7-beta 2.1") == Version{ 2, 1, 0, 0 }, "lone number skipped");
	Check(ParseVersion("v1.2.3.4.5") == Version{ 1, 2, 3, 4 }, "fifth component ignored");
	Check(!ParseVersion("latest"), "no digits");
	Check(!ParseVersion("v7"), "single number");
	Check(!ParseVersion(""), "empty");
	Check(!ParseVersion("1.2345678901"), "overlong component");

	Check(Newer("0.7.6", "0.7.5"), "patch bump");
	Check(Newer("1.10.0", "1.9.9"), "numeric not lexical");
	Check(Newer("1.2.8.5", "1.2.8.4"), "build bump");
	Check(Newer("1.2.9", "1.2.8.4"), "shorter remote still newer");
	Check(!Newer("0.7.5", "0.7.5"), "equal");
	Check(!Newer("1.2.8", "1.2.8.0"), "missing component is zero");
	Check(!Newer("0.7.4", "0.7.5"), "older remote");
	Check(!Newer("1.2.8.3", "1.2.9"), "newer local never matches");

	const std::string release = "{\"url\":\"x\",\"tag_name\": \"v0.7.6\",\"name\":\"v0.7.6\"}";
	Check(ExtractJsonString(release, "tag_name") == std::string("v0.7.6"), "tag_name");
	Check(!ExtractJsonString(release, "missing"), "missing key");
	Check(!ExtractJsonString("{\"tag_name\":\"a\\\"b\"}", "tag_name"), "escaped value rejected");
	Check(!ExtractJsonString("{\"tag_name\":null}", "tag_name"), "null value");
	Check(!ExtractJsonString("{\"message\":\"API rate limit exceeded\"}", "tag_name"), "rate limit body");

	Check(FormatVersion({ 1, 2, 8, 4 }) == "1.2.8.4", "format four");
	Check(FormatVersion({ 0, 7, 5, 0 }) == "0.7.5", "format drops trailing zero");
	Check(FormatVersion({ 1, 0, 0, 0 }) == "1.0", "format keeps two");

	Check(!CheckTag("RS_ASIO", { 0, 7, 5, 0 }, "v0.7.5"), "no update when equal");
	Check(!CheckTag("RS_ASIO", { 0, 7, 5, 0 }, "garbage"), "no update on garbage tag");
	const auto asio = CheckTag("RS_ASIO", { 0, 7, 5, 0 }, "v0.7.6");
	const auto mods = CheckTag("RSMods", { 1, 2, 8, 4 }, "RSModsInstaller-v1.2.9.0_OnCommit");
	Check(asio && mods, "updates found");

	Check(BuildBody(std::vector<Candidate>{}).empty(), "empty body when nothing newer");
	if (asio && mods) {
		Check(BuildBody(std::vector<Candidate>{ *mods }) ==
			"RSMods 1.2.9 (installed 1.2.8.4). Get it from the RSMods releases page on GitHub.",
			"single body");
		Check(BuildBody(std::vector<Candidate>{ *asio }) ==
			"RS_ASIO 0.7.6 (installed 0.7.5). Get it from the RS_ASIO releases page on GitHub.",
			"single asio body");
		Check(BuildBody(std::vector<Candidate>{ *mods, *asio }) ==
			"RSMods 1.2.9 (installed 1.2.8.4), RS_ASIO 0.7.6 (installed 0.7.5). "
			"Get them from their GitHub releases pages.", "double body");
	}

	CheckRecord record;
	record.checkedAt = 1700000000;
	record.jitter = 3 * 24 * 60 * 60;
	record.modsTag = "RSModsInstaller-v1.2.9.0_OnCommit";
	record.asioTag = "";
	const auto encoded = EncodeRecord(record);
	const auto decoded = DecodeRecord(encoded);
	Check(decoded && decoded->checkedAt == record.checkedAt && decoded->jitter == record.jitter &&
		decoded->modsTag == record.modsTag && decoded->asioTag.empty(), "record round trip");
	auto truncated = encoded;
	truncated.pop_back();
	Check(!DecodeRecord(truncated), "truncated record rejected");
	auto trailing = encoded;
	trailing.push_back(0);
	Check(!DecodeRecord(trailing), "trailing bytes rejected");
	Check(!DecodeRecord({}), "empty record rejected");
	CheckRecord oversized = record;
	oversized.asioTag = std::string(kMaxTagBytes + 1, 'x');
	Check(!DecodeRecord(EncodeRecord(oversized)), "oversized tag rejected");
	CheckRecord badJitter = record;
	badJitter.jitter = kMaxJitterSeconds;
	Check(!DecodeRecord(EncodeRecord(badJitter)), "jitter out of range rejected");

	Check(JitterFromSeed(0xFFFFFFFFu) < kMaxJitterSeconds, "jitter in range");
	const uint64_t limit = record.checkedAt + kCheckIntervalSeconds + record.jitter;
	Check(IsFresh(record, record.checkedAt), "fresh right after the check");
	Check(IsFresh(record, limit - 1), "fresh just before the limit");
	Check(!IsFresh(record, limit), "stale at the limit");
	Check(!IsFresh(record, record.checkedAt - 1), "clock set back is stale");
	Check(AgeDays(record, record.checkedAt + 5 * 24 * 60 * 60 + 100) == 5, "age in days");

	const std::optional<Version> localMods = Version{ 1, 2, 8, 4 };
	const std::optional<Version> localAsio = Version{ 0, 7, 5, 0 };
	Check(FindUpdates(localMods, "RSModsInstaller-v1.2.9.0_OnCommit", localAsio, "v0.7.5").size() == 1, "stored tags: one update");
	Check(FindUpdates(localMods, "RSModsInstaller-v1.2.9.0_OnCommit", localAsio, "v0.7.6").size() == 2, "stored tags: two updates");
	Check(FindUpdates(Version{ 1, 2, 9, 0 }, "RSModsInstaller-v1.2.9.0_OnCommit", localAsio, "").empty(), "installed since: no claim");
	Check(FindUpdates(localMods, "", std::nullopt, "v0.7.6").empty(), "empty tag and absent RS_ASIO give none");

	if (failures == 0)
		std::cout << "Update notice checks ALL PASS\n";
	return failures == 0 ? 0 : 1;
}
