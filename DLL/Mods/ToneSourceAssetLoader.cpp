#include "../stdafx.h"
#include "ToneSourceAssetLoader.hpp"

#include "ToneSourceTimeline.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <set>
#include <system_error>
#include <unordered_map>

#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

#ifndef MZ_ASSERT
#define MZ_ASSERT(x) ((void)0)
#endif
#include "../Lib/Miniz/miniz_tinfl.h"

namespace {
	using QCAutomation::ToneSourceAssetLoader::ManifestArrangementDescriptor;
	using QCAutomation::ToneSourceTimeline::ArrangementToneTimeline;
	using QCAutomation::ToneSourceTimeline::ResolveToneNameFromToneId;
	using QCAutomation::ToneSourceTimeline::ToneEvent;

	constexpr const char* kLogPrefix = "[QCAutomation] ";
	constexpr uint32_t kPsarcTocEncryptedFlag = 0x4;
	constexpr uint32_t kPsarcHeaderSize = 32;
	constexpr uint32_t kExpectedPsarcBlockSize = 0x10000;
	constexpr int kMinToneId = 0;
	constexpr int kMaxToneId = 3;
	constexpr std::size_t kMaxCandidateLogs = 8;
	constexpr std::size_t kSngHeaderSize = 24;
	constexpr uint32_t kSngMagic = 0x4A;
	constexpr uint32_t kSngCompressedFlag = 1;
	constexpr std::size_t kMaxSngEncryptedPayloadSize = 64ULL * 1024ULL * 1024ULL;
	constexpr uint32_t kMaxSngUncompressedSize = 64U * 1024U * 1024U;
	constexpr int32_t kMaxToneEventCount = 500000;

	constexpr std::array<uint8_t, 32> kPsarcKey{
		0xC5, 0x3D, 0xB2, 0x38, 0x70, 0xA1, 0xA2, 0xF7,
		0x1C, 0xAE, 0x64, 0x06, 0x1F, 0xDD, 0x0E, 0x11,
		0x57, 0x30, 0x9D, 0xC8, 0x52, 0x04, 0xD4, 0xC5,
		0xBF, 0xDF, 0x25, 0x09, 0x0D, 0xF2, 0x57, 0x2C
	};

	constexpr std::array<uint8_t, 32> kSngKeyPc{
		0xCB, 0x64, 0x8D, 0xF3, 0xD1, 0x2A, 0x16, 0xBF,
		0x71, 0x70, 0x14, 0x14, 0xE6, 0x96, 0x19, 0xEC,
		0x17, 0x1C, 0xCA, 0x5D, 0x2A, 0x14, 0x2E, 0x3E,
		0x59, 0xDE, 0x7A, 0xDD, 0xA1, 0x8A, 0x3A, 0x30
	};

	struct PsarcEntry {
		uint32_t startBlock = 0;
		uint64_t length = 0;
		uint64_t offset = 0;
		std::string path;
		std::string pathLower;
	};

	class AesEcb256 {
	public:
		~AesEcb256() {
			if (keyHandle_ != nullptr) {
				BCryptDestroyKey(keyHandle_);
				keyHandle_ = nullptr;
			}
			if (algHandle_ != nullptr) {
				BCryptCloseAlgorithmProvider(algHandle_, 0);
				algHandle_ = nullptr;
			}
		}

		bool Initialize(const std::array<uint8_t, 32>& key, std::string& error) {
			const NTSTATUS openStatus = BCryptOpenAlgorithmProvider(&algHandle_, BCRYPT_AES_ALGORITHM, nullptr, 0);
			if (!BCRYPT_SUCCESS(openStatus)) {
				error = "BCryptOpenAlgorithmProvider(AES) failed";
				return false;
			}

			const wchar_t* mode = BCRYPT_CHAIN_MODE_ECB;
			const NTSTATUS modeStatus = BCryptSetProperty(
				algHandle_,
				BCRYPT_CHAINING_MODE,
				reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(mode)),
				static_cast<ULONG>((wcslen(mode) + 1) * sizeof(wchar_t)),
				0);
			if (!BCRYPT_SUCCESS(modeStatus)) {
				error = "BCryptSetProperty(CHAIN_MODE_ECB) failed";
				return false;
			}

			ULONG keyObjectSize = 0;
			ULONG cbResult = 0;
			const NTSTATUS objectLengthStatus = BCryptGetProperty(
				algHandle_,
				BCRYPT_OBJECT_LENGTH,
				reinterpret_cast<PUCHAR>(&keyObjectSize),
				sizeof(keyObjectSize),
				&cbResult,
				0);
			if (!BCRYPT_SUCCESS(objectLengthStatus) || keyObjectSize == 0) {
				error = "BCryptGetProperty(OBJECT_LENGTH) failed";
				return false;
			}

			keyObject_.assign(keyObjectSize, 0);

			const NTSTATUS keyStatus = BCryptGenerateSymmetricKey(
				algHandle_,
				&keyHandle_,
				keyObject_.data(),
				static_cast<ULONG>(keyObject_.size()),
				const_cast<PUCHAR>(key.data()),
				static_cast<ULONG>(key.size()),
				0);
			if (!BCRYPT_SUCCESS(keyStatus)) {
				error = "BCryptGenerateSymmetricKey failed";
				return false;
			}

			return true;
		}

		bool EncryptBlock(const uint8_t in[16], uint8_t out[16], std::string& error) const {
			if (keyHandle_ == nullptr) {
				error = "AES key handle not initialized";
				return false;
			}

			uint8_t inputBlock[16];
			std::memcpy(inputBlock, in, sizeof(inputBlock));

			ULONG bytesWritten = 0;
			const NTSTATUS status = BCryptEncrypt(
				keyHandle_,
				inputBlock,
				sizeof(inputBlock),
				nullptr,
				nullptr,
				0,
				out,
				16,
				&bytesWritten,
				0);
			if (!BCRYPT_SUCCESS(status) || bytesWritten != 16) {
				error = "BCryptEncrypt(block) failed";
				return false;
			}

			return true;
		}

	private:
		BCRYPT_ALG_HANDLE algHandle_ = nullptr;
		BCRYPT_KEY_HANDLE keyHandle_ = nullptr;
		std::vector<uint8_t> keyObject_;
	};

	class BinaryCursor {
	public:
		explicit BinaryCursor(const std::vector<uint8_t>& data)
			: data_(data) {}

		std::size_t Remaining() const {
			if (position_ >= data_.size()) {
				return 0;
			}
			return data_.size() - position_;
		}

		bool Skip(std::size_t byteCount) {
			if (byteCount > Remaining()) {
				return false;
			}
			position_ += byteCount;
			return true;
		}

		bool ReadUInt32(uint32_t& out) {
			if (Remaining() < 4) {
				return false;
			}

			out = static_cast<uint32_t>(data_[position_ + 0]) |
				(static_cast<uint32_t>(data_[position_ + 1]) << 8) |
				(static_cast<uint32_t>(data_[position_ + 2]) << 16) |
				(static_cast<uint32_t>(data_[position_ + 3]) << 24);
			position_ += 4;
			return true;
		}

		bool ReadInt32(int32_t& out) {
			uint32_t tmp = 0;
			if (!ReadUInt32(tmp)) {
				return false;
			}
			out = static_cast<int32_t>(tmp);
			return true;
		}

		bool ReadFloat(float& out) {
			uint32_t raw = 0;
			if (!ReadUInt32(raw)) {
				return false;
			}

			std::memcpy(&out, &raw, sizeof(out));
			return true;
		}

	private:
		const std::vector<uint8_t>& data_;
		std::size_t position_ = 0;
	};

	class PsarcReader {
	public:
		bool Open(const std::filesystem::path& path, std::string& error) {
			path_ = path;
			file_.open(path, std::ios::binary);
			if (!file_.is_open()) {
				error = "failed to open PSARC";
				return false;
			}

			std::array<uint8_t, kPsarcHeaderSize> header{};
			if (!ReadExact(file_, header.data(), header.size())) {
				error = "failed to read PSARC header";
				return false;
			}

			const std::string identifier(reinterpret_cast<const char*>(header.data()), 4);
			if (identifier != "PSAR") {
				error = "invalid PSARC identifier";
				return false;
			}

			const uint32_t tocSize = ReadBE32(header.data() + 12);
			const uint32_t tocEntrySize = ReadBE32(header.data() + 16);
			const uint32_t entryCount = ReadBE32(header.data() + 20);
			blockSize_ = ReadBE32(header.data() + 24);
			const uint32_t archiveFlags = ReadBE32(header.data() + 28);

			if (entryCount == 0 || tocEntrySize == 0 || blockSize_ == 0 || tocSize < kPsarcHeaderSize) {
				error = "invalid PSARC header values";
				return false;
			}

			if (blockSize_ != kExpectedPsarcBlockSize) {
				// Keep running, but this helps diagnostics if Ubisoft changes layout assumptions.
				LOG_WARNING(kLogPrefix << "Unexpected PSARC block size: " << blockSize_ << std::endl);
			}

			std::vector<uint8_t> tocRaw(tocSize);
			file_.seekg(static_cast<std::streamoff>(kPsarcHeaderSize), std::ios::beg);
			if (!ReadExact(file_, tocRaw.data(), tocRaw.size())) {
				error = "failed to read PSARC TOC";
				return false;
			}

			std::vector<uint8_t> tocData;
			if ((archiveFlags & kPsarcTocEncryptedFlag) != 0) {
				std::string decryptError;
				if (!DecryptAesCfb128(tocRaw, kPsarcKey, tocData, decryptError)) {
					error = std::string("failed to decrypt PSARC TOC: ") + decryptError;
					return false;
				}
			}
			else {
				tocData = std::move(tocRaw);
			}

			if (!ParseToc(tocData, entryCount, tocEntrySize, tocSize, error)) {
				return false;
			}

			if (!LoadNamesBlock(error)) {
				return false;
			}

			return true;
		}

		const std::vector<PsarcEntry>& Entries() const {
			return entries_;
		}

		std::optional<std::size_t> FindEntryIndexByPath(std::string_view targetPath) const {
			const std::string normalized = NormalizeArchivePath(targetPath);
			const auto it = entryIndexByPath_.find(normalized);
			if (it != entryIndexByPath_.end()) {
				return it->second;
			}
			return std::nullopt;
		}

		bool InflateEntry(std::size_t index, std::vector<uint8_t>& outData, std::string& error) {
			if (index >= entries_.size()) {
				error = "entry index out of range";
				return false;
			}

			const PsarcEntry& entry = entries_[index];
			if (entry.length == 0) {
				outData.clear();
				return true;
			}

			const uint64_t blockCount = (entry.length + static_cast<uint64_t>(blockSize_) - 1) / static_cast<uint64_t>(blockSize_);
			if (blockCount == 0) {
				outData.clear();
				return true;
			}

			if (entry.startBlock >= zipBlockSizes_.size()) {
				error = "entry startBlock out of zip block size range";
				return false;
			}

			const uint64_t lastBlock = static_cast<uint64_t>(entry.startBlock) + blockCount - 1;
			if (lastBlock >= zipBlockSizes_.size()) {
				error = "entry lastBlock out of zip block size range";
				return false;
			}

			file_.seekg(static_cast<std::streamoff>(entry.offset), std::ios::beg);
			if (!file_) {
				error = "failed to seek to entry offset";
				return false;
			}

			outData.clear();
			outData.reserve(static_cast<std::size_t>(entry.length));

			for (uint64_t blockIndex = static_cast<uint64_t>(entry.startBlock); blockIndex <= lastBlock; ++blockIndex) {
				const uint32_t zipBlockSize = zipBlockSizes_[static_cast<std::size_t>(blockIndex)];

				if (zipBlockSize == 0) {
					std::vector<uint8_t> rawBlock(blockSize_);
					if (!ReadExact(file_, rawBlock.data(), rawBlock.size())) {
						error = "failed to read raw PSARC block";
						return false;
					}

					outData.insert(outData.end(), rawBlock.begin(), rawBlock.end());
					continue;
				}

				std::vector<uint8_t> block(zipBlockSize);
				if (!ReadExact(file_, block.data(), block.size())) {
					error = "failed to read compressed PSARC block";
					return false;
				}

				if (LooksLikeZlibBlock(block)) {
					std::vector<uint8_t> uncompressed;
					std::string inflateError;
					if (!InflateZlibBlock(block, blockSize_, uncompressed, inflateError)) {
						error = std::string("failed to inflate PSARC zlib block: ") + inflateError;
						return false;
					}

					outData.insert(outData.end(), uncompressed.begin(), uncompressed.end());
				}
				else {
					outData.insert(outData.end(), block.begin(), block.end());
				}
			}

			if (outData.size() < entry.length) {
				error = "inflated PSARC entry shorter than declared length";
				return false;
			}

			outData.resize(static_cast<std::size_t>(entry.length));
			return true;
		}

	private:
		static uint32_t ReadBE16(const uint8_t* ptr) {
			return (static_cast<uint32_t>(ptr[0]) << 8) |
				static_cast<uint32_t>(ptr[1]);
		}

		static uint32_t ReadBE24(const uint8_t* ptr) {
			return (static_cast<uint32_t>(ptr[0]) << 16) |
				(static_cast<uint32_t>(ptr[1]) << 8) |
				static_cast<uint32_t>(ptr[2]);
		}

		static uint32_t ReadBE32(const uint8_t* ptr) {
			return (static_cast<uint32_t>(ptr[0]) << 24) |
				(static_cast<uint32_t>(ptr[1]) << 16) |
				(static_cast<uint32_t>(ptr[2]) << 8) |
				static_cast<uint32_t>(ptr[3]);
		}

		static uint64_t ReadBE40(const uint8_t* ptr) {
			return (static_cast<uint64_t>(ptr[0]) << 32) |
				(static_cast<uint64_t>(ptr[1]) << 24) |
				(static_cast<uint64_t>(ptr[2]) << 16) |
				(static_cast<uint64_t>(ptr[3]) << 8) |
				static_cast<uint64_t>(ptr[4]);
		}

		static bool ReadExact(std::ifstream& stream, void* buffer, std::size_t size) {
			stream.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(size));
			return stream.good() || stream.gcount() == static_cast<std::streamsize>(size);
		}

		static bool LooksLikeZlibBlock(const std::vector<uint8_t>& data) {
			if (data.size() < 2) {
				return false;
			}
			return data[0] == 0x78;
		}

		static bool InflateZlibBlock(const std::vector<uint8_t>& compressed, uint32_t blockSize, std::vector<uint8_t>& outData, std::string& error) {
			if (blockSize == 0) {
				error = "invalid target inflate size";
				return false;
			}

			outData.assign(blockSize, 0);
			const std::size_t written = tinfl_decompress_mem_to_mem(
				outData.data(),
				outData.size(),
				compressed.data(),
				compressed.size(),
				TINFL_FLAG_PARSE_ZLIB_HEADER);
			if (written == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
				error = "tinfl_decompress_mem_to_mem failed";
				return false;
			}

			outData.resize(written);
			return true;
		}

		static std::string NormalizeArchivePath(std::string_view path) {
			std::string normalized;
			normalized.reserve(path.size());
			for (const char c : path) {
				const char mapped = (c == '\\') ? '/' : c;
				normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(mapped))));
			}
			return normalized;
		}

		static std::vector<std::string> SplitLines(const std::string& text) {
			std::vector<std::string> lines;
			std::string current;
			current.reserve(128);

			for (const char c : text) {
				if (c == '\n') {
					if (!current.empty() && current.back() == '\r') {
						current.pop_back();
					}
					lines.push_back(current);
					current.clear();
					continue;
				}
				current.push_back(c);
			}

			if (!current.empty()) {
				if (!current.empty() && current.back() == '\r') {
					current.pop_back();
				}
				lines.push_back(current);
			}

			return lines;
		}

		bool ParseToc(
			const std::vector<uint8_t>& tocData,
			uint32_t entryCount,
			uint32_t tocEntrySize,
			uint32_t tocSize,
			std::string& error) {
			const uint64_t tocChunkSize = static_cast<uint64_t>(entryCount) * static_cast<uint64_t>(tocEntrySize);
			if (tocChunkSize > tocData.size()) {
				error = "PSARC TOC entry table out of bounds";
				return false;
			}

			entries_.clear();
			entries_.reserve(entryCount);

			std::size_t cursor = 0;
			for (uint32_t i = 0; i < entryCount; ++i) {
				if (cursor + tocEntrySize > tocData.size() || tocEntrySize < 30) {
					error = "PSARC TOC entry parse overflow";
					return false;
				}

				const uint8_t* entryPtr = tocData.data() + cursor;
				PsarcEntry entry;
				entry.startBlock = ReadBE32(entryPtr + 16);
				entry.length = ReadBE40(entryPtr + 20);
				entry.offset = ReadBE40(entryPtr + 25);
				entries_.push_back(std::move(entry));

				cursor += tocEntrySize;
			}

			uint32_t blockSizeTmp = blockSize_;
			int bytesPerZipSize = 0;
			while (blockSizeTmp > 1 && blockSizeTmp % 256 == 0) {
				blockSizeTmp /= 256;
				bytesPerZipSize++;
			}
			if (bytesPerZipSize < 2 || bytesPerZipSize > 4) {
				error = "unsupported PSARC bytesPerZipSize";
				return false;
			}

			if (tocSize < kPsarcHeaderSize + tocChunkSize) {
				error = "invalid PSARC TOC size";
				return false;
			}

			const uint64_t zipByteCount = static_cast<uint64_t>(tocSize) - kPsarcHeaderSize - tocChunkSize;
			if (zipByteCount % static_cast<uint64_t>(bytesPerZipSize) != 0) {
				error = "PSARC zip block table byte count misaligned";
				return false;
			}

			const std::size_t zipBlockCount = static_cast<std::size_t>(zipByteCount / static_cast<uint64_t>(bytesPerZipSize));
			zipBlockSizes_.clear();
			zipBlockSizes_.reserve(zipBlockCount);

			for (std::size_t i = 0; i < zipBlockCount; ++i) {
				if (cursor + static_cast<std::size_t>(bytesPerZipSize) > tocData.size()) {
					error = "PSARC zip block table parse overflow";
					return false;
				}

				uint32_t zipSize = 0;
				if (bytesPerZipSize == 2) {
					zipSize = ReadBE16(tocData.data() + cursor);
				}
				else if (bytesPerZipSize == 3) {
					zipSize = ReadBE24(tocData.data() + cursor);
				}
				else {
					zipSize = ReadBE32(tocData.data() + cursor);
				}

				zipBlockSizes_.push_back(zipSize);
				cursor += static_cast<std::size_t>(bytesPerZipSize);
			}

			return true;
		}

		bool LoadNamesBlock(std::string& error) {
			if (entries_.empty()) {
				error = "PSARC has no entries";
				return false;
			}

			std::vector<uint8_t> namesBlock;
			if (!InflateEntry(0, namesBlock, error)) {
				error = std::string("failed to inflate NamesBlock: ") + error;
				return false;
			}

			const std::string namesText(reinterpret_cast<const char*>(namesBlock.data()), namesBlock.size());
			const std::vector<std::string> names = SplitLines(namesText);

			entryIndexByPath_.clear();
			for (std::size_t i = 0; i < names.size() && (i + 1) < entries_.size(); ++i) {
				entries_[i + 1].path = names[i];
				entries_[i + 1].pathLower = NormalizeArchivePath(entries_[i + 1].path);
				entryIndexByPath_[entries_[i + 1].pathLower] = i + 1;
			}

			return true;
		}

		static bool DecryptAesCfb128(
			const std::vector<uint8_t>& cipherBytes,
			const std::array<uint8_t, 32>& key,
			std::vector<uint8_t>& plainBytes,
			std::string& error) {
			AesEcb256 aes;
			if (!aes.Initialize(key, error)) {
				return false;
			}

			std::array<uint8_t, 16> feedback{};
			plainBytes.assign(cipherBytes.size(), 0);

			std::size_t offset = 0;
			while (offset < cipherBytes.size()) {
				uint8_t keystream[16]{};
				if (!aes.EncryptBlock(feedback.data(), keystream, error)) {
					return false;
				}

				const std::size_t blockLen = std::min<std::size_t>(16, cipherBytes.size() - offset);
				for (std::size_t i = 0; i < blockLen; ++i) {
					plainBytes[offset + i] = static_cast<uint8_t>(cipherBytes[offset + i] ^ keystream[i]);
				}

				if (blockLen == 16) {
					std::memcpy(feedback.data(), cipherBytes.data() + offset, 16);
				}

				offset += blockLen;
			}

			return true;
		}

		std::filesystem::path path_;
		std::ifstream file_;
		uint32_t blockSize_ = 0;
		std::vector<PsarcEntry> entries_;
		std::vector<uint32_t> zipBlockSizes_;
		std::unordered_map<std::string, std::size_t> entryIndexByPath_;
	};

	std::unordered_map<std::string, std::vector<ManifestArrangementDescriptor>> manifestDescriptorsByPsarc;
	std::unordered_map<std::string, std::string> manifestLoadErrorByPsarc;
	std::unordered_map<std::string, ArrangementToneTimeline> timelineByDescriptorKey;
	std::unordered_map<std::string, std::string> psarcHintBySongKey;

	std::filesystem::path cachedGameRoot;
	std::vector<std::filesystem::path> cachedPsarcFileList;

	std::string ReadStringField(const nlohmann::json& node, const std::initializer_list<const char*> keys) {
		if (!node.is_object()) {
			return "";
		}

		for (const char* key : keys) {
			const auto it = node.find(key);
			if (it == node.end()) {
				continue;
			}
			if (it->is_string()) {
				return it->get<std::string>();
			}
		}

		return "";
	}

	std::string ToLowerAscii(std::string_view value) {
		std::string lowered;
		lowered.reserve(value.size());
		for (const char c : value) {
			lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		}
		return lowered;
	}

	bool EndsWithCaseInsensitive(std::string_view value, std::string_view suffix) {
		if (suffix.size() > value.size()) {
			return false;
		}

		const std::string valueSuffix = ToLowerAscii(value.substr(value.size() - suffix.size()));
		return valueSuffix == ToLowerAscii(suffix);
	}

	std::string NormalizeIdentifier(std::string_view value) {
		std::string normalized;
		normalized.reserve(value.size());
		for (const char c : value) {
			const unsigned char uc = static_cast<unsigned char>(c);
			if (std::isalnum(uc) || c == '-' || c == '_') {
				normalized.push_back(static_cast<char>(std::toupper(uc)));
			}
		}
		return normalized;
	}

	std::string NormalizeSongKeyForPathMatch(std::string_view value) {
		std::string normalized;
		normalized.reserve(value.size());
		for (const char c : value) {
			const unsigned char uc = static_cast<unsigned char>(c);
			if (std::isalnum(uc)) {
				normalized.push_back(static_cast<char>(std::tolower(uc)));
			}
		}
		return normalized;
	}

	std::string NormalizeArchivePath(std::string_view value) {
		std::string normalized;
		normalized.reserve(value.size());
		for (const char c : value) {
			char mapped = c;
			if (mapped == '\\') {
				mapped = '/';
			}
			normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(mapped))));
		}
		return normalized;
	}

	std::filesystem::path GetGameRootPath() {
		char exePath[MAX_PATH]{};
		const DWORD len = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
		if (len == 0 || len >= MAX_PATH) {
			std::error_code ec;
			const auto fallback = std::filesystem::current_path(ec);
			if (!ec) {
				return fallback;
			}
			return std::filesystem::path(".");
		}

		return std::filesystem::path(exePath).parent_path();
	}

	std::string BuildSngPathFromSongXml(const std::string& songXmlRaw) {
		if (songXmlRaw.empty()) {
			return "";
		}

		std::string songXml = NormalizeArchivePath(songXmlRaw);
		while (!songXml.empty() && songXml.front() == '/') {
			songXml.erase(songXml.begin());
		}

		if (songXml.empty()) {
			return "";
		}

		if (EndsWithCaseInsensitive(songXml, ".sng")) {
			return songXml;
		}

		// RockSniffer uses SongXml.Substring(20) and prefixes songs/bin/generic/.
		std::string tail;
		if (songXml.size() > 20) {
			tail = songXml.substr(20);
		}
		else {
			const std::size_t lastSlash = songXml.find_last_of('/');
			tail = (lastSlash == std::string::npos) ? songXml : songXml.substr(lastSlash + 1);
		}

		if (EndsWithCaseInsensitive(tail, ".xml")) {
			tail = tail.substr(0, tail.size() - 4);
		}

		while (!tail.empty() && tail.front() == '/') {
			tail.erase(tail.begin());
		}
		while (!tail.empty() && tail.back() == '/') {
			tail.pop_back();
		}

		if (tail.empty()) {
			return "";
		}

		return "songs/bin/generic/" + tail + ".sng";
	}

	void PushDescriptorFromAttributes(
		const nlohmann::json& attributesNode,
		const std::string& outerSongKey,
		const std::string& outerArrangementName,
		const std::filesystem::path& psarcPath,
		std::vector<ManifestArrangementDescriptor>& outDescriptors,
		bool requireToneNames) {
		if (!attributesNode.is_object()) {
			return;
		}

		ManifestArrangementDescriptor descriptor;
		descriptor.psarcPath = psarcPath;
		descriptor.songKey = ReadStringField(attributesNode, { "SongKey", "songKey", "songID", "songId" });
		if (descriptor.songKey.empty()) {
			descriptor.songKey = outerSongKey;
		}

		descriptor.arrangementID = ReadStringField(attributesNode, { "arrangementID", "arrangementId", "ArrangementID", "ArrangementId" });
		descriptor.persistentID = ReadStringField(attributesNode, { "persistentID", "PersistentID" });
		descriptor.arrangementName = ReadStringField(attributesNode, { "arrangementName", "ArrangementName", "name", "Name" });
		if (descriptor.arrangementName.empty()) {
			descriptor.arrangementName = outerArrangementName;
		}
		descriptor.arrangementType = ReadStringField(attributesNode, { "arrangementType", "ArrangementType", "pathType", "PathType" });
		descriptor.arrangementIDNormalized = NormalizeIdentifier(descriptor.arrangementID);
		descriptor.persistentIDNormalized = NormalizeIdentifier(descriptor.persistentID);

		descriptor.toneBase = ReadStringField(attributesNode, { "toneBase", "ToneBase", "Tone_Base" });
		descriptor.toneA = ReadStringField(attributesNode, { "toneA", "ToneA", "Tone_A" });
		descriptor.toneB = ReadStringField(attributesNode, { "toneB", "ToneB", "Tone_B" });
		descriptor.toneC = ReadStringField(attributesNode, { "toneC", "ToneC", "Tone_C" });
		descriptor.toneD = ReadStringField(attributesNode, { "toneD", "ToneD", "Tone_D" });

		descriptor.songXmlPath = ReadStringField(attributesNode, { "songXml", "SongXml", "songXML", "SongXML" });
		descriptor.sngPath = BuildSngPathFromSongXml(descriptor.songXmlPath);

		const bool hasAnyArrangementId = !descriptor.arrangementIDNormalized.empty() || !descriptor.persistentIDNormalized.empty();
		const bool hasAnyToneNames = !descriptor.toneBase.empty() || !descriptor.toneA.empty() || !descriptor.toneB.empty() || !descriptor.toneC.empty() || !descriptor.toneD.empty();

		if (!descriptor.songKey.empty() && hasAnyArrangementId && (!requireToneNames || hasAnyToneNames) && !descriptor.sngPath.empty()) {
			outDescriptors.push_back(std::move(descriptor));
		}
	}

	void CollectDescriptorsFromManifestJson(
		const nlohmann::json& rootNode,
		const std::filesystem::path& psarcPath,
		std::vector<ManifestArrangementDescriptor>& outDescriptors,
		bool requireToneNames = true) {
		if (rootNode.is_array()) {
			for (const auto& item : rootNode) {
				CollectDescriptorsFromManifestJson(item, psarcPath, outDescriptors, requireToneNames);
			}
			return;
		}

		if (!rootNode.is_object()) {
			return;
		}

		const auto entriesIt = rootNode.find("Entries");
		if (entriesIt != rootNode.end() && entriesIt->is_object()) {
			for (const auto& [songKey, arrangementMap] : entriesIt->items()) {
				if (!arrangementMap.is_object()) {
					continue;
				}

				for (const auto& [arrangementName, arrangementAttributes] : arrangementMap.items()) {
					PushDescriptorFromAttributes(arrangementAttributes, songKey, arrangementName, psarcPath, outDescriptors, requireToneNames);
				}
			}
		}

		const auto attributesIt = rootNode.find("Attributes");
		if (attributesIt != rootNode.end() && attributesIt->is_object()) {
			PushDescriptorFromAttributes(*attributesIt, "", "", psarcPath, outDescriptors, requireToneNames);
		}
		else {
			PushDescriptorFromAttributes(rootNode, "", "", psarcPath, outDescriptors, requireToneNames);
		}
	}

	void RebuildPsarcFileListIfNeeded() {
		const std::filesystem::path gameRoot = GetGameRootPath();
		if (gameRoot == cachedGameRoot && !cachedPsarcFileList.empty()) {
			return;
		}

		cachedGameRoot = gameRoot;
		cachedPsarcFileList.clear();

		const std::filesystem::path songsPsarc = gameRoot / "songs.psarc";
		std::error_code songsEc;
		if (std::filesystem::exists(songsPsarc, songsEc) && !songsEc) {
			cachedPsarcFileList.push_back(songsPsarc);
		}

		const std::filesystem::path dlcPath = gameRoot / "dlc";
		std::error_code dlcEc;
		if (std::filesystem::exists(dlcPath, dlcEc) && !dlcEc &&
			std::filesystem::is_directory(dlcPath, dlcEc) && !dlcEc) {
			std::vector<std::filesystem::path> dlcFiles;
			for (std::filesystem::directory_iterator it(dlcPath, dlcEc), end; it != end && !dlcEc; it.increment(dlcEc)) {
				const auto& entry = *it;
				if (!entry.is_regular_file(dlcEc) || dlcEc) {
					dlcEc.clear();
					continue;
				}

				const std::filesystem::path filePath = entry.path();
				if (EndsWithCaseInsensitive(filePath.filename().string(), ".psarc")) {
					dlcFiles.push_back(filePath);
				}
			}

			std::sort(dlcFiles.begin(), dlcFiles.end());
			cachedPsarcFileList.insert(cachedPsarcFileList.end(), dlcFiles.begin(), dlcFiles.end());
		}
	}

	bool EnsureManifestCachedForPsarc(const std::filesystem::path& psarcPath, std::string& outError) {
		const std::string psarcKey = psarcPath.string();
		const auto cachedDescriptorsIt = manifestDescriptorsByPsarc.find(psarcKey);
		if (cachedDescriptorsIt != manifestDescriptorsByPsarc.end()) {
			outError.clear();
			return true;
		}

		const auto cachedErrorIt = manifestLoadErrorByPsarc.find(psarcKey);
		if (cachedErrorIt != manifestLoadErrorByPsarc.end()) {
			outError = cachedErrorIt->second;
			return false;
		}

		PsarcReader reader;
		std::string openError;
		if (!reader.Open(psarcPath, openError)) {
			outError = openError;
			manifestLoadErrorByPsarc[psarcKey] = outError;
			return false;
		}

		std::vector<ManifestArrangementDescriptor> descriptors;
		for (std::size_t i = 1; i < reader.Entries().size(); ++i) {
			const PsarcEntry& entry = reader.Entries()[i];
			if (entry.pathLower.empty() || !EndsWithCaseInsensitive(entry.pathLower, ".json")) {
				continue;
			}

			std::vector<uint8_t> jsonBytes;
			std::string inflateError;
			if (!reader.InflateEntry(i, jsonBytes, inflateError)) {
				continue;
			}

			nlohmann::json root;
			try {
				root = nlohmann::json::parse(jsonBytes.begin(), jsonBytes.end(), nullptr, false);
			}
			catch (...) {
				continue;
			}

			if (root.is_discarded()) {
				continue;
			}

			CollectDescriptorsFromManifestJson(root, psarcPath, descriptors);
		}

		manifestDescriptorsByPsarc.emplace(psarcKey, std::move(descriptors));
		outError.clear();
		return true;
	}

	std::string BuildCandidateSummary(const std::vector<ManifestArrangementDescriptor>& candidates) {
		if (candidates.empty()) {
			return "none";
		}

		std::ostringstream oss;
		const std::size_t count = std::min(candidates.size(), kMaxCandidateLogs);
		for (std::size_t i = 0; i < count; ++i) {
			const ManifestArrangementDescriptor& candidate = candidates[i];
			if (i != 0) {
				oss << ", ";
			}

			oss << "{arrangementID='" << candidate.arrangementID
				<< "', persistentID='" << candidate.persistentID
				<< "', psarc='" << candidate.psarcPath.filename().string() << "'}";
		}

		if (candidates.size() > count) {
			oss << ", ...";
		}

		return oss.str();
	}

	std::optional<std::size_t> FindSngEntryIndex(const PsarcReader& reader, const std::string& desiredPath) {
		if (const auto exactMatch = reader.FindEntryIndexByPath(desiredPath)) {
			return exactMatch;
		}

		const std::string desiredLower = NormalizeArchivePath(desiredPath);
		const std::filesystem::path desiredFilePath(desiredLower);
		const std::string desiredFilename = desiredFilePath.filename().string();
		if (desiredFilename.empty()) {
			return std::nullopt;
		}

		std::optional<std::size_t> onlyMatch;
		for (std::size_t i = 0; i < reader.Entries().size(); ++i) {
			const PsarcEntry& entry = reader.Entries()[i];
			if (entry.pathLower.empty()) {
				continue;
			}

			const std::filesystem::path entryPath(entry.pathLower);
			if (entryPath.filename().string() == desiredFilename) {
				if (onlyMatch.has_value()) {
					return std::nullopt;
				}
				onlyMatch = i;
			}
		}

		return onlyMatch;
	}

	void IncrementCounter(std::array<uint8_t, 16>& counter) {
		for (int i = static_cast<int>(counter.size()) - 1; i >= 0; --i) {
			counter[static_cast<std::size_t>(i)] = static_cast<uint8_t>(counter[static_cast<std::size_t>(i)] + 1);
			if (counter[static_cast<std::size_t>(i)] != 0) {
				break;
			}
		}
	}

	bool AesCtrTransform(
		const std::array<uint8_t, 32>& key,
		const std::array<uint8_t, 16>& iv,
		const std::vector<uint8_t>& input,
		std::vector<uint8_t>& output,
		std::string& error) {
		AesEcb256 aes;
		if (!aes.Initialize(key, error)) {
			return false;
		}

		std::array<uint8_t, 16> counter = iv;
		output.assign(input.size(), 0);

		std::size_t offset = 0;
		while (offset < input.size()) {
			uint8_t keystream[16]{};
			if (!aes.EncryptBlock(counter.data(), keystream, error)) {
				return false;
			}

			const std::size_t blockLen = std::min<std::size_t>(16, input.size() - offset);
			for (std::size_t i = 0; i < blockLen; ++i) {
				output[offset + i] = static_cast<uint8_t>(input[offset + i] ^ keystream[i]);
			}

			IncrementCounter(counter);
			offset += blockLen;
		}

		return true;
	}

	bool SkipCountedStructArray(BinaryCursor& cursor, std::size_t elementSize, const char* sectionName, std::string& error) {
		int32_t count = 0;
		if (!cursor.ReadInt32(count)) {
			error = std::string("failed to read count for section: ") + sectionName;
			return false;
		}
		if (count < 0) {
			error = std::string("negative count in section: ") + sectionName;
			return false;
		}

		const uint64_t byteCount = static_cast<uint64_t>(count) * static_cast<uint64_t>(elementSize);
		if (byteCount > cursor.Remaining()) {
			error = std::string("section overflow while skipping: ") + sectionName;
			return false;
		}

		return cursor.Skip(static_cast<std::size_t>(byteCount));
	}

	bool ParseToneEventsFromDecodedSng(
		const std::vector<uint8_t>& decodedSng,
		std::vector<ToneEvent>& outToneEvents,
		std::size_t& outIgnoredOutOfRangeToneIds,
		std::string& error) {
		BinaryCursor cursor(decodedSng);

		if (!SkipCountedStructArray(cursor, 16, "BPMs", error)) return false;
		if (!SkipCountedStructArray(cursor, 44, "Phrases", error)) return false;
		if (!SkipCountedStructArray(cursor, 72, "Chords", error)) return false;
		if (!SkipCountedStructArray(cursor, 2376, "ChordNotes", error)) return false;

		int32_t vocalCount = 0;
		if (!cursor.ReadInt32(vocalCount) || vocalCount < 0) {
			error = "failed to read Vocals count";
			return false;
		}
		{
			const uint64_t vocalBytes = static_cast<uint64_t>(vocalCount) * 60ULL;
			if (vocalBytes > cursor.Remaining() || !cursor.Skip(static_cast<std::size_t>(vocalBytes))) {
				error = "Vocals section overflow";
				return false;
			}
		}

		if (vocalCount > 0) {
			if (!SkipCountedStructArray(cursor, 32, "SymbolHeaders", error)) return false;
			if (!SkipCountedStructArray(cursor, 144, "SymbolTextures", error)) return false;
			if (!SkipCountedStructArray(cursor, 44, "SymbolDefinitions", error)) return false;
		}

		if (!SkipCountedStructArray(cursor, 24, "PhraseIterations", error)) return false;
		if (!SkipCountedStructArray(cursor, 16, "PhraseExtraInfoByLevel", error)) return false;

		int32_t nldCount = 0;
		if (!cursor.ReadInt32(nldCount) || nldCount < 0) {
			error = "failed to read NLinkedDifficulty count";
			return false;
		}
		for (int32_t i = 0; i < nldCount; ++i) {
			int32_t levelBreak = 0;
			int32_t phraseCount = 0;
			if (!cursor.ReadInt32(levelBreak) || !cursor.ReadInt32(phraseCount) || phraseCount < 0) {
				error = "failed to read NLinkedDifficulty entry";
				return false;
			}

			(void)levelBreak;
			const uint64_t phraseBytes = static_cast<uint64_t>(phraseCount) * 4ULL;
			if (phraseBytes > cursor.Remaining() || !cursor.Skip(static_cast<std::size_t>(phraseBytes))) {
				error = "NLinkedDifficulty phrase array overflow";
				return false;
			}
		}

		if (!SkipCountedStructArray(cursor, 260, "Actions", error)) return false;
		if (!SkipCountedStructArray(cursor, 260, "Events", error)) return false;

		int32_t toneCount = 0;
		if (!cursor.ReadInt32(toneCount) || toneCount < 0) {
			error = "failed to read Tones count";
			return false;
		}
		if (toneCount > kMaxToneEventCount) {
			std::ostringstream oversize;
			oversize << "Tones count is too large (" << toneCount
				<< ", max supported " << kMaxToneEventCount << ")";
			error = oversize.str();
			return false;
		}

		outToneEvents.clear();
		outIgnoredOutOfRangeToneIds = 0;
		try {
			outToneEvents.reserve(static_cast<std::size_t>(toneCount));
		}
		catch (const std::exception& ex) {
			error = std::string("failed to reserve tone event vector: ") + ex.what();
			return false;
		}
		catch (...) {
			error = "failed to reserve tone event vector: unknown exception";
			return false;
		}

		for (int32_t i = 0; i < toneCount; ++i) {
			float timeSec = 0.0f;
			int32_t toneId = 0;
			if (!cursor.ReadFloat(timeSec) || !cursor.ReadInt32(toneId)) {
				error = "failed to read Tone event";
				return false;
			}

			if (toneId < kMinToneId || toneId > kMaxToneId) {
				outIgnoredOutOfRangeToneIds++;
				continue;
			}

			ToneEvent toneEvent;
			toneEvent.timeSec = timeSec;
			toneEvent.toneIdDebug = toneId;
			outToneEvents.push_back(std::move(toneEvent));
		}

		std::stable_sort(outToneEvents.begin(), outToneEvents.end(), [](const ToneEvent& lhs, const ToneEvent& rhs) {
			return lhs.timeSec < rhs.timeSec;
		});

		return true;
	}

	bool DecodeSngEntry(
		const std::vector<uint8_t>& sngEntryBytes,
		std::vector<uint8_t>& outDecodedSng,
		std::string& error) {
		if (sngEntryBytes.size() < kSngHeaderSize) {
			error = "SNG entry is too small";
			return false;
		}

		const uint32_t magic = static_cast<uint32_t>(sngEntryBytes[0]) |
			(static_cast<uint32_t>(sngEntryBytes[1]) << 8) |
			(static_cast<uint32_t>(sngEntryBytes[2]) << 16) |
			(static_cast<uint32_t>(sngEntryBytes[3]) << 24);
		if (magic != kSngMagic) {
			error = "invalid SNG magic";
			return false;
		}

		const uint32_t flags = static_cast<uint32_t>(sngEntryBytes[4]) |
			(static_cast<uint32_t>(sngEntryBytes[5]) << 8) |
			(static_cast<uint32_t>(sngEntryBytes[6]) << 16) |
			(static_cast<uint32_t>(sngEntryBytes[7]) << 24);

		std::array<uint8_t, 16> iv{};
		std::memcpy(iv.data(), sngEntryBytes.data() + 8, iv.size());

		const std::size_t encryptedPayloadSize = sngEntryBytes.size() - kSngHeaderSize;
		if (encryptedPayloadSize > kMaxSngEncryptedPayloadSize) {
			std::ostringstream oversize;
			oversize << "encrypted SNG payload is too large (" << encryptedPayloadSize
				<< " bytes, max supported " << kMaxSngEncryptedPayloadSize << ")";
			error = oversize.str();
			return false;
		}

		std::vector<uint8_t> encryptedPayload(sngEntryBytes.begin() + static_cast<std::ptrdiff_t>(kSngHeaderSize), sngEntryBytes.end());
		std::vector<uint8_t> decryptedPayload;
		std::string decryptError;
		if (!AesCtrTransform(kSngKeyPc, iv, encryptedPayload, decryptedPayload, decryptError)) {
			error = std::string("failed to AES-CTR decrypt SNG payload: ") + decryptError;
			return false;
		}

		if ((flags & kSngCompressedFlag) == 0) {
			outDecodedSng = std::move(decryptedPayload);
			return true;
		}

		if (decryptedPayload.size() < 4) {
			error = "compressed SNG payload too small";
			return false;
		}

		const uint32_t uncompressedSize = static_cast<uint32_t>(decryptedPayload[0]) |
			(static_cast<uint32_t>(decryptedPayload[1]) << 8) |
			(static_cast<uint32_t>(decryptedPayload[2]) << 16) |
			(static_cast<uint32_t>(decryptedPayload[3]) << 24);
		if (uncompressedSize == 0) {
			error = "compressed SNG reported zero output size";
			return false;
		}
		if (uncompressedSize > kMaxSngUncompressedSize) {
			std::ostringstream oversize;
			oversize << "compressed SNG reported oversized output (" << uncompressedSize
				<< " bytes, max supported " << kMaxSngUncompressedSize << ")";
			error = oversize.str();
			return false;
		}

		std::vector<uint8_t> compressedData(decryptedPayload.begin() + 4, decryptedPayload.end());
		outDecodedSng.assign(uncompressedSize, 0);
		const std::size_t written = tinfl_decompress_mem_to_mem(
			outDecodedSng.data(),
			outDecodedSng.size(),
			compressedData.data(),
			compressedData.size(),
			TINFL_FLAG_PARSE_ZLIB_HEADER);
		if (written == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
			error = "tinfl_decompress_mem_to_mem(SNG) failed";
			return false;
		}

		outDecodedSng.resize(written);
		return true;
	}

	std::string BuildDescriptorKey(const ManifestArrangementDescriptor& descriptor) {
		std::ostringstream oss;
		oss << descriptor.psarcPath.string() << "|"
			<< descriptor.songKey << "|"
			<< descriptor.arrangementID << "|"
			<< descriptor.persistentID << "|"
			<< descriptor.arrangementName << "|"
			<< descriptor.arrangementType << "|"
			<< descriptor.sngPath;
		return oss.str();
	}

	bool BuildToneBaseFallbackTimelineFromDescriptorImpl(
		const ManifestArrangementDescriptor& descriptor,
		ArrangementToneTimeline& outTimeline) {
		outTimeline = {};
		outTimeline.songKey = descriptor.songKey;
		outTimeline.arrangementID = descriptor.arrangementID;
		outTimeline.persistentID = descriptor.persistentID;
		outTimeline.arrangementIDNormalized = descriptor.arrangementIDNormalized;
		outTimeline.persistentIDNormalized = descriptor.persistentIDNormalized;
		outTimeline.toneBase = descriptor.toneBase;
		outTimeline.toneA = descriptor.toneA;
		outTimeline.toneB = descriptor.toneB;
		outTimeline.toneC = descriptor.toneC;
		outTimeline.toneD = descriptor.toneD;

		// Fallback for malformed SNG timelines: preserve auto-follow using slot metadata only.
		if (outTimeline.toneBase.empty() && !outTimeline.toneA.empty()) {
			outTimeline.toneBase = outTimeline.toneA;
		}

		return !outTimeline.toneBase.empty() ||
			!outTimeline.toneA.empty() ||
			!outTimeline.toneB.empty() ||
			!outTimeline.toneC.empty() ||
			!outTimeline.toneD.empty();
	}

	bool BuildTimelineFromDescriptorImpl(
		const ManifestArrangementDescriptor& descriptor,
		ArrangementToneTimeline& outTimeline,
		std::string& outError) {
		try {
			const std::string timelineKey = BuildDescriptorKey(descriptor);
			const auto cachedTimelineIt = timelineByDescriptorKey.find(timelineKey);
			if (cachedTimelineIt != timelineByDescriptorKey.end()) {
				outTimeline = cachedTimelineIt->second;
				outError.clear();
				return true;
			}

			PsarcReader reader;
			std::string openError;
			if (!reader.Open(descriptor.psarcPath, openError)) {
				outError = std::string("failed to open PSARC for timeline load: ") + openError;
				return false;
			}

			const auto sngEntryIndex = FindSngEntryIndex(reader, descriptor.sngPath);
			if (!sngEntryIndex.has_value()) {
				outError = "failed to locate SNG entry in PSARC: " + descriptor.sngPath;
				return false;
			}

			std::vector<uint8_t> sngEntryBytes;
			std::string inflateError;
			if (!reader.InflateEntry(*sngEntryIndex, sngEntryBytes, inflateError)) {
				outError = std::string("failed to read SNG entry from PSARC: ") + inflateError;
				return false;
			}

			std::vector<uint8_t> decodedSng;
			std::string decodeError;
			if (!DecodeSngEntry(sngEntryBytes, decodedSng, decodeError)) {
				outError = std::string("failed to decode SNG entry: ") + decodeError;
				return false;
			}

			ArrangementToneTimeline timeline;
			timeline.songKey = descriptor.songKey;
			timeline.arrangementID = descriptor.arrangementID;
			timeline.persistentID = descriptor.persistentID;
			timeline.arrangementIDNormalized = descriptor.arrangementIDNormalized;
			timeline.persistentIDNormalized = descriptor.persistentIDNormalized;
			timeline.toneBase = descriptor.toneBase;
			timeline.toneA = descriptor.toneA;
			timeline.toneB = descriptor.toneB;
			timeline.toneC = descriptor.toneC;
			timeline.toneD = descriptor.toneD;

			std::size_t ignoredOutOfRange = 0;
			std::string parseError;
			if (!ParseToneEventsFromDecodedSng(decodedSng, timeline.toneEvents, ignoredOutOfRange, parseError)) {
				outError = std::string("failed to parse decoded SNG tone events: ") + parseError;
				return false;
			}
			timeline.ignoredOutOfRangeToneIdCount = ignoredOutOfRange;

			for (ToneEvent& event : timeline.toneEvents) {
				if (!event.toneIdDebug.has_value()) {
					continue;
				}

				bool ambiguousToneIdZero = false;
				const auto resolvedName = ResolveToneNameFromToneId(timeline, *event.toneIdDebug, ambiguousToneIdZero);
				if (resolvedName.has_value() && !resolvedName->empty()) {
					event.authoredToneName = *resolvedName;
				}
				else {
					timeline.unresolvedToneNameCount++;
				}

				if (ambiguousToneIdZero) {
					event.toneIdZeroAmbiguous = true;
					timeline.ambiguousToneIdZeroCount++;
				}
			}

			timelineByDescriptorKey[timelineKey] = timeline;
			outTimeline = std::move(timeline);
			outError.clear();
			return true;
		}
		catch (const std::exception& ex) {
			outError = std::string("exception while building timeline: ") + ex.what();
			return false;
		}
		catch (...) {
			outError = "unknown exception while building timeline";
			return false;
		}
	}

	bool TryMatchDescriptor(
		const ManifestArrangementDescriptor& descriptor,
		const std::string& runtimeArrangementIdNormalized,
		ManifestArrangementDescriptor& outPersistentIdMatch,
		bool& hasPersistentIdMatch,
		ManifestArrangementDescriptor& outArrangementIdMatch,
		bool& hasArrangementIdMatch) {
		if (!descriptor.persistentIDNormalized.empty() && descriptor.persistentIDNormalized == runtimeArrangementIdNormalized) {
			outPersistentIdMatch = descriptor;
			hasPersistentIdMatch = true;
			return true;
		}

		if (!descriptor.arrangementIDNormalized.empty() && descriptor.arrangementIDNormalized == runtimeArrangementIdNormalized && !hasArrangementIdMatch) {
			outArrangementIdMatch = descriptor;
			hasArrangementIdMatch = true;
		}

		return false;
	}

	bool ResolveDescriptorForCurrentSongImpl(
		const std::string& songKey,
		const std::string& arrangementHash,
		ManifestArrangementDescriptor& outDescriptor,
		std::string& outMatchedBy,
		std::string& outFailureReason) {
		RebuildPsarcFileListIfNeeded();

		if (cachedPsarcFileList.empty()) {
			outFailureReason = "no installed PSARC assets found (songs.psarc/dlc)";
			return false;
		}

		const std::string runtimeArrangementIdNormalized = NormalizeIdentifier(arrangementHash);
		if (runtimeArrangementIdNormalized.empty()) {
			outFailureReason = "runtime arrangement hash is empty or invalid";
			return false;
		}

		std::set<std::string> scannedPsarcs;
		std::vector<ManifestArrangementDescriptor> songCandidates;
		ManifestArrangementDescriptor persistentIdMatch;
		ManifestArrangementDescriptor arrangementIdMatch;
		bool hasPersistentIdMatch = false;
		bool hasArrangementIdMatch = false;

		auto scanPsarc = [&](const std::filesystem::path& psarcPath) {
			const std::string psarcKey = psarcPath.string();
			if (scannedPsarcs.find(psarcKey) != scannedPsarcs.end()) {
				return;
			}
			scannedPsarcs.insert(psarcKey);

			std::string manifestError;
			if (!EnsureManifestCachedForPsarc(psarcPath, manifestError)) {
				return;
			}

			const auto descriptorsIt = manifestDescriptorsByPsarc.find(psarcKey);
			if (descriptorsIt == manifestDescriptorsByPsarc.end()) {
				return;
			}

			for (const ManifestArrangementDescriptor& descriptor : descriptorsIt->second) {
				if (descriptor.songKey != songKey) {
					continue;
				}

				songCandidates.push_back(descriptor);
				if (TryMatchDescriptor(
					descriptor,
					runtimeArrangementIdNormalized,
					persistentIdMatch,
					hasPersistentIdMatch,
					arrangementIdMatch,
					hasArrangementIdMatch)) {
					return;
				}
			}
		};

		const auto hintIt = psarcHintBySongKey.find(songKey);
		if (hintIt != psarcHintBySongKey.end()) {
			scanPsarc(hintIt->second);
		}

		const std::string songKeyForPathMatch = NormalizeSongKeyForPathMatch(songKey);
		if (!songKeyForPathMatch.empty()) {
			for (const std::filesystem::path& psarcPath : cachedPsarcFileList) {
				if (hasPersistentIdMatch) {
					break;
				}

				const std::string fileNameLower = ToLowerAscii(psarcPath.filename().string());
				if (fileNameLower.find(songKeyForPathMatch) != std::string::npos) {
					scanPsarc(psarcPath);
				}
			}
		}

		for (const std::filesystem::path& psarcPath : cachedPsarcFileList) {
			if (hasPersistentIdMatch) {
				break;
			}

			if (EndsWithCaseInsensitive(psarcPath.filename().string(), "songs.psarc")) {
				scanPsarc(psarcPath);
			}
		}

		for (const std::filesystem::path& psarcPath : cachedPsarcFileList) {
			if (hasPersistentIdMatch) {
				break;
			}
			scanPsarc(psarcPath);
		}

		if (hasPersistentIdMatch) {
			outDescriptor = persistentIdMatch;
			outMatchedBy = "persistentID";
			psarcHintBySongKey[songKey] = persistentIdMatch.psarcPath.string();
			return true;
		}

		if (hasArrangementIdMatch) {
			outDescriptor = arrangementIdMatch;
			outMatchedBy = "arrangementID";
			psarcHintBySongKey[songKey] = arrangementIdMatch.psarcPath.string();
			return true;
		}

		std::ostringstream oss;
		oss << "no manifest descriptor match for song=" << songKey
			<< " arrangementHash='" << arrangementHash
			<< "' candidatesForSong=" << BuildCandidateSummary(songCandidates);
		outFailureReason = oss.str();
		return false;
	}
}

namespace QCAutomation::ToneSourceAssetLoader {
	bool ResolveDescriptorForCurrentSong(
		const std::string& songKey,
		const std::string& arrangementHash,
		ManifestArrangementDescriptor& outDescriptor,
		std::string& outMatchedBy,
		std::string& outFailureReason) {
		return ResolveDescriptorForCurrentSongImpl(songKey, arrangementHash, outDescriptor, outMatchedBy, outFailureReason);
	}

	bool BuildToneBaseFallbackTimelineFromDescriptor(
		const ManifestArrangementDescriptor& descriptor,
		ToneSourceTimeline::ArrangementToneTimeline& outTimeline) {
		return BuildToneBaseFallbackTimelineFromDescriptorImpl(descriptor, outTimeline);
	}

	bool BuildTimelineFromDescriptor(
		const ManifestArrangementDescriptor& descriptor,
		ToneSourceTimeline::ArrangementToneTimeline& outTimeline,
		std::string& outError) {
		return BuildTimelineFromDescriptorImpl(descriptor, outTimeline, outError);
	}
}
