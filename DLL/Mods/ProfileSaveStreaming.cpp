#include "../stdafx.h"
#include "ProfileSaveStreaming.hpp"
#include "../MemUtil.hpp"
#include "../SamplingProfiler.hpp"
#include <condition_variable>
#include <deque>
#include <mutex>

namespace ProfileSaveStreaming {
	namespace {
		/// <summary>
		/// The game's std::string (STLport + efd::CustomAllocator). Strings of 15 chars or less live in the 16 byte inline buffer,
		/// and endOfStorage then points at finish (the end of that buffer).
		/// </summary>
		struct EngineString {
			char* data;
			char inlineRest[12];
			char* finish;
			char* endOfStorage;
		};
		static_assert(sizeof(EngineString) == 0x18, "EngineString must match the game's layout");

		/// <summary>
		/// z_stream from the zlib 1.2.7 the game links statically.
		/// </summary>
		struct ZStream {
			const uint8_t* next_in;
			uint32_t avail_in;
			uint32_t total_in;
			uint8_t* next_out;
			uint32_t avail_out;
			uint32_t total_out;
			const char* msg;
			void* state;
			void* zalloc;
			void* zfree;
			void* opaque;
			int data_type;
			uint32_t adler;
			uint32_t reserved;
		};
		static_assert(sizeof(ZStream) == 0x38, "ZStream must match zlib 1.2.7");

		typedef void(__cdecl* tJsonWriter)(void* ctx, const char* format, ...);
		typedef int(__cdecl* tJsonIndent)(tJsonWriter writer, void* ctx, int depth);
		typedef void(__fastcall* tJsonPrint)(void* value, void* edx, tJsonWriter writer, tJsonIndent indent, void* ctx, int depth);
		typedef void(__fastcall* tJsonRefCount)(void* value, void* edx);
		typedef int(__cdecl* tDeflateInit)(ZStream* stream, int level, const char* version, int streamSize);
		typedef int(__cdecl* tDeflate)(ZStream* stream, int flush);
		typedef int(__cdecl* tDeflateEnd)(ZStream* stream);
		typedef uint32_t(__cdecl* tCompressBound)(uint32_t sourceLen);
		typedef int(__cdecl* tCompress2)(uint8_t* dest, uint32_t* destLen, const uint8_t* source, uint32_t sourceLen, int level);
		typedef void(__fastcall* tStringDestroy)(EngineString* str);

		constexpr int Z_OK = 0, Z_STREAM_END = 1, Z_STREAM_ERROR = -2, Z_BUF_ERROR = -5;
		constexpr int Z_NO_FLUSH = 0, Z_FINISH = 4;
		constexpr int compressionLevel = 6;					// The game uses 9. On 100MB+ of JSON that costs seconds for a ~5% smaller file.
		constexpr uint32_t drainThreshold = 1024 * 1024;	// Hand the JSON to zlib every MB instead of holding all of it.
		constexpr size_t minOutputSpace = 64 * 1024;

		struct Section {
			std::string name;
			void* value;
		};

		std::vector<Section> sections;
		ZStream stream;
		bool streamFailed = false;
		std::vector<uint8_t> compressed;
		uint32_t uncompressedSize = 0;
		bool pendingValid = false;	// compressed holds this save's PRFLDB payload.
		bool pendingArmed = false;	// The PRFLDB writer asked for it; the next compress2 hands it over.
		tCompressBound origCompressBound = nullptr;
		tCompress2 origCompress2 = nullptr;

		char* Data(EngineString* str) {
			return str->endOfStorage == reinterpret_cast<char*>(&str->finish) ? reinterpret_cast<char*>(str) : str->data;
		}

		void** VTable(void* object) {
			return *reinterpret_cast<void***>(object);
		}

		void AddRef(void* value) {
			reinterpret_cast<tJsonRefCount>(VTable(value)[0])(value, nullptr);
		}

		void Release(void* value) {
			reinterpret_cast<tJsonRefCount>(VTable(value)[1])(value, nullptr);
		}

		void Print(void* value, tJsonWriter writer, tJsonIndent indent, void* ctx, int depth) {
			reinterpret_cast<tJsonPrint>(VTable(value)[0x68 / 4])(value, nullptr, writer, indent, ctx, depth);
		}

		/// <summary>
		/// Push bytes through the deflate stream, growing the output buffer as needed.
		/// </summary>
		bool Feed(const uint8_t* bytes, uint32_t length, int flush) {
			const tDeflate deflate = reinterpret_cast<tDeflate>(Offsets::func_zlibDeflate.Get());

			stream.next_in = bytes;
			stream.avail_in = length;

			try {
				while (true) {
					if (compressed.size() - stream.total_out < minOutputSpace)
						compressed.resize(compressed.size() + (std::max)(compressed.size() / 2, (size_t)1024 * 1024));

					stream.next_out = compressed.data() + stream.total_out;
					stream.avail_out = static_cast<uint32_t>(compressed.size() - stream.total_out);

					const int ret = deflate(&stream, flush);
					if (ret == Z_STREAM_ERROR)
						return false;

					if (flush == Z_FINISH) {
						if (ret == Z_STREAM_END)
							return true;
						if (ret != Z_OK && ret != Z_BUF_ERROR)
							return false;
					}
					else if (stream.avail_in == 0 && stream.avail_out != 0)
						return true;
				}
			}
			catch (const std::exception&) {
				return false; // Out of memory. The caller falls back to the game's own save.
			}
		}


		// ---- Writer ----
		// The game's writer formats every token into a temporary std::string, takes a UTF-8 substring of it and appends that, and the
		// numbers go through printf's "%f". Profiling a 235MB save put over half of its time there. This writes straight into a buffer
		// that feeds zlib. Formatting still goes through the game's msvcr100 for anything but whole numbers, so the output is the same.

		typedef int(__cdecl* tVsnprintf)(char* buffer, size_t count, const char* format, va_list args);
		typedef int(__cdecl* tVscprintf)(const char* format, va_list args);

		tVsnprintf gameVsnprintf = nullptr;
		tVscprintf gameVscprintf = nullptr;

		// The main thread prints into one of these while the compressor thread deflates the full ones, so a save takes about as long
		// as the slower of the two instead of both. Only the compressor touches the deflate stream until it's joined.
		constexpr size_t sinkBufferCount = 4;
		std::vector<char> sinkBuffers[sinkBufferCount];
		std::mutex sinkMutex;
		std::condition_variable sinkChanged;
		std::deque<std::pair<size_t, size_t>> filledBuffers; // Index, bytes used
		std::deque<size_t> freeBuffers;
		bool sinkFinished = false;
		size_t sinkIndex = 0;
		size_t sinkUsed = 0;

		void Compressor() {
			while (true) {
				std::pair<size_t, size_t> filled;
				{
					std::unique_lock lock(sinkMutex);
					sinkChanged.wait(lock, [] { return !filledBuffers.empty() || sinkFinished; });
					if (filledBuffers.empty())
						return;
					filled = filledBuffers.front();
					filledBuffers.pop_front();
				}

				if (!streamFailed && !Feed(reinterpret_cast<const uint8_t*>(sinkBuffers[filled.first].data()), static_cast<uint32_t>(filled.second), Z_NO_FLUSH))
					streamFailed = true;
				uncompressedSize += static_cast<uint32_t>(filled.second);

				{
					std::lock_guard lock(sinkMutex);
					freeBuffers.push_back(filled.first);
				}
				sinkChanged.notify_all();
			}
		}

		/// <summary>
		/// Hand the current buffer to the compressor and take a free one, waiting if it is behind.
		/// </summary>
		void FlushSink() {
			if (sinkUsed == 0)
				return;

			std::unique_lock lock(sinkMutex);
			filledBuffers.push_back({ sinkIndex, sinkUsed });
			sinkChanged.notify_all();
			sinkChanged.wait(lock, [] { return !freeBuffers.empty(); });
			sinkIndex = freeBuffers.front();
			freeBuffers.pop_front();
			sinkUsed = 0;
		}

		void Write(const char* bytes, size_t length) {
			while (length != 0) {
				std::vector<char>& buffer = sinkBuffers[sinkIndex];
				const size_t chunk = (std::min)(length, buffer.size() - sinkUsed);
				memcpy(buffer.data() + sinkUsed, bytes, chunk);
				sinkUsed += chunk;
				bytes += chunk;
				length -= chunk;
				if (sinkUsed == buffer.size())
					FlushSink();
			}
		}

		bool StartSink() {
			try {
				for (std::vector<char>& buffer : sinkBuffers)
					buffer.resize(drainThreshold);
			}
			catch (const std::exception&) {
				return false;
			}

			filledBuffers.clear();
			freeBuffers.clear();
			for (size_t i = 1; i < sinkBufferCount; i++)
				freeBuffers.push_back(i);
			sinkIndex = 0;
			sinkUsed = 0;
			sinkFinished = false;
			return true;
		}

		void FreeSink() {
			for (std::vector<char>& buffer : sinkBuffers)
				std::vector<char>().swap(buffer);
		}

		/// <summary>
		/// "%f" for a whole number is its digits and ".000000". Anything else goes to printf.
		/// </summary>
		bool WriteWholeNumber(double value) {
			if (!(value > -1e15 && value < 1e15) || value != static_cast<double>(static_cast<int64_t>(value)) || (value == 0.0 && signbit(value)))
				return false;

			char digits[32];
			char* end = digits + sizeof(digits);
			char* start = end;
			const char fraction[] = ".000000";
			start -= sizeof(fraction) - 1;
			memcpy(start, fraction, sizeof(fraction) - 1);

			int64_t whole = static_cast<int64_t>(value);
			const bool negative = whole < 0;
			uint64_t magnitude = negative ? static_cast<uint64_t>(-whole) : static_cast<uint64_t>(whole);
			do {
				*--start = static_cast<char>('0' + magnitude % 10);
				magnitude /= 10;
			} while (magnitude != 0);
			if (negative)
				*--start = '-';

			Write(start, end - start);
			return true;
		}

		void __cdecl FastWriter(void*, const char* format, ...) {
			va_list args;
			va_start(args, format);

			if (format[0] == '%' && format[1] == 'f' && format[2] == '\0') {
				va_list copy = args;
				if (WriteWholeNumber(va_arg(copy, double))) {
					va_end(args);
					return;
				}
			}
			else if (!strchr(format, '%')) {
				Write(format, strlen(format));
				va_end(args);
				return;
			}
			else if (strcmp(format, "\"%s\" : ") == 0) {
				const char* key = va_arg(args, const char*);
				Write("\"", 1);
				Write(key, strlen(key));
				Write("\" : ", 4);
				va_end(args);
				return;
			}

			char buffer[512];
			const int length = gameVsnprintf(buffer, sizeof(buffer), format, args);
			if (length >= 0 && length < static_cast<int>(sizeof(buffer)))
				Write(buffer, length);
			else {
				std::vector<char> big(static_cast<size_t>(gameVscprintf(format, args)) + 1);
				const int bigLength = gameVsnprintf(big.data(), big.size(), format, args);
				if (bigLength > 0)
					Write(big.data(), bigLength);
			}
			va_end(args);
		}

		int __cdecl NoIndent(tJsonWriter, void*, int) {
			return 0;
		}

		/// <summary>
		/// Same output as the game's JSON::Object::Print on the root it builds out of the cloned sections.
		/// </summary>
		void PrintSections(tJsonWriter writer, tJsonIndent indent, void* out) {
			writer(out, "{\n");
			for (size_t i = 0; i < sections.size(); i++) {
				indent(writer, out, 1);
				writer(out, "\"%s\" : ", sections[i].name.c_str());
				Print(sections[i].value, writer, indent, out, 1);
				writer(out, i + 1 < sections.size() ? ",\n" : "\n");
			}
			indent(writer, out, 0);
			writer(out, "}");
		}

		/// <summary>
		/// Replaces "section->Clone(); root->SetKeyValue(name, clone);" for each persistent section.
		/// We only keep a reference; the section itself can't go in a second parent (SetKeyValue detaches it from the live database).
		/// </summary>
		void __stdcall RecordSection(EngineString* name, void* value) {
			AddRef(value);

			const char* nameStr = Data(name);
			for (Section& section : sections) {
				if (section.name == nameStr) { // SetKeyValue replaces duplicates
					Release(section.value);
					section.value = value;
					return;
				}
			}

			sections.push_back({ nameStr, value });
		}

		/// <summary>
		/// Replaces "root->Print(writer, stub, out, 0);". Prints the sections straight into a deflate stream.
		/// </summary>
		void __stdcall PrintProfile(void* root, EngineString* out) {
			const tJsonWriter writer = reinterpret_cast<tJsonWriter>(Offsets::func_profileJsonWriter.Get());

			pendingValid = pendingArmed = false;
			std::vector<uint8_t>().swap(compressed);

			if (sections.empty()) { // Nothing was recorded (hook not placed?), so the game filled the root itself.
				Print(root, writer, NoIndent, out, 0);
				return;
			}

			const tDeflateInit deflateInit = reinterpret_cast<tDeflateInit>(Offsets::func_zlibDeflateInit.Get());
			const tDeflateEnd deflateEnd = reinterpret_cast<tDeflateEnd>(Offsets::func_zlibDeflateEnd.Get());

			bool streamed = false;
			stream = {};
			uncompressedSize = 0;
			streamFailed = !gameVsnprintf || !gameVscprintf || !StartSink();

			if (!streamFailed && deflateInit(&stream, compressionLevel, "1.2.7", sizeof(ZStream)) == Z_OK) {
				std::thread compressor;
				try {
					compressor = std::thread(Compressor);
				}
				catch (const std::exception&) {
					streamFailed = true;
				}

				if (compressor.joinable()) {
					PrintSections(FastWriter, NoIndent, nullptr);
					FlushSink();
					{
						std::lock_guard lock(sinkMutex);
						sinkFinished = true;
					}
					sinkChanged.notify_all();
					compressor.join();

					const uint8_t terminator = '\0'; // The game compresses the string's null terminator too, and the loader expects it.
					if (!streamFailed && Feed(&terminator, 1, Z_FINISH)) {
						uncompressedSize += 1;
						compressed.resize(stream.total_out);
						streamed = true;
					}
				}

				deflateEnd(&stream);
			}
			FreeSink();

			if (streamed) {
				pendingValid = true;
				LOG_INFO("(PROFILE SAVE) Streamed profile save: " << uncompressedSize / 1024 << " KB of JSON into " << compressed.size() / 1024 << " KB" << std::endl);
			}
			else {
				// Start over the way the game does it: the whole profile in one string.
				std::vector<uint8_t>().swap(compressed);
				char* data = Data(out);
				out->finish = data;
				*data = '\0';
				PrintSections(writer, NoIndent, out);
				LOG_WARNING("(PROFILE SAVE) Couldn't stream the profile save, used the game's save instead" << std::endl);
			}

			for (Section& section : sections)
				Release(section.value);
			sections.clear();
		}

		/// <summary>
		/// The PRFLDB writer asks for the worst case compressed size of the JSON before compressing it.
		/// We already have the compressed data, and GRProfileSave::CommitFile passes an empty buffer because our JSON string is empty.
		/// </summary>
		uint32_t __cdecl CompressBound(uint32_t sourceLen) {
			if (sourceLen == 0 && pendingValid) {
				pendingArmed = true;
				return static_cast<uint32_t>(compressed.size());
			}
			return origCompressBound(sourceLen);
		}

		/// <summary>
		/// Hand over the already compressed profile. The EVAS header was filled from the (empty) source, so fix its uncompressed size, the dword before dest.
		/// </summary>
		int __cdecl Compress2(uint8_t* dest, uint32_t* destLen, const uint8_t* source, uint32_t sourceLen, int level) {
			if (!pendingArmed || sourceLen != 0)
				return origCompress2(dest, destLen, source, sourceLen, level);

			memcpy(dest, compressed.data(), compressed.size());
			*destLen = static_cast<uint32_t>(compressed.size());
			reinterpret_cast<uint32_t*>(dest)[-1] = uncompressedSize;

			pendingValid = pendingArmed = false;
			std::vector<uint8_t>().swap(compressed);
			return Z_OK;
		}

		/// <summary>
		/// After the profile is parsed, the game empties the JSON text it loaded, but keeps the memory (the whole size of the profile) until it quits.
		/// </summary>
		void __stdcall ReleaseLoadedDocument(EngineString* document) {
			if (document->endOfStorage != reinterpret_cast<char*>(&document->finish))
				reinterpret_cast<tStringDestroy>(Offsets::func_engineStringDestroy.Get())(document);

			document->data = nullptr; // First byte of the inline buffer = empty string.
			document->finish = reinterpret_cast<char*>(document);
			document->endOfStorage = reinterpret_cast<char*>(&document->finish);
		}

		void __declspec(naked) cloneSectionHook() {
			__asm {
				pushad
				push ebx						// Section value (the game holds a reference until the jump back)
				lea eax, [ebp - 0x20]			// Section name
				push eax
				call RecordSection

				lea ecx, Offsets::ptr_profileSaveCloneSectionJmpBck
				call VersioningStruct<uintptr_t>::GetValue
				mov Offsets::runtimeVersionStructValue, eax

				popad
				jmp Offsets::runtimeVersionStructValue // Releases the section, skipping the clone and insert.
			}
		}

		void __declspec(naked) printRootHook() {
			__asm {
				pushad
				push dword ptr [ebp - 0x3C]		// Output string
				push ebx						// Root
				call PrintProfile

				lea ecx, Offsets::ptr_profileSavePrintRootJmpBck
				call VersioningStruct<uintptr_t>::GetValue
				mov Offsets::runtimeVersionStructValue, eax

				popad
				jmp Offsets::runtimeVersionStructValue
			}
		}

		void __declspec(naked) loadClearDocumentHook() {
			__asm {
				mov byte ptr [ebp + 0xF], al	// The code we are overwriting to place this hook (DeserializeProfileFromString's result)

				pushad
				push esi						// GRProfileSave's JSON text
				call ReleaseLoadedDocument

				lea ecx, Offsets::ptr_profileLoadClearDocumentJmpBck
				call VersioningStruct<uintptr_t>::GetValue
				mov Offsets::runtimeVersionStructValue, eax

				popad
				jmp Offsets::runtimeVersionStructValue
			}
		}

		bool RedirectCall(VersioningStruct<uintptr_t>& callSite, void* newTarget, uintptr_t& originalTarget) {
			const uintptr_t site = callSite.Get();
			originalTarget = site + 5 + *reinterpret_cast<int32_t*>(site + 1);
			const int32_t relative = static_cast<int32_t>(reinterpret_cast<uintptr_t>(newTarget) - (site + 5));
			return MemUtil::PatchAdr(site + 1, &relative, sizeof(relative), false);
		}

		// ---- Loading ----

		typedef void(__fastcall* tProfileCall)(void* adapter, void* edx, int index);
		typedef bool(__fastcall* tProfileIsComplete)(void* adapter, void* edx, int index);
		typedef int(__fastcall* tProfileResult)(void* adapter, void* edx, int index);
		typedef void(__cdecl* tJsonParse)(void** out, int flags, const char** cursor);
		typedef void(__fastcall* tSaveDatabase)(void* profileSave, void* edx);

		// RSConnectConfigAdapter slots RSProfileService_Win32::OnTick uses to load a profile.
		constexpr int slotLoadStart = 0x7C / 4;
		constexpr int slotLoadIsComplete = 0x80 / 4;
		constexpr int slotLoadFinish = 0x84 / 4;
		constexpr int slotLoadResult = 0x88 / 4;

		enum class LoadState {
			Idle,
			Reading,	// LoadProfileAsyncStart ran; waiting for its file operation.
			Parsing,	// The JSON is being parsed on parseThread.
		};

		LoadState loadState = LoadState::Idle;
		HANDLE parseThread = nullptr;
		const char* parseText = nullptr;
		void* parsedRoot = nullptr;
		ULONGLONG loadStartTime = 0, parseStartTime = 0, parseEndTime = 0;
		tJsonParse origParse = nullptr;
		tSaveDatabase origSaveDatabase = nullptr;

		typedef void* (*tMemManagerGet)();
		typedef void(__fastcall* tAllocatorCall)(void* allocator, void* edx);
		constexpr int slotAllocatorPerThreadInit = 9;
		constexpr int slotAllocatorPerThreadShutdown = 10;

		/// <summary>
		/// Game threads give efd's small object allocator a per thread cache; without one every allocation takes the pool's lock.
		/// </summary>
		void* AllocatorPerThread(int slot) {
			void* memManager = reinterpret_cast<tMemManagerGet>(Offsets::func_memManagerGet.Get())();
			void* allocator = memManager ? *reinterpret_cast<void**>(memManager) : nullptr;
			if (allocator)
				reinterpret_cast<tAllocatorCall>(VTable(allocator)[slot])(allocator, nullptr);
			return allocator;
		}

		DWORD WINAPI ParseProfile(LPVOID) {
			AllocatorPerThread(slotAllocatorPerThreadInit);

			const char* cursor = parseText;
			void* root = nullptr;
			origParse(&root, 0, &cursor);
			parsedRoot = root; // The main thread reads it after the thread handle is signaled.

			AllocatorPerThread(slotAllocatorPerThreadShutdown); // Hands the cached blocks back to the pools.
			return 0;
		}

		/// <summary>
		/// Start parsing the profile LoadProfileAsyncStart read, if LoadProfileAsyncFinish is going to parse it.
		/// </summary>
		bool StartParse(int index) {
			uint8_t* grService = *reinterpret_cast<uint8_t**>(Offsets::ptr_grService.Get());
			uint8_t* saveManager = *reinterpret_cast<uint8_t**>(grService + 0x30);
			uint8_t* profileSave = *reinterpret_cast<uint8_t**>(saveManager + index * 4);

			const int loadResult = *reinterpret_cast<int*>(profileSave + 0x8);
			if (loadResult != 0 && loadResult != 5) // LoadProfileAsyncFinish only deserializes on these.
				return false;

			EngineString* document = reinterpret_cast<EngineString*>(profileSave + 0x18);
			parseText = Data(document);
			if (document->finish == parseText)
				return false;

			parsedRoot = nullptr;
			parseStartTime = GetTickCount64();
			parseThread = CreateThread(nullptr, 0, ParseProfile, nullptr, 0, nullptr);
			if (!parseThread)
				return false;

			SamplingProfiler::Start(GetThreadId(parseThread), "profile_parse");
			return true;
		}

		/// <summary>
		/// Replaces "Start(); while (!IsComplete()) Sleep(100); Finish();" in RSProfileService_Win32::OnTick with one step per tick.
		/// The sign-in flow already waits on loadRequested (this + 0x68) every tick, so the game keeps running while the profile loads.
		/// </summary>
		void __stdcall LoadTick(uint8_t* profileService) {
			void* adapter = *reinterpret_cast<void**>(Offsets::ptr_rsConnectConfigAdapter.Get());
			void** vtable = VTable(adapter);
			const int index = *reinterpret_cast<int*>(profileService + 0x5C);

			if (loadState == LoadState::Idle) {
				loadStartTime = GetTickCount64();
				reinterpret_cast<tProfileCall>(vtable[slotLoadStart])(adapter, nullptr, index);
				loadState = LoadState::Reading;
			}

			if (loadState == LoadState::Reading) {
				if (!reinterpret_cast<tProfileIsComplete>(vtable[slotLoadIsComplete])(adapter, nullptr, index))
					return;

				if (StartParse(index)) {
					loadState = LoadState::Parsing;
					return;
				}
			}

			if (loadState == LoadState::Parsing) {
				if (WaitForSingleObject(parseThread, 0) != WAIT_OBJECT_0)
					return;
				SamplingProfiler::Stop();
				CloseHandle(parseThread);
				parseThread = nullptr;
			}
			else
				parseStartTime = GetTickCount64();
			parseEndTime = GetTickCount64();

			reinterpret_cast<tProfileCall>(vtable[slotLoadFinish])(adapter, nullptr, index);

			if (parsedRoot) { // Finish didn't take it (it only deserializes a good read).
				Release(parsedRoot);
				parsedRoot = nullptr;
			}
			parseText = nullptr;

			*reinterpret_cast<int*>(profileService + 0x6C) = reinterpret_cast<tProfileResult>(vtable[slotLoadResult])(adapter, nullptr, index);
			profileService[0x68] = 0;
			loadState = LoadState::Idle;

			const ULONGLONG now = GetTickCount64();
			LOG_INFO("(PROFILE SAVE) Loaded profile in " << (now - loadStartTime) / 1000.0 << "s (read " << (parseStartTime - loadStartTime) / 1000.0
				<< "s, parse " << (parseEndTime - parseStartTime) / 1000.0 << "s, finish " << (now - parseEndTime) / 1000.0 << "s)" << std::endl);
		}

		/// <summary>
		/// DeserializeProfileFromString's JSON::Parse. Hand over what ParseProfile already parsed.
		/// </summary>
		void __cdecl Parse(void** out, int flags, const char** cursor) {
			if (parsedRoot && *cursor == parseText) {
				*out = parsedRoot;
				parsedRoot = nullptr;
				return;
			}
			origParse(out, flags, cursor);
		}

		/// <summary>
		/// LoadProfileAsyncStart empties the profile in memory before reading the file, and the game keeps running while it's parsed.
		/// Anything that saves in the meantime would write that empty profile over the real one.
		/// </summary>
		void __fastcall SaveDatabase(void* profileSave, void*) {
			if (loadState != LoadState::Idle) {
				LOG_WARNING("(PROFILE SAVE) Skipped a profile save while the profile is loading" << std::endl);
				return;
			}
			const ULONGLONG start = GetTickCount64();
			SamplingProfiler::Start(GetCurrentThreadId(), "profile_save");
			origSaveDatabase(profileSave, nullptr);
			SamplingProfiler::Stop();
			LOG_INFO("(PROFILE SAVE) Saved profile in " << (GetTickCount64() - start) / 1000.0 << "s" << std::endl);
		}

		void __declspec(naked) loadTickHook() {
			__asm {
				pushad
				push ebx						// RSProfileService_Win32
				call LoadTick

				lea ecx, Offsets::ptr_profileLoadTickJmpBck
				call VersioningStruct<uintptr_t>::GetValue
				mov Offsets::runtimeVersionStructValue, eax

				popad
				jmp Offsets::runtimeVersionStructValue // End of OnTick
			}
		}

		// ---- Number interning ----

		/// <summary>
		/// efd::DataStore interns every JSON number in a hash set keyed by value. The game's hash truncates the double to an integer first,
		/// so every value in [n, n+1) shares a bucket, and that bucket is a sorted list walked one entry at a time.
		/// A big profile has ~39,000 distinct numbers in [0, 1) (accuracies and such), which makes parsing it billions of comparisons.
		/// Equal doubles still hash equal: -0.0 is folded into 0.0 because they compare equal.
		/// </summary>
		uint32_t __cdecl HashNumber(const double* value) {
			double number = *value;
			if (number == 0.0)
				number = 0.0;

			uint64_t bits;
			memcpy(&bits, &number, sizeof(bits));
			bits ^= bits >> 33; // MurmurHash3 finalizer
			bits *= 0xFF51AFD7ED558CCDull;
			bits ^= bits >> 33;
			bits *= 0xC4CEB9FE1A85EC53ull;
			bits ^= bits >> 33;
			return static_cast<uint32_t>(bits);
		}

		void __declspec(naked) numberHashHook() {
			__asm {
				push ecx
				push edx
				push eax						// const double*
				call HashNumber
				add esp, 4
				pop edx
				pop ecx
				ret								// Hash in EAX, same as the game's
			}
		}

		void RehashNumberTable(void* table) {
			const uintptr_t rehash = Offsets::func_jsonNumberTableRehash.Get();
			__asm {
				push edi
				mov edi, table
				call rehash
				pop edi
			}
		}

		void InitializeNumberHash() {
			CRITICAL_SECTION* lock = *reinterpret_cast<CRITICAL_SECTION**>(Offsets::ptr_jsonNumberTableLock.Get());
			uint8_t* table = *reinterpret_cast<uint8_t**>(Offsets::ptr_jsonNumberTable.Get());

			// Numbers already in the table sit in the buckets of the old hash. Swap the hash and rebuild the table without letting anyone in between.
			if (lock)
				EnterCriticalSection(lock);

			const bool hooked = MemUtil::PlaceHook(Offsets::func_jsonNumberHash, numberHashHook, 6);
			if (hooked) {
				FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::func_jsonNumberHash.Get(), 6);

				const uint32_t size = table ? *reinterpret_cast<uint32_t*>(table + 0x8) : 0;
				const uint32_t buckets = table ? *reinterpret_cast<uint32_t*>(table + 0x24) : 0;
				if (size != 0 && buckets != 0) {
					// The rehash grows the buckets 8x and reinserts every number with the current hash, once the load factor is over the max.
					// Force exactly one: half the current load triggers it, and the reinserts (which check again) see 1/8 of it.
					float& maxLoadFactor = *reinterpret_cast<float*>(table + 0x28);
					const float savedMaxLoadFactor = maxLoadFactor;
					maxLoadFactor = 0.5f * static_cast<float>(size) / static_cast<float>(buckets);
					RehashNumberTable(table);
					maxLoadFactor = savedMaxLoadFactor;
				}
			}

			if (lock)
				LeaveCriticalSection(lock);

			if (hooked)
				LOG_INFO("(PROFILE SAVE) Replaced the JSON number hash" << std::endl);
			else
				LOG_ERROR("(PROFILE SAVE) Failed to replace the JSON number hash" << std::endl);
		}

		// ---- Playnext stats trim ----

		/// <summary>
		/// songs::TrimPlaynextProfileStats keeps at most 100 PlaynextStats across the profile's songs, but it removes one per pass,
		/// and each pass walks every song (building the "PlaynextStats" key string, which takes the intern lock, for each one).
		/// A profile with ~25,000 of them makes that ~25,000 passes over ~43,000 songs whenever a song gets new stats (song load
		/// and end), which locks the game up. This reads each song's timestamps once and runs the same selection over plain
		/// arrays, including its comparison (a song's own TimeStamp against the current oldest PlaynextStats' TimeStamp), so on a
		/// normal profile it removes exactly what the game would have, through the game's own RemoveChild.
		/// Profiles far over the cap are left untouched instead of being trimmed down to it.
		/// </summary>
		typedef void* (__fastcall* tJsonGetByName)(void* object, void* edx, const char* name);
		typedef void* (__fastcall* tJsonCast)(void* value, void* edx);
		typedef uint32_t(__fastcall* tJsonCount)(void* object, void* edx);
		typedef void(__fastcall* tJsonGetAll)(void* object, void* edx, void** elements);
		typedef void(__fastcall* tJsonRemoveChild)(void* object, void* edx, void* child);
		typedef void(__fastcall* tAllocatorDeallocate)(void* allocator, void* edx, void* pointer, int hint, size_t size);

		constexpr uint32_t maxPlaynextStats = 100;

		struct PlaynextEntry {
			void* stats;			// PlaynextStats object (we hold a reference)
			double songTime;		// The song entry's TimeStamp
			double statsTime;		// The PlaynextStats' TimeStamp
			bool hasSongTime;
			bool hasStatsTime;
			bool removed;
		};

		bool ReadTimeStamp(void* object, double& time) {
			void* value = reinterpret_cast<tJsonGetByName>(VTable(object)[0x14 / 4])(object, nullptr, "TimeStamp");
			if (!value)
				return false;
			uint8_t* number = static_cast<uint8_t*>(reinterpret_cast<tJsonCast>(VTable(value)[0x50 / 4])(value, nullptr));
			if (!number)
				return false;
			time = *reinterpret_cast<double*>(number + 8);
			return true;
		}

		/// <summary>
		/// The game's comparator: is the candidate song older than the current oldest PlaynextStats?
		/// </summary>
		bool IsOlder(const PlaynextEntry& candidate, const PlaynextEntry& oldest) {
			if (!oldest.hasStatsTime)
				return false;
			if (!candidate.hasSongTime)
				return true;
			return candidate.songTime < oldest.statsTime;
		}

		void* FindProfileSongs(void* profile) {
			void* songs = nullptr;
			void** songsOut = &songs;
			const uintptr_t find = Offsets::func_findProfileSongs.Get();
			__asm {
				push songsOut
				mov eax, profile
				call find
				add esp, 4
			}
			return songs;
		}

		void __cdecl TrimPlaynextProfileStats(void* profile) {
			void* songs = FindProfileSongs(profile);
			if (!songs)
				return;

			if (reinterpret_cast<tJsonCount>(VTable(songs)[0x78 / 4])(songs, nullptr) < maxPlaynextStats) {
				Release(songs);
				return;
			}

			void* elements[3] = {}; // begin, end, capacity
			reinterpret_cast<tJsonGetAll>(VTable(songs)[0x74 / 4])(songs, nullptr, elements);

			std::vector<PlaynextEntry> entries;
			for (void** it = static_cast<void**>(elements[0]); it != static_cast<void**>(elements[1]); it++) {
				void* song = *it;
				if (!song)
					continue;
				void* value = reinterpret_cast<tJsonGetByName>(VTable(song)[0x18 / 4])(song, nullptr, "PlaynextStats");
				if (!value || !reinterpret_cast<tJsonCast>(VTable(value)[0x5C / 4])(value, nullptr))
					continue;
				void* stats = reinterpret_cast<tJsonCast>(VTable(value)[0x5C / 4])(value, nullptr);
				if (!stats)
					continue;

				AddRef(stats);
				PlaynextEntry entry{ stats, 0.0, 0.0, false, false, false };
				entry.hasSongTime = ReadTimeStamp(song, entry.songTime);
				entry.hasStatsTime = ReadTimeStamp(stats, entry.statsTime);
				entries.push_back(entry);
			}

			// Normal play adds one PlaynextStats at a time, so the game only ever trims one. A profile far over the cap didn't
			// come from normal play; trimming it would delete thousands of stats, so leave it exactly as it is.
			std::vector<size_t> removals;
			size_t remaining = entries.size();
			if (remaining > maxPlaynextStats + 1) {
				static bool logged = false;
				if (!logged) {
					LOG_WARNING("(PROFILE SAVE) Profile has " << remaining << " PlaynextStats (the game keeps " << maxPlaynextStats << "), leaving them alone" << std::endl);
					logged = true;
				}
				remaining = 0; // Skip the trim below.
			}

			// Same selection as the game: each pass starts from the first remaining entry and takes any later one that IsOlder.
			while (remaining > maxPlaynextStats) {
				size_t oldest = SIZE_MAX;
				for (size_t i = 0; i < entries.size(); i++) {
					if (entries[i].removed)
						continue;
					if (oldest == SIZE_MAX || IsOlder(entries[i], entries[oldest]))
						oldest = i;
				}
				entries[oldest].removed = true;
				removals.push_back(oldest);
				remaining--;
			}

			for (size_t index : removals) {
				void* stats = entries[index].stats;
				void* parent = reinterpret_cast<tJsonCast>(VTable(stats)[0x0C / 4])(stats, nullptr);
				void* parentObject = parent ? reinterpret_cast<tJsonCast>(VTable(parent)[0x5C / 4])(parent, nullptr) : nullptr;
				if (!parentObject)
					continue;
				AddRef(parentObject);
				reinterpret_cast<tJsonRemoveChild>(VTable(parentObject)[0x70 / 4])(parentObject, nullptr, stats);
				Release(parentObject);
			}

			for (PlaynextEntry& entry : entries)
				Release(entry.stats);

			for (void** it = static_cast<void**>(elements[0]); it != static_cast<void**>(elements[1]); it++)
				if (*it)
					Release(*it);

			if (elements[0]) {
				void* memManager = reinterpret_cast<tMemManagerGet>(Offsets::func_memManagerGet.Get())();
				void* allocator = *reinterpret_cast<void**>(memManager);
				const size_t size = (std::max)(static_cast<size_t>(static_cast<uint8_t*>(elements[2]) - static_cast<uint8_t*>(elements[0])), static_cast<size_t>(1));
				reinterpret_cast<tAllocatorDeallocate>(VTable(allocator)[2])(allocator, nullptr, elements[0], 9, size);
			}

			Release(songs);

			if (!removals.empty())
				LOG_INFO("(PROFILE SAVE) Trimmed " << removals.size() << " PlaynextStats (kept " << remaining << ")" << std::endl);
		}

		void InitializePlaynextTrim() {
			// The replacement has the same cdecl signature, so the hook just jumps to it.
			if (MemUtil::PlaceHook(Offsets::func_trimPlaynextProfileStats, TrimPlaynextProfileStats, 6)) {
				FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::func_trimPlaynextProfileStats.Get(), 6);
				LOG_INFO("(PROFILE SAVE) Replaced the PlaynextStats trim" << std::endl);
			}
			else
				LOG_ERROR("(PROFILE SAVE) Failed to replace the PlaynextStats trim" << std::endl);
		}

		void InitializeSaving() {
			HMODULE msvcr100 = GetModuleHandleA("msvcr100.dll");
			if (msvcr100) {
				gameVsnprintf = reinterpret_cast<tVsnprintf>(GetProcAddress(msvcr100, "_vsnprintf"));
				gameVscprintf = reinterpret_cast<tVscprintf>(GetProcAddress(msvcr100, "_vscprintf"));
			}

			uintptr_t compressBound = 0, compress2 = 0;
			if (!RedirectCall(Offsets::ptr_profileSaveCompressBoundCall, CompressBound, compressBound)
				|| !RedirectCall(Offsets::ptr_profileSaveCompress2Call, Compress2, compress2)) {
				LOG_ERROR("(PROFILE SAVE) Failed to hook the profile writer, not streaming profile saves" << std::endl);
				return;
			}
			origCompressBound = reinterpret_cast<tCompressBound>(compressBound);
			origCompress2 = reinterpret_cast<tCompress2>(compress2);

			// Print first: on its own it falls back to the game's print. The clone hook on its own would save an empty profile.
			if (!MemUtil::PlaceHook(Offsets::ptr_profileSavePrintRoot, printRootHook, 5)
				|| !MemUtil::PlaceHook(Offsets::ptr_profileSaveCloneSection, cloneSectionHook, 5)) {
				LOG_ERROR("(PROFILE SAVE) Failed to hook the profile save, not streaming profile saves" << std::endl);
				return;
			}
			FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_profileSavePrintRoot.Get(), 5);
			FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_profileSaveCloneSection.Get(), 5);

			LOG_INFO("(PROFILE SAVE) Streaming profile saves" << std::endl);
		}

		void InitializeLoading() {
			if (MemUtil::PlaceHook(Offsets::ptr_profileLoadClearDocument, loadClearDocumentHook, 6))
				FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_profileLoadClearDocument.Get(), 6);
			else
				LOG_ERROR("(PROFILE SAVE) Failed to free the loaded profile text" << std::endl);

			// The save guard goes in first: without it, a save during the load would overwrite the profile.
			uintptr_t saveDatabase = 0, parse = 0;
			if (!RedirectCall(Offsets::ptr_profileSaveDatabaseCall, SaveDatabase, saveDatabase)) {
				LOG_ERROR("(PROFILE SAVE) Failed to hook profile saves, loading profiles on the main thread" << std::endl);
				return;
			}
			origSaveDatabase = reinterpret_cast<tSaveDatabase>(saveDatabase);

			if (!RedirectCall(Offsets::ptr_profileParseCall, Parse, parse)) {
				LOG_ERROR("(PROFILE SAVE) Failed to hook the profile parser, loading profiles on the main thread" << std::endl);
				return;
			}
			origParse = reinterpret_cast<tJsonParse>(parse);

			if (!MemUtil::PlaceHook(Offsets::ptr_profileLoadTick, loadTickHook, 7)) {
				LOG_ERROR("(PROFILE SAVE) Failed to hook the profile load, loading profiles on the main thread" << std::endl);
				return;
			}
			FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_profileLoadTick.Get(), 7);

			LOG_INFO("(PROFILE SAVE) Loading profiles in the background" << std::endl);
		}
	}

	/// <summary>
	/// Big profiles (100MB+ of JSON, 10M+ values) crash the game when it saves them, and make every save freeze the game for a while.
	/// The game is 32-bit without large address awareness, so it only has 2GB of address space, and a save used to need:
	///  - a deep copy of every section of the profile (JSON::Object::Clone), as many small allocations as the profile itself,
	///  - the JSON as one string (grown by doubling, so up to 2x its size), a copy of it passed by value, and another copy for the writer,
	///  - a buffer for zlib's worst case output, the size of the JSON again.
	/// When one of those allocations fails the game writes through a null pointer (efd::FixedSizeAllocator::FillCache for the clone).
	/// This prints each section straight into a deflate stream instead, 1MB at a time, and gives the writer the compressed data.
	/// Only the compressed profile (a few MB) is ever held in memory. The file is the same format; only the compression level changes.
	/// It also frees the loaded JSON text once the profile is parsed, which the game otherwise keeps for the whole session.
	///
	/// Loading: RSProfileService_Win32::OnTick reads the profile, waits for it and parses it (JSON::Parse) in one tick, which freezes the game
	/// for minutes on a big profile. This spreads that over ticks and parses on a worker thread, so the game keeps rendering while it loads.
	/// The JSON code is safe to run next to the main thread: interned strings and numbers are locked (JSON::s_UseInternCS is on),
	/// ref counts are interlocked, the small object allocator locks, and nothing else can see the parsed tree until Finish attaches it.
	/// </summary>
	void Initialize() {
		// The read and decompress before the parse still happen on the main thread and can take a few seconds on a big profile.
		// Without this, Windows swaps the window for a "Not Responding" ghost (and offers to close the game) while they run.
		DisableProcessWindowsGhosting();

		InitializeNumberHash();
		InitializePlaynextTrim();
		InitializeSaving();
		InitializeLoading();
	}
}
