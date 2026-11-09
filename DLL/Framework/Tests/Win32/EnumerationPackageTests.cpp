#include "../../../Mods/EnumerationFrameBudget.hpp"
#define NOMINMAX
#include "../../../Mods/EnumerationPackage.hpp"
#include "../../../Mods/EnumerationCache.hpp"
#include <cassert>
#include <cstdio>
#include <thread>
#define ENUMERATION_HOOK_ABI_TEST
#define ENUMERATION_PREPARATION_TEST
#include "../../../Mods/EnumerationPreparation.cpp"

namespace {
    unsigned int mapperNativeCalls = 0;
    uint32_t __cdecl MapperReference(const EnumerationMapperHash::EngineString* text) {
        ++mapperNativeCalls;
        const auto* data = text->storage == &text->finish ? text->data.local : text->data.heap;
        uint32_t result = 0x811C9DC5u;
        for (const auto* at = data; at != text->finish; ++at) {
            if (*at >= 0x80) return 0xFEEDFACEu;
            result ^= static_cast<uint32_t>(static_cast<signed char>(*at));
            result *= 16777619u;
        }
        return result;
    }
    uint32_t __declspec(naked) AbiMapperNative() {
        __asm { push edi }
        __asm { call MapperReference }
        __asm { add esp, 4 }
        __asm { ret }
    }
    uint32_t __declspec(naked) __cdecl AbiMapperHash(const EnumerationMapperHash::EngineString*) {
        __asm { push edi }
        __asm { mov edi, [esp + 8] }
        __asm { call EnumerationMapperHash::Hook }
        __asm { pop edi }
        __asm { ret }
    }
    void MapperHashChecks() {
        using namespace EnumerationMapperHash;
        native = reinterpret_cast<uintptr_t>(&AbiMapperNative);
        uint32_t hash = 0;
        assert(Ascii(nullptr, 0, hash) && hash == 0x811C9DC5u);
        const unsigned char hello[] = "hello";
        assert(Ascii(hello, 5, hash) && hash == 0x4F9F2CABu);
        for (size_t length = 0; length <= 256; ++length) {
            std::vector<unsigned char> bytes(length + 1, 0);
            for (size_t i = 0; i < length; ++i) bytes[i] = static_cast<unsigned char>(1 + (i * 53) % 127);
            EngineString text{};
            if (length < 16) {
                std::memcpy(text.data.local, bytes.data(), length);
                text.finish = text.data.local + length;
                text.storage = &text.finish;
            }
            else {
                text.data.heap = bytes.data();
                text.finish = bytes.data() + length;
                text.storage = bytes.data() + bytes.size();
            }
            assert(AbiMapperHash(&text) == MapperReference(&text));
        }
        const unsigned char utf8[] = {0xC3, 0xA9, 0};
        EngineString text{};
        text.data.heap = utf8;
        text.finish = utf8 + 2;
        text.storage = utf8 + 3;
        assert(!Ascii(utf8, 2, hash));
        const auto before = mapperNativeCalls;
        assert(AbiMapperHash(&text) == 0xFEEDFACEu && mapperNativeCalls == before + 1);
    }
    std::atomic<unsigned int> ownershipCalls = 0;
    bool __fastcall AbiOwnership(void*, void*, uint32_t) {
        ++ownershipCalls;
        return false;
    }
    bool __declspec(naked) AbiOwnershipDispatch(void*, uint32_t) {
        __asm { mov ecx, [esp + 4] }
        __asm { mov edx, offset AbiOwnership }
        __asm { push [esp + 8] }
        __asm { call EnumerationOwnership::Dispatch }
        __asm { ret }
    }
    void OwnershipChecks() {
        uintptr_t table[8]{};
        table[7] = reinterpret_cast<uintptr_t>(&AbiOwnership);
        void* apps = table;
        uintptr_t before = 0, after = 0;
        __asm { mov before, esp }
        // The thunk must pop the appid argument, matching the native ABI.
        assert(AbiOwnershipDispatch(&apps, 123));
        assert(AbiOwnershipDispatch(&apps, 124));
        __asm { mov after, esp }
        assert(before == after);
        assert(ownershipCalls == 0);
        assert(EnumerationOwnership::Answered() == 2);
    }
	void __declspec(naked) AbiIntern() {
		__asm { mov eax, 12345678h }
		__asm { ret }
	}
	__declspec(naked) void* __cdecl AbiBorrow(void*, int, const char*) {
		__asm { push ebp }
		__asm { mov ebp, esp }
		__asm { sub esp, 2Ch }
		__asm { mov eax, [ebp + 8] }
		__asm { mov [ebp - 2Ch], eax }
		__asm { mov ecx, [ebp + 12] }
		__asm { push [ebp + 16] }
		__asm { call BorrowInternHook }
		__asm { add esp, 4 }
		__asm { mov esp, ebp }
		__asm { pop ebp }
		__asm { ret }
	}
}

namespace {
	constexpr char kArchive[] =
		"50534152000100047a6c69620000007e0000001e000000030001000000000004"
		"9b350ff518b6649166fdeaa7dcbc69ed68119a7508db0c58d9fe7d09d6d16a10"
		"5b68f6e3a606ec1fc3f5f11e8fc9b1db3bcee31d3d965f71ad122deddc8f9b86"
		"e661c7036b457e6d303a630668705652f6defef50a48b7635bbed470c2f17"
		"89c4b2c28c84cd14b04915cb989799969a9c525c5faa91589b90539a9b868bdace2fc3c0091721673"
		"323231363830";
	constexpr char kCompressed[] =
		"789c4b2c28c84cd14b04915cb989799969a9c525c5faa91589b90539a9b868bdace2fc3c0091721673";
	constexpr std::string_view kNames =
		"appid.appid\nmanifests/example/example/example/example.json";

	std::vector<unsigned char> Unhex(std::string_view hex) {
		std::vector<unsigned char> bytes;
		for (size_t i = 0; i < hex.size(); i += 2) {
			const auto digit = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
			bytes.push_back(static_cast<unsigned char>(digit(hex[i]) * 16 + digit(hex[i + 1])));
		}
		return bytes;
	}

	int __cdecl FixtureInflate(void* out, unsigned long* size, const void* input,
		unsigned long count) {
		const auto compressed = Unhex(kCompressed);
		assert(count == compressed.size());
		assert(std::memcmp(input, compressed.data(), count) == 0);
		assert(*size == kNames.size());
		std::memcpy(out, kNames.data(), kNames.size());
		return 0;
	}

	void Write(const wchar_t* path, const std::vector<unsigned char>& data) {
		const HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		assert(file != INVALID_HANDLE_VALUE);
		DWORD count = 0;
		assert(WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &count, nullptr));
		assert(count == data.size());
		CloseHandle(file);
	}

	struct Stream {
		uintptr_t* table;
		unsigned int refs = 1;
	};
	void __fastcall AddRef(Stream* stream, void*) { ++stream->refs; }
	void __fastcall Release(Stream* stream, void*) { --stream->refs; }
	unsigned int parserFallbacks = 0;
	unsigned int readFallbacks = 0;
	unsigned int hashFallbacks = 0;
	void** __cdecl FallbackParse(void** out, void** toc, void* stream, char** names) {
		assert(out && toc && stream && names);
		*out = stream;
		*toc = nullptr;
		*names = nullptr;
		++parserFallbacks;
		return out;
	}
	void __declspec(naked) FallbackParser() {
		__asm {
			push [esp + 8]
			push [esp + 8]
			push edx
			push ecx
			call FallbackParse
			add esp, 16
			ret
		}
	}
	void __declspec(naked) FallbackReadBody() {
		__asm {
			inc dword ptr [readFallbacks]
			mov esp, ebp
			pop ebp
			mov eax, 1234h
			ret
		}
	}
	void __cdecl FallbackHash(void* out, const char* text, int size) {
		assert(std::string_view(text, size) == "uncached");
		std::memset(out, 0x5A, 16);
		++hashFallbacks;
	}
	void __declspec(naked) FallbackHashAbi() {
		__asm {
			push [esp + 4]
			push edx
			push ecx
			call FallbackHash
			add esp, 12
			ret
		}
	}
	void* openedStream = nullptr;
	bool __fastcall FallbackOpen(const char* path, void** out) {
		assert(std::string_view(path) == "test.psarc");
		*out = openedStream;
		return true;
	}

	void CheckNativeHandoff() {
		uintptr_t table[] = { 0, reinterpret_cast<uintptr_t>(&AddRef), reinterpret_cast<uintptr_t>(&Release) };
		Stream stream{ table };
		void* input = &stream;
		void* output = nullptr;
		void* toc = nullptr;
		char* names = nullptr;
		void** returned = nullptr;
		EnumerationPreparation::Prepared prepared;
		crtMalloc = &std::malloc;
		crtFree = &std::free;
		parserFunc = reinterpret_cast<uintptr_t>(&FallbackParser);
		hashFunc = reinterpret_cast<uintptr_t>(&FallbackHashAbi);
		readBody = reinterpret_cast<uintptr_t>(&FallbackReadBody);
		openFunc = reinterpret_cast<uintptr_t>(&FallbackOpen);
		prepared.toc.reset(static_cast<unsigned char*>(crtMalloc(256)));
		prepared.names.reset(static_cast<unsigned char*>(crtMalloc(16)));
		prepared.valid = true;
		prepared.package.appidIndex = 1;
		prepared.package.appid = { '2', '2', '1', '6', '8', '0' };
		EnumerationPackage::Hash hash;
		hash.name = "cached";
		hash.digest.fill(0xA5);
		prepared.package.hashes.push_back(hash);
		EnumerationPreparation::Begin(&prepared);
		openedStream = input;
		void* source = nullptr;
		assert(Open("test.psarc", &source) && source == input && prepared.sourceStream == input);
		Stream unrelated{ table };
		void* unrelatedOut = nullptr;
		void* unrelatedToc = nullptr;
		char* unrelatedNames = nullptr;
		Parse(&unrelatedOut, &unrelatedToc, &unrelated, &unrelatedNames);
		assert(unrelatedOut == &unrelated && !unrelatedToc && !unrelatedNames);
		assert(prepared.toc && prepared.names && parserFallbacks == 1);
		uintptr_t before = 0;
		uintptr_t after = 0;
		__asm {
			mov before, esp
			lea eax, names
			push eax
			push input
			lea edx, toc
			lea ecx, output
			call ParserHook
			add esp, 8
			mov returned, eax
			mov after, esp
		}
		assert(before == after && returned == &output && output == input);
		assert(toc && names && !prepared.toc && !prepared.names && stream.refs == 2);
		assert(parserFallbacks == 1);
		const auto* entry = static_cast<unsigned char*>(toc) + 62;
		char buffer[16]{};
		unsigned int read = 0;
		__asm {
			mov before, esp
			push 0
			push input
			push toc
			push 1
			push 3
			lea eax, buffer
			push eax
			mov ecx, entry
			call ReadHook
			add esp, 24
			mov read, eax
			mov after, esp
		}
		assert(before == after && read == 3 && std::string_view(buffer, 3) == "216");
		assert(stream.refs == 1 && !readFallbacks);
		for (const char* text : { "cached", "uncached" }) {
			const int hashSize = static_cast<int>(std::strlen(text));
			unsigned char digest[16]{};
			__asm {
				mov before, esp
				push hashSize
				mov edx, text
				lea ecx, digest
				call HashHook
				add esp, 4
				mov after, esp
			}
			assert(before == after);
			assert(digest[0] == (hashSize == 6 ? 0xA5 : 0x5A));
		}
		assert(hashFallbacks == 1);
		EnumerationPreparation::End();
		crtFree(toc);
		crtFree(names);
		CallParser(&output, &toc, input, &names);
		assert(parserFallbacks == 2 && !toc && !names);
		const char* path = "test.psarc";
		openedStream = nullptr;
		bool opened = false;
		__asm {
			mov before, esp
			mov ecx, path
			lea edx, output
			call OpenHook
			mov opened, al
			mov after, esp
		}
		assert(before == after && opened && !output);
		__asm {
			mov before, esp
			push 0
			push input
			push 0
			push 0
			push 3
			lea eax, buffer
			push eax
			xor ecx, ecx
			call ReadHook
			add esp, 24
			mov read, eax
			mov after, esp
		}
		assert(before == after && read == 0x1234 && readFallbacks == 1);
	}
}

int wmain() {
    OwnershipChecks();
    MapperHashChecks();
	assert(EnumerationDrain::FrameBudgetMs(false, 60, 8.0, 3.0) == 2.0);
	assert(std::abs(EnumerationDrain::FrameBudgetMs(true, 60, 13.0, 5.0) - (1000.0 / 60 - 8.0)) < 0.00001);
	assert(EnumerationDrain::FrameBudgetMs(true, 120, 12.0, 2.0) == 8.0);
	assert(std::abs(EnumerationDrain::FrameBudgetMs(true, 120, 7.0, 4.0) - (1000.0 / 120 - 3.0)) < 0.00001);
	assert(EnumerationDrain::FrameBudgetMs(true, 60, 1.0, 2.0) == 1000.0 / 60);

	assert(EnumerationDrain::FrameBudgetMs(true, 60, 32.0, 2.0) == 8.0);
	assert(EnumerationDrain::FrameBudgetMs(true, 60, 50.0, 3.0) == 8.0);
	assert(EnumerationDrain::FrameBudgetMs(false, 60, 50.0, 3.0) == 2.0);
	assert(EnumerationDrain::FrameBudgetMs(true, 60, 1000.0 / 60, 0.0) == 2.0);

	{
		EnumerationPreparation::Prepared prepared;
		prepared.valid = true;
		int toc = 0;
		prepared.nativeToc = &toc;
		prepared.package.hashes.push_back({ "appid.appid", {} });
		unsigned char archive[0xB0]{};
		*reinterpret_cast<const void**>(archive + 0x58) = &toc;
		g_internFallback = reinterpret_cast<uintptr_t>(&AbiIntern);
		EnumerationPreparation::Begin(&prepared);
		auto* node = static_cast<BorrowedName*>(AbiBorrow(archive, 11, "appid.appid"));
		assert(node && std::string(node->text) == "appid.appid" && IsBorrowedName(node));
		archive[0x61] = 1;
		assert(AbiBorrow(archive, 11, "appid.appid") == reinterpret_cast<void*>(0x12345678));
		archive[0x61] = 0;
		assert(!BorrowName(4, "miss", archive));
		*reinterpret_cast<const void**>(archive + 0x58) = nullptr;
		assert(!BorrowName(11, "appid.appid", archive));
		EnumerationPreparation::End();
		assert(!BorrowName(11, "appid.appid", archive) && !IsBorrowedName(node));
	}

	{
		// Native vectors store pointers; inline strings store pointer ends.
		alignas(uintptr_t) unsigned char records[3][0x3C]{};
		const char* names[] = { "alpha", "beta", "gamma" };
		uintptr_t slots[3]{};
		for (size_t i = 0; i < 3; ++i) {
			char* text = reinterpret_cast<char*>(records[i] + 0x20);
			strcpy_s(text, 16, names[i]);
			*reinterpret_cast<char**>(records[i] + 0x30) = text + strlen(text);
			*reinterpret_cast<void**>(records[i] + 0x34) = records[i] + 0x30;
			slots[i] = reinterpret_cast<uintptr_t>(records[i]);
		}
		uintptr_t vector[] = { reinterpret_cast<uintptr_t>(slots),
			reinterpret_cast<uintptr_t>(slots + 2), reinterpret_cast<uintptr_t>(slots + 3) };
		uintptr_t object = reinterpret_cast<uintptr_t>(vector);
		g_packages = reinterpret_cast<uintptr_t>(&object);
		g_indexValid = false;
		assert(IndexedDuplicate("alpha") == 1 && IndexedDuplicate("beta") == 1);
		assert(IndexedDuplicate("Alpha") == 2 && IndexedDuplicate("alph") == 2);
		vector[1] += 4;
		IndexMutation(slots[2], true);
		assert(g_indexValid && IndexedDuplicate("gamma") == 1);
		const auto removed = slots[1];
		slots[1] = slots[2];
		vector[1] -= 4;
		IndexMutation(removed, false);
		assert(g_indexValid && IndexedDuplicate("beta") == 2);
		assert(IndexedDuplicate("alpha") == 1 && IndexedDuplicate("gamma") == 1);
		InvalidateIndex();
		assert(IndexedDuplicate("gamma") == 1);
		slots[1] = 1;
		InvalidateIndex();
		// An unreadable package must remain unknown rather than absent.
		assert(IndexedDuplicate("gamma") == 0);
		slots[1] = reinterpret_cast<uintptr_t>(records[2]);
		assert(IndexedDuplicate("gamma") == 1);
		g_indexThread.store(GetCurrentThreadId() + 1);
		assert(IndexedDuplicate("alpha") == 0 && g_indexDisabled.load());
		g_indexThread.store(0);
		g_indexDisabled.store(false);
		g_indexValid = false;
	}

	CheckNativeHandoff();
	wchar_t folder[MAX_PATH]{};
	wchar_t path[MAX_PATH]{};
	assert(GetTempPathW(MAX_PATH, folder));
	assert(GetTempFileNameW(folder, L"enu", 0, path));
	const auto fixture = Unhex(kArchive);
	Write(path, fixture);
	{
		EnumerationPackage::Package package;
		assert(EnumerationPackage::Prepare(path, FixtureInflate, package));
		assert(package.toc.size() == 126);
		uint32_t header = 0;
		std::memcpy(&header, package.toc.data(), sizeof(header));
		assert(header == 0x50534152);
		assert(package.appidIndex == 1);
		assert(std::string(package.appid.begin(), package.appid.end()) == "221680");
		assert(package.hashes.size() == 2);
		const auto md5 = Unhex("8f63cc5819e2d776da20bba466188858");
		assert(std::memcmp(package.hashes[0].digest.data(), md5.data(), 16) == 0);
		// Retain the file generation until native installation finishes.
		const HANDLE replacement = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ,
			nullptr, OPEN_EXISTING, 0, nullptr);
		assert(replacement == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION);
	}
	{
		EnumerationPackage::Package built;
		assert(EnumerationPackage::Prepare(path, FixtureInflate, built));
		auto record = EnumerationPackage::Capture(built, "p|1|2", 7);
		record.mapperRefs = { 4, 5, 6 };
		built.file.reset();
		const auto bytes = EnumerationCache::Serialize("id", { &record });
		std::vector<EnumerationPackage::Record> records;
		std::string reason;
		assert(EnumerationCache::Deserialize(bytes, "id", records, reason) && records.size() == 1);
		assert(records[0].key == "p|1|2" && records[0].costUs == 7 && records[0].indexHash == built.indexHash);
		assert(EnumerationCache::PathOf(records[0].key) == "p");
		assert((records[0].mapperRefs == std::vector<unsigned char>{ 4, 5, 6 }));
		assert(!EnumerationCache::Deserialize(bytes, "other", records, reason) && records.empty());
		for (const size_t offset : { size_t(0), size_t(9), bytes.size() / 2, bytes.size() - 1 }) {
			auto damaged = bytes;
			damaged[offset] ^= 0x01;
			assert(!EnumerationCache::Deserialize(damaged, "id", records, reason));
		}
		auto shortBytes = bytes;
		shortBytes.resize(bytes.size() - 3);
		assert(!EnumerationCache::Deserialize(shortBytes, "id", records, reason));
		assert(EnumerationCache::Deserialize(bytes, "id", records, reason));

		const auto restore = [&](const EnumerationPackage::Record& candidate, EnumerationPackage::Package& out) {
			unsigned char header[32]{};
			assert(EnumerationPackage::Open(path, out) && EnumerationPackage::ReadIndex(out, header));
			return EnumerationPackage::Restore(out, header, candidate);
		};
		EnumerationPackage::Package restored;
		assert(restore(records[0], restored));
		assert(restored.toc == built.toc && restored.names == built.names && restored.appid == built.appid);
		assert(restored.appidIndex == built.appidIndex && restored.hashes.size() == built.hashes.size());
		for (size_t i = 0; i < built.hashes.size(); ++i)
			assert(restored.hashes[i].name == built.hashes[i].name && restored.hashes[i].digest == built.hashes[i].digest);
		restored.file.reset();

		auto stale = records[0];
		stale.indexHash ^= 1;
		auto names = records[0];
		names.names.push_back('\n');
		names.names.push_back('x');
		auto appid = records[0];
		appid.appidIndex = 2;
		for (const auto* bad : { &stale, &names, &appid }) {
			EnumerationPackage::Package package;
			assert(!restore(*bad, package));
		}
	}
	for (const size_t offset : { size_t(0), size_t(7), size_t(16), size_t(31), size_t(62) }) {
		auto bad = fixture;
		bad[offset] ^= 0x40;
		Write(path, bad);
		EnumerationPackage::Package package;
		assert(!EnumerationPackage::Prepare(path, FixtureInflate, package));
	}
	{
		auto shortFile = fixture;
		shortFile.resize(100);
		Write(path, shortFile);
		EnumerationPackage::Package package;
		assert(!EnumerationPackage::Prepare(path, FixtureInflate, package));
	}
	assert(DeleteFileW(path));
	std::puts("Enumeration checks passed: CFB128, TOC, file list, MD5, appid, bounds, file lifetime, x86 handoff/fallback ABI, stream references and cache records");
}
