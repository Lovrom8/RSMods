#pragma once

#include <Windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

// File data and CNG objects only; no VFS, platform, engine allocator or intern-table access.
namespace EnumerationPackage {
	using Inflate = int(__cdecl*)(void*, unsigned long*, const void*, unsigned long);
	constexpr uint32_t kMaxIndexBytes = 2 * 1024 * 1024;

	inline long long Clock() {
		return 0;
	}

	struct FileCloser {
		void operator()(void* file) const { CloseHandle(file); }
	};
	using File = std::unique_ptr<void, FileCloser>;

	struct Hash {
		std::string name;
		std::array<unsigned char, 16> digest{};
	};

	struct Package {
		File file;
		std::vector<unsigned char> toc;
		std::vector<unsigned char> names;
		std::vector<Hash> hashes;
		std::vector<unsigned char> appid;
		uint32_t appidIndex = 0;
		long long ioTicks = 0;
		long long parseTicks = 0;
		long long hashTicks = 0;
		uint64_t fileSize = 0;
		uint64_t writeTime = 0;
		uint64_t indexHash = 0;
	};

	inline uint32_t Big32(const unsigned char* data) {
		return (uint32_t(data[0]) << 24) | (uint32_t(data[1]) << 16)
			| (uint32_t(data[2]) << 8) | data[3];
	}

	inline uint64_t Big40(const unsigned char* data) {
		return (uint64_t(data[0]) << 32) | Big32(data + 1);
	}

	class Crypto {
	public:
		Crypto() {
			if (BCryptOpenAlgorithmProvider(&aes_, BCRYPT_AES_ALGORITHM, nullptr, 0) < 0)
				return;
			if (BCryptSetProperty(aes_, BCRYPT_CHAINING_MODE,
				reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_ECB)),
				sizeof(BCRYPT_CHAIN_MODE_ECB), 0) < 0)
				return;
			// Native PC PSARC key with a zero IV and CFB128 feedback.
			unsigned char key[] = {
				0xC5, 0x3D, 0xB2, 0x38, 0x70, 0xA1, 0xA2, 0xF7,
				0x1C, 0xAE, 0x64, 0x06, 0x1F, 0xDD, 0x0E, 0x11,
				0x57, 0x30, 0x9D, 0xC8, 0x52, 0x04, 0xD4, 0xC5,
				0xBF, 0xDF, 0x25, 0x09, 0x0D, 0xF2, 0x57, 0x2C
			};
			if (BCryptGenerateSymmetricKey(aes_, &key_, nullptr, 0, key, sizeof(key), 0) < 0)
				return;
			ready_ = BCryptOpenAlgorithmProvider(&md5_, BCRYPT_MD5_ALGORITHM, nullptr, 0) >= 0;
		}
		~Crypto() {
			if (key_) BCryptDestroyKey(key_);
			if (aes_) BCryptCloseAlgorithmProvider(aes_, 0);
			if (md5_) BCryptCloseAlgorithmProvider(md5_, 0);
		}
		Crypto(const Crypto&) = delete;
		Crypto& operator=(const Crypto&) = delete;

		bool Decrypt(unsigned char* data, size_t size) const {
			if (!ready_) return false;
			unsigned char feedback[16]{};
			for (size_t offset = 0; offset < size; offset += 16) {
				unsigned char mask[16]{};
				ULONG written = 0;
				if (BCryptEncrypt(key_, feedback, 16, nullptr, nullptr, 0,
					mask, 16, &written, 0) < 0 || written != 16)
					return false;
				const size_t count = (std::min)(size - offset, size_t(16));
				for (size_t i = 0; i < count; ++i) {
					feedback[i] = data[offset + i];
					data[offset + i] ^= mask[i];
				}
			}
			return true;
		}

		bool Digest(std::string_view text, std::array<unsigned char, 16>& digest) const {
			BCRYPT_HASH_HANDLE hash = nullptr;
			if (!ready_ || BCryptCreateHash(md5_, &hash, nullptr, 0, nullptr, 0, 0) < 0)
				return false;
			const bool ok = BCryptHashData(hash,
				reinterpret_cast<PUCHAR>(const_cast<char*>(text.data())),
				static_cast<ULONG>(text.size()), 0) >= 0
				&& BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) >= 0;
			BCryptDestroyHash(hash);
			return ok;
		}
	private:
		BCRYPT_ALG_HANDLE aes_ = nullptr;
		BCRYPT_ALG_HANDLE md5_ = nullptr;
		BCRYPT_KEY_HANDLE key_ = nullptr;
		bool ready_ = false;
	};

	inline bool Read(Package& package, uint64_t offset, void* dest, uint32_t count) {
		if (offset > package.fileSize || count > package.fileSize - offset)
			return false;
		const long long start = Clock();
		LARGE_INTEGER position{};
		position.QuadPart = static_cast<LONGLONG>(offset);
		DWORD read = 0;
		const bool ok = SetFilePointerEx(package.file.get(), position, nullptr, FILE_BEGIN)
			&& ReadFile(package.file.get(), dest, count, &read, nullptr) && read == count;
		package.ioTicks += Clock() - start;
		return ok;
	}

	inline bool ReadEntry(Package& package, uint32_t index, uint32_t tocSize,
		uint32_t entries, Inflate inflate, std::vector<unsigned char>& out) {
		const auto* entry = package.toc.data() + 32 + index * 30;
		const uint64_t length = Big40(entry + 20);
		uint64_t offset = Big40(entry + 25);
		uint32_t block = Big32(entry + 16);
		const uint32_t blocks = (tocSize - 32 - entries * 30) / 2;
		if (length > kMaxIndexBytes || offset < tocSize || block >= blocks)
			return false;
		out.resize(static_cast<size_t>(length));
		std::vector<unsigned char> compressed(65536);
		for (size_t done = 0; done < out.size(); ++block) {
			if (block >= blocks) return false;
			const auto* size = package.toc.data() + 32 + entries * 30 + block * 2;
			const uint32_t stored = (uint32_t(size[0]) << 8) | size[1];
			const uint32_t count = stored ? stored : 65536;
			const uint32_t wanted = static_cast<uint32_t>((std::min)(out.size() - done, size_t(65536)));
			if (!Read(package, offset, compressed.data(), count)) return false;
			if (!stored || count == wanted) {
				std::memcpy(out.data() + done, compressed.data(), wanted);
			}
			else {
				unsigned long expanded = wanted;
				if (!inflate || inflate(out.data() + done, &expanded, compressed.data(), count) != 0
					|| expanded != wanted)
					return false;
			}
			offset += count;
			done += wanted;
		}
		return true;
	}

	// 64-bit FNV-1a over raw encrypted index bytes.
	inline uint64_t Fnv64(const unsigned char* data, size_t size, uint64_t hash = 14695981039346656037ull) {
		for (size_t i = 0; i < size; ++i) {
			hash ^= data[i];
			hash *= 1099511628211ull;
		}
		return hash;
	}

	inline bool Open(const std::wstring& path, Package& package) {
		const long long start = Clock();
		// Retain the write-denying handle until installation ends to keep prepared bytes valid.
		const HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
			nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
		if (file == INVALID_HANDLE_VALUE) return false;
		package.file.reset(file);
		BY_HANDLE_FILE_INFORMATION info{};
		if (!GetFileInformationByHandle(file, &info)) return false;
		package.fileSize = (uint64_t(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
		package.writeTime = (uint64_t(info.ftLastWriteTime.dwHighDateTime) << 32)
			| info.ftLastWriteTime.dwLowDateTime;
		package.ioTicks += Clock() - start;
		return true;
	}

	inline bool ReadIndex(Package& package, unsigned char (&header)[32]) {
		if (!Read(package, 0, header, 32)) return false;
		const uint32_t tocSize = Big32(header + 12);
		const uint32_t entries = Big32(header + 20);
		if (Big32(header) != 0x50534152 || Big32(header + 4) != 0x10004
			|| Big32(header + 8) != 0x7A6C6962 || Big32(header + 16) != 30
			|| Big32(header + 24) != 65536 || Big32(header + 28) != 4
			|| tocSize < 32 || tocSize > kMaxIndexBytes || entries < 2
			|| entries > (tocSize - 32) / 30 || (tocSize - 32 - entries * 30) % 2)
			return false;
		package.toc.resize(tocSize);
		if (!Read(package, 32, package.toc.data() + 32, tocSize - 32)) return false;
		package.indexHash = Fnv64(package.toc.data() + 32, tocSize - 32, Fnv64(header, 32));
		return true;
	}

	// Native header words are host-endian; TOC entries remain big-endian.
	inline void NativeHeader(Package& package, const unsigned char (&header)[32]) {
		for (size_t i = 0; i < 8; ++i) {
			const uint32_t value = Big32(header + i * 4);
			std::memcpy(package.toc.data() + i * 4, &value, sizeof(value));
		}
	}

	inline bool Parse(Package& package, const unsigned char (&header)[32], Inflate inflate) {
		const uint32_t tocSize = Big32(header + 12);
		const uint32_t entries = Big32(header + 20);
		const long long parseStart = Clock();
		const long long ioAtParse = package.ioTicks;
		Crypto crypto;
		if (!crypto.Decrypt(package.toc.data() + 32, tocSize - 32)) return false;
		NativeHeader(package, header);
		if (!ReadEntry(package, 0, tocSize, entries, inflate, package.names)) return false;
		if (std::find(package.names.begin(), package.names.end(), 0) != package.names.end())
			return false;
		package.names.push_back(0);
		std::string_view names(reinterpret_cast<const char*>(package.names.data()), package.names.size() - 1);
		uint32_t index = 1;
		while (!names.empty()) {
			const size_t newline = names.find('\n');
			const auto name = names.substr(0, newline);
			if (name.empty() || index >= entries) return false;
			Hash hash;
			hash.name = name;
			const long long hashStart = Clock();
			if (!crypto.Digest(name, hash.digest)) return false;
			package.hashTicks += Clock() - hashStart;
			// Validate before calling the native constructor, whose tree builder is unchecked.
			if (std::memcmp(hash.digest.data(), package.toc.data() + 32 + index * 30, 16))
				return false;
			if (name == "appid.appid") package.appidIndex = index;
			package.hashes.push_back(std::move(hash));
			++index;
			if (newline == std::string_view::npos) break;
			names.remove_prefix(newline + 1);
		}
		if (index != entries || !package.appidIndex
			|| !ReadEntry(package, package.appidIndex, tocSize, entries, inflate, package.appid)
			|| package.appid.empty() || package.appid.size() > 511)
			return false;
		std::sort(package.hashes.begin(), package.hashes.end(),
			[](const Hash& a, const Hash& b) { return a.name < b.name; });
		package.parseTicks += Clock() - parseStart - package.hashTicks
			- (package.ioTicks - ioAtParse);
		return true;
	}

	inline bool Prepare(const std::wstring& path, Inflate inflate, Package& package) {
		unsigned char header[32]{};
		return Open(path, package) && ReadIndex(package, header) && Parse(package, header, inflate);
	}

	// Cache only file-derived data, excluding ownership, VFS and engine state.
	struct Record {
		// Cache key: path, size and write time.
		std::string key;
		uint64_t indexHash = 0;
		uint32_t appidIndex = 0;
		// Parse duration is zero outside Debug builds.
		uint32_t costUs = 0;
		std::vector<unsigned char> toc;
		// Excludes the file-list terminator.
		std::vector<unsigned char> names;
		std::vector<unsigned char> appid;
		// Opaque mapper records bound to this package index hash; empty if unavailable.
		std::vector<unsigned char> mapperRefs;
	};

	inline Record Capture(const Package& package, std::string key, uint32_t costUs) {
		Record record;
		record.key = std::move(key);
		record.indexHash = package.indexHash;
		record.appidIndex = package.appidIndex;
		record.costUs = costUs;
		record.toc = package.toc;
		if (!package.names.empty())
			record.names.assign(package.names.begin(), package.names.end() - 1);
		record.appid = package.appid;
		return record;
	}

	// Validate the cached TOC shape before native use.
	// Cache-only restoration checks path, size and mtime without reopening the file.
	inline bool Restore(Package& package, const unsigned char (&header)[32], const Record& record);
	inline bool RestoreCached(Package& package, const Record& record) {
		if (record.toc.size() < 32) return false;
		uint32_t tocSize = 0, entries = 0;
		std::memcpy(&tocSize, record.toc.data() + 12, sizeof(tocSize));
		std::memcpy(&entries, record.toc.data() + 20, sizeof(entries));
		if (tocSize != record.toc.size() || entries < 2 || 32 + uint64_t(entries) * 30 > tocSize) return false;
		unsigned char header[32]{};
		for (size_t i = 0; i < 8; ++i) {
			uint32_t value = 0;
			std::memcpy(&value, record.toc.data() + i * 4, sizeof(value));
			header[i * 4] = static_cast<unsigned char>(value >> 24);
			header[i * 4 + 1] = static_cast<unsigned char>(value >> 16);
			header[i * 4 + 2] = static_cast<unsigned char>(value >> 8);
			header[i * 4 + 3] = static_cast<unsigned char>(value);
		}
		package.indexHash = record.indexHash;
		return Restore(package, header, record);
	}

	inline bool Restore(Package& package, const unsigned char (&header)[32], const Record& record) {
		const uint32_t tocSize = Big32(header + 12);
		const uint32_t entries = Big32(header + 20);
		if (record.indexHash != package.indexHash || record.toc.size() != tocSize
			|| record.appidIndex == 0 || record.appidIndex >= entries
			|| record.appid.empty() || record.appid.size() > 511
			|| record.names.empty() || record.names.size() > kMaxIndexBytes
			|| std::find(record.names.begin(), record.names.end(), 0) != record.names.end())
			return false;
		std::vector<Hash> hashes;
		hashes.reserve(entries - 1);
		std::string_view names(reinterpret_cast<const char*>(record.names.data()), record.names.size());
		uint32_t index = 1;
		while (!names.empty()) {
			const size_t newline = names.find('\n');
			const auto name = names.substr(0, newline);
			if (name.empty() || index >= entries) return false;
			if ((index == record.appidIndex) != (name == "appid.appid")) return false;
			Hash hash;
			hash.name = name;
			std::memcpy(hash.digest.data(), record.toc.data() + 32 + index * 30, 16);
			hashes.push_back(std::move(hash));
			++index;
			if (newline == std::string_view::npos) break;
			names.remove_prefix(newline + 1);
		}
		if (index != entries) return false;
		std::sort(hashes.begin(), hashes.end(),
			[](const Hash& a, const Hash& b) { return a.name < b.name; });
		package.toc = record.toc;
		NativeHeader(package, header);
		package.names.assign(record.names.begin(), record.names.end());
		package.names.push_back(0);
		package.hashes = std::move(hashes);
		package.appid = record.appid;
		package.appidIndex = record.appidIndex;
		return true;
	}
}
