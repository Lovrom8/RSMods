#include "../CacheFile.hpp"

#include <cstdio>
#include <iostream>

namespace {
	int failures = 0;

	void Check(bool condition, const char* what) {
		if (!condition) {
			++failures;
			std::cerr << "FAIL: " << what << "\n";
		}
	}

	using namespace Framework::CacheFile;

	const std::map<uint32_t, uint32_t> versions = { { kEnumeration, 1 }, { kVfs, 1 } };

	Sections Sample() {
		Sections sections;
		sections[kEnumeration] = { 1, { 1, 2, 3, 4, 5 } };
		sections[kVfs] = { 1, { 9, 8, 7 } };
		return sections;
	}

	// Offset of a section's payload, found by walking the layout Build writes.
	size_t PayloadAt(const std::vector<unsigned char>& bytes, uint32_t wanted) {
		Reader in{ bytes.data(), bytes.size() };
		uint32_t value = 0;
		uint64_t sum = 0;
		std::string identity;
		in.Get32(value);
		in.Get32(value);
		in.GetBytes(identity, kMaxIdentity);
		uint32_t count = 0;
		in.Get32(count);
		in.Get64(sum);
		for (uint32_t i = 0; i < count; ++i) {
			uint32_t id = 0;
			uint32_t size = 0;
			in.Get32(id);
			in.Get32(value);
			in.Get64(sum);
			in.Get32(size);
			if (id == wanted) return in.at;
			in.at += size;
		}
		return 0;
	}

	size_t VersionAt(const std::vector<unsigned char>& bytes, uint32_t wanted) {
		return PayloadAt(bytes, wanted) - 4 - 8 - 4;
	}
}

int main() {
	const std::string identity = "root C:/Game/; exe 1,2; pe 3,4";
	const auto bytes = Build(identity, Sample());

	{
		const auto parsed = Parse(bytes, identity, versions);
		Check(parsed.status == Status::Ok, "round trip status");
		Check(parsed.sections.size() == 2, "round trip keeps both sections");
		Check(parsed.sections.at(kEnumeration).payload == Sample()[kEnumeration].payload, "round trip enumeration payload");
		Check(parsed.sections.at(kVfs).payload == Sample()[kVfs].payload, "round trip vfs payload");
		Check(parsed.dropped.empty() && !parsed.truncated, "round trip drops nothing");
	}
	{
		auto damaged = bytes;
		damaged[PayloadAt(damaged, kEnumeration)] ^= 0xFF;
		const auto parsed = Parse(damaged, identity, versions);
		Check(parsed.status == Status::Ok, "corrupt section keeps the file");
		Check(!parsed.sections.count(kEnumeration), "corrupt section dropped");
		Check(parsed.sections.count(kVfs) == 1, "other section kept after corruption");
		Check(parsed.dropped.size() == 1 && parsed.dropped[0] == kEnumeration, "corrupt section reported");
	}
	{
		auto bumped = bytes;
		bumped[VersionAt(bumped, kVfs)] = 2;
		const auto parsed = Parse(bumped, identity, versions);
		Check(parsed.status == Status::Ok, "version bump keeps the file");
		Check(!parsed.sections.count(kVfs), "bumped section dropped");
		Check(parsed.sections.count(kEnumeration) == 1, "other section kept after version bump");
		const auto newer = Parse(bytes, identity, { { kEnumeration, 1 }, { kVfs, 2 } });
		Check(!newer.sections.count(kVfs) && newer.sections.count(kEnumeration) == 1, "owner version bump drops only its section");
	}
	{
		Sections withUnknown = Sample();
		withUnknown[77] = { 1, { 1 } };
		const auto parsed = Parse(Build(identity, withUnknown), identity, versions);
		Check(parsed.sections.size() == 2 && parsed.dropped.size() == 1, "unknown section dropped alone");
	}
	{
		const auto parsed = Parse(bytes, "root D:/Other/; exe 1,2; pe 3,4", versions);
		Check(parsed.status == Status::OtherIdentity, "identity mismatch reported");
		Check(parsed.sections.empty(), "identity mismatch drops all sections");
	}
	{
		auto header = bytes;
		header[14] ^= 0x01;
		const auto parsed = Parse(header, identity, versions);
		Check(parsed.status == Status::Damaged && parsed.sections.empty(), "damaged header drops all sections");
		auto format = bytes;
		format[4] = 9;
		Check(Parse(format, identity, versions).status == Status::OtherFormat, "other file format reported");
	}
	{
		auto cut = bytes;
		cut.resize(PayloadAt(bytes, kVfs) + 1);
		const auto parsed = Parse(cut, identity, versions);
		Check(parsed.status == Status::Ok && parsed.truncated, "truncated file reported");
		Check(parsed.sections.count(kEnumeration) == 1 && !parsed.sections.count(kVfs), "truncated file keeps whole sections");
		for (size_t size = 0; size < bytes.size(); ++size) {
			const std::vector<unsigned char> prefix(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(size));
			const auto partial = Parse(prefix, identity, versions);
			Check(partial.sections.size() < 2, "every prefix loses at least one section");
		}
		Check(Parse({}, identity, versions).status == Status::Damaged, "empty file is damaged");
	}
	{
		auto trailing = bytes;
		trailing.push_back(0);
		const auto parsed = Parse(trailing, identity, versions);
		Check(parsed.sections.size() == 2 && parsed.truncated, "trailing bytes reported, sections kept");
	}

	if (failures) {
		std::cerr << failures << " cache file checks failed\n";
		return 1;
	}
	std::cout << "Cache file ALL PASS\n";
	return 0;
}
