#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

// One cache file holding independent sections. The header binds the file to one game install;
// each section carries its own version, length and checksum so a bad section drops alone.
namespace Framework::CacheFile {
	// RSMC magic.
	constexpr uint32_t kMagic = 0x434D5352;
	constexpr uint32_t kFormat = 1;
	constexpr uint32_t kMaxIdentity = 8192;
	constexpr uint32_t kMaxSections = 64;
	constexpr size_t kMaxFile = 256u * 1024 * 1024;

	constexpr uint32_t kEnumeration = 1;
	constexpr uint32_t kVfs = 2;

	inline uint64_t Fnv64(const void* data, size_t size) {
		uint64_t hash = 0xcbf29ce484222325ull;
		const auto* bytes = static_cast<const unsigned char*>(data);
		for (size_t i = 0; i < size; ++i) {
			hash ^= bytes[i];
			hash *= 0x100000001b3ull;
		}
		return hash;
	}

	inline void Put32(std::vector<unsigned char>& out, uint32_t value) {
		for (int i = 0; i < 4; ++i)
			out.push_back(static_cast<unsigned char>(value >> (8 * i)));
	}

	inline void Put64(std::vector<unsigned char>& out, uint64_t value) {
		for (int i = 0; i < 8; ++i)
			out.push_back(static_cast<unsigned char>(value >> (8 * i)));
	}

	inline void PutBytes(std::vector<unsigned char>& out, const void* data, size_t size) {
		Put32(out, static_cast<uint32_t>(size));
		const auto* bytes = static_cast<const unsigned char*>(data);
		out.insert(out.end(), bytes, bytes + size);
	}

	// Bounds-checked little-endian reader.
	struct Reader {
		const unsigned char* data;
		size_t size;
		size_t at = 0;

		size_t Left() const { return size - at; }
		bool Get32(uint32_t& value) {
			if (Left() < 4) return false;
			value = 0;
			for (int i = 0; i < 4; ++i)
				value |= static_cast<uint32_t>(data[at + i]) << (8 * i);
			at += 4;
			return true;
		}
		bool Get64(uint64_t& value) {
			if (Left() < 8) return false;
			value = 0;
			for (int i = 0; i < 8; ++i)
				value |= static_cast<uint64_t>(data[at + i]) << (8 * i);
			at += 8;
			return true;
		}
		template <typename T>
		bool GetBytes(T& out, size_t limit) {
			uint32_t count = 0;
			if (!Get32(count) || count > limit || Left() < count) return false;
			out.assign(data + at, data + at + count);
			at += count;
			return true;
		}
	};

	struct Section {
		uint32_t version = 0;
		std::vector<unsigned char> payload;
	};

	using Sections = std::map<uint32_t, Section>;

	inline std::vector<unsigned char> Build(const std::string& identity, const Sections& sections) {
		std::vector<unsigned char> out;
		Put32(out, kMagic);
		Put32(out, kFormat);
		PutBytes(out, identity.data(), identity.size());
		Put32(out, static_cast<uint32_t>(sections.size()));
		Put64(out, Fnv64(out.data(), out.size()));
		for (const auto& [id, section] : sections) {
			Put32(out, id);
			Put32(out, section.version);
			Put64(out, Fnv64(section.payload.data(), section.payload.size()));
			PutBytes(out, section.payload.data(), section.payload.size());
		}
		return out;
	}

	enum class Status { Ok, Damaged, OtherFormat, OtherIdentity };

	struct Parsed {
		Status status = Status::Damaged;
		// Sections whose checksum and version matched.
		Sections sections;
		// Section ids dropped for a damaged body or a different version.
		std::vector<uint32_t> dropped;
		// Bytes after the last readable section were cut off.
		bool truncated = false;
	};

	// Accept a section only at the version its owner expects; unknown ids are dropped.
	inline Parsed Parse(const std::vector<unsigned char>& bytes, const std::string& identity,
		const std::map<uint32_t, uint32_t>& versions) {
		Parsed result;
		if (bytes.size() > kMaxFile) return result;
		Reader in{ bytes.data(), bytes.size() };
		uint32_t magic = 0;
		uint32_t format = 0;
		std::string stored;
		uint32_t count = 0;
		uint64_t headerSum = 0;
		if (!in.Get32(magic) || magic != kMagic) return result;
		if (!in.Get32(format)) return result;
		if (format != kFormat) {
			result.status = Status::OtherFormat;
			return result;
		}
		if (!in.GetBytes(stored, kMaxIdentity) || !in.Get32(count)) return result;
		const size_t headerEnd = in.at;
		if (!in.Get64(headerSum) || headerSum != Fnv64(bytes.data(), headerEnd) || count > kMaxSections)
			return result;
		if (stored != identity) {
			result.status = Status::OtherIdentity;
			return result;
		}
		result.status = Status::Ok;
		for (uint32_t i = 0; i < count; ++i) {
			uint32_t id = 0;
			uint32_t version = 0;
			uint64_t sum = 0;
			Section section;
			if (!in.Get32(id) || !in.Get32(version) || !in.Get64(sum) || !in.GetBytes(section.payload, kMaxFile)) {
				result.truncated = true;
				break;
			}
			const auto wanted = versions.find(id);
			if (wanted == versions.end() || wanted->second != version || result.sections.count(id)
				|| sum != Fnv64(section.payload.data(), section.payload.size())) {
				result.dropped.push_back(id);
				continue;
			}
			section.version = version;
			result.sections.emplace(id, std::move(section));
		}
		if (!result.truncated && in.Left() != 0) result.truncated = true;
		return result;
	}
}
