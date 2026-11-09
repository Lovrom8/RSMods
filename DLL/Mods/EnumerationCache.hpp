#pragma once

#include "EnumerationPackage.hpp"

// Cache file-derived preparation data; ownership and engine registration remain native.
// Validate file identity and index hashes before reusing a record.
namespace EnumerationCache {
	// RSEC magic.
	constexpr uint32_t kMagic = 0x43455352;
	constexpr uint32_t kFormat = 1;
	constexpr uint32_t kFormatMinor = 2;
	constexpr uint32_t kMaxMapperRefs = 4u * 1024 * 1024;
	constexpr uint32_t kMaxRecords = 100000;
	constexpr uint32_t kMaxKey = 4096;
	constexpr size_t kMaxFile = 64u * 1024 * 1024;

	namespace Format {
		inline void Put32(std::vector<unsigned char>& out, uint32_t value) {
			const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
			out.insert(out.end(), bytes, bytes + sizeof(value));
		}
		inline void Put64(std::vector<unsigned char>& out, uint64_t value) {
			const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
			out.insert(out.end(), bytes, bytes + sizeof(value));
		}
		inline void PutBytes(std::vector<unsigned char>& out, const void* data, size_t size) {
			Put32(out, static_cast<uint32_t>(size));
			const auto* bytes = static_cast<const unsigned char*>(data);
			out.insert(out.end(), bytes, bytes + size);
		}

		struct Reader {
			const unsigned char* data;
			size_t size;
			size_t at = 0;
			bool Get32(uint32_t& value) {
				if (size - at < sizeof(value)) return false;
				std::memcpy(&value, data + at, sizeof(value));
				at += sizeof(value);
				return true;
			}
			bool Get64(uint64_t& value) {
				if (size - at < sizeof(value)) return false;
				std::memcpy(&value, data + at, sizeof(value));
				at += sizeof(value);
				return true;
			}
			template <typename T>
			bool GetBytes(T& out, uint32_t limit) {
				uint32_t count = 0;
				if (!Get32(count) || count > limit || size - at < count) return false;
				out.assign(data + at, data + at + count);
				at += count;
				return true;
			}
		};
	}

	inline std::vector<unsigned char> Serialize(const std::string& identity,
		const std::vector<const EnumerationPackage::Record*>& records) {
		std::vector<unsigned char> out;
		Format::Put32(out, kMagic);
		Format::Put32(out, kFormat);
		Format::Put32(out, kFormatMinor);
		Format::PutBytes(out, identity.data(), identity.size());
		Format::Put32(out, static_cast<uint32_t>(records.size()));
		for (const auto* record : records) {
			Format::PutBytes(out, record->key.data(), record->key.size());
			Format::Put64(out, record->indexHash);
			Format::Put32(out, record->appidIndex);
			Format::Put32(out, record->costUs);
			Format::PutBytes(out, record->toc.data(), record->toc.size());
			Format::PutBytes(out, record->names.data(), record->names.size());
			Format::PutBytes(out, record->appid.data(), record->appid.size());
			Format::PutBytes(out, record->mapperRefs.data(), record->mapperRefs.size());
		}
		Format::Put64(out, EnumerationPackage::Fnv64(out.data(), out.size()));
		return out;
	}

	// Reject any malformed file and report the reason to the caller.
	inline bool Deserialize(const std::vector<unsigned char>& bytes, const std::string& identity,
		std::vector<EnumerationPackage::Record>& records, std::string& reason) {
		records.clear();
		if (bytes.size() < 8 + 4 + 4 + 8 || bytes.size() > kMaxFile) {
			reason = "bad size";
			return false;
		}
		const size_t body = bytes.size() - 8;
		uint64_t trailer = 0;
		std::memcpy(&trailer, bytes.data() + body, sizeof(trailer));
		if (trailer != EnumerationPackage::Fnv64(bytes.data(), body)) {
			reason = "damaged (trailer mismatch)";
			return false;
		}
		Format::Reader in{ bytes.data(), body };
		uint32_t magic = 0;
		uint32_t format = 0;
		uint32_t minor = 0;
		std::string stored;
		if (!in.Get32(magic) || magic != kMagic || !in.Get32(format) || format != kFormat
			|| !in.Get32(minor) || minor != kFormatMinor) {
			reason = "different format";
			return false;
		}
		if (!in.GetBytes(stored, kMaxKey)) {
			reason = "damaged identity";
			return false;
		}
		if (stored != identity) {
			reason = "built for " + stored + ", now " + identity;
			return false;
		}
		uint32_t count = 0;
		if (!in.Get32(count) || count > kMaxRecords) {
			reason = "damaged record count";
			return false;
		}
		records.resize(count);
		for (auto& record : records) {
			if (!in.GetBytes(record.key, kMaxKey) || !in.Get64(record.indexHash)
				|| !in.Get32(record.appidIndex) || !in.Get32(record.costUs)
				|| !in.GetBytes(record.toc, EnumerationPackage::kMaxIndexBytes)
				|| !in.GetBytes(record.names, EnumerationPackage::kMaxIndexBytes)
				|| !in.GetBytes(record.appid, 511)
				|| !in.GetBytes(record.mapperRefs, kMaxMapperRefs)
				|| record.key.find('|') == std::string::npos) {
				records.clear();
				reason = "damaged record";
				return false;
			}
		}
		if (in.at != body) {
			records.clear();
			reason = "trailing bytes";
			return false;
		}
		return true;
	}

	inline std::string_view PathOf(std::string_view key) {
		return key.substr(0, key.find('|'));
	}

	enum class Miss { None, New, Changed, Content };

	// Thread-safe initialization for preparation workers; runs once per process.
	void Load();
	std::vector<std::vector<unsigned char>> AppIds();
	size_t LoadedCount();
	const EnumerationPackage::Record* Find(const std::string& key, Miss& miss);
	void NoteHit(const EnumerationPackage::Record& record, long long ticks);
	void NoteMiss(const std::string& key, Miss miss);
	void Remember(EnumerationPackage::Record record, long long ticks);
	// Mapper records require a matching file key and validated index hash.
	const std::vector<unsigned char>* FindMapperRefs(const std::string& key, uint64_t indexHash);
	void StoreMapperRefs(const std::string& key, uint64_t indexHash, std::vector<unsigned char> refs);
	uint64_t BeginMapperRefsScan();
	void MapperRefsScanEnded(uint64_t generation);
	// Save only from the last worker of a completed job.
	void Complete(const std::vector<std::string>& queuedPaths, uint64_t generation);
}
