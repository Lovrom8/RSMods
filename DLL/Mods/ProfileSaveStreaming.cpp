#include "../stdafx.h"
#include "ProfileSaveStreaming.hpp"
#include "ProfileBackups.hpp"
#include "../MemUtil.hpp"
#include "../SamplingProfiler.hpp"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace ProfileSaveStreaming {
	namespace {
		/// <summary>
		/// The game's string layout. Strings of 15 chars or less live in the 16 byte inline buffer,
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

		constexpr int Z_OK = 0, Z_STREAM_END = 1, Z_STREAM_ERROR = -2, Z_MEM_ERROR = -4, Z_BUF_ERROR = -5;
		constexpr int Z_NO_FLUSH = 0, Z_FINISH = 4;
		constexpr int compressionLevel = 6;					// The game uses 9. On 100MB+ of JSON that costs seconds for a ~5% smaller file.
		constexpr uint32_t drainThreshold = 1024 * 1024;	// Hand the JSON to zlib every MB instead of holding all of it.
		constexpr size_t minOutputSpace = 64 * 1024;

		struct Section {
			std::string name;
			void* value;
		};

		enum class Payload {
			None,		// The game compresses its own JSON.
			Streamed,	// compressed holds this save's PRFLDB data.
			Cancelled,	// The profile couldn't be streamed. Fail the write so the file on disk stays as it was.
		};

		/// <summary>
		/// Everything one save hands from the clone hook to the PRFLDB writer.
		/// The game's profile save runs the clone, print, commit and the writer in one call on one thread, and SaveWrapper wraps that call,
		/// so a save owns this from start to end. Only one save can own it at a time (see SaveWrapper).
		/// </summary>
		struct SaveState {
			std::vector<Section> sections;
			bool recordFailed = false;
			std::vector<uint8_t> compressed;
			uint32_t uncompressedSize = 0;
			Payload payload = Payload::None;
			bool armed = false;	// The PRFLDB writer asked for the payload; the next compress2 on this thread takes it.
		};

		SaveState save;
		std::atomic<DWORD> saveThread = 0; // Thread running the save that owns `save`, or 0.

		bool OnSaveThread() {
			return saveThread.load() == GetCurrentThreadId();
		}

		ZStream stream;
		std::atomic<bool> streamFailed = false;
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
			std::vector<uint8_t>& compressed = save.compressed;

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
				return false; // Out of memory. The save is cancelled.
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
				save.uncompressedSize += static_cast<uint32_t>(filled.second);

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
				try {
					std::vector<char> big(static_cast<size_t>(gameVscprintf(format, args)) + 1);
					const int bigLength = gameVsnprintf(big.data(), big.size(), format, args);
					if (bigLength > 0)
						Write(big.data(), bigLength);
				}
				catch (const std::exception&) {
					streamFailed = true; // We're inside the game's Print, so nothing can be thrown through it.
				}
			}
			va_end(args);
		}

		int __cdecl NoIndent(tJsonWriter, void*, int) {
			return 0;
		}

		/// <summary>
		/// Same output as the game's own JSON print of the root it builds out of the cloned sections.
		/// </summary>
		void PrintSections(tJsonWriter writer, tJsonIndent indent, void* out) {
			const std::vector<Section>& sections = save.sections;
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
		/// Replaces cloning each persistent section into the root that gets printed.
		/// We only keep a reference; the section itself can't go in a second parent (adding it to the root detaches it from the live database).
		/// </summary>
		void __stdcall RecordSection(EngineString* name, void* value) {
			if (!OnSaveThread()) // Every save comes through SaveWrapper. If one ever doesn't, PrintProfile makes sure it writes nothing.
				return;

			const char* nameStr = Data(name);
			for (Section& section : save.sections) {
				if (section.name == nameStr) { // the game replaces duplicates
					AddRef(value);
					Release(section.value);
					section.value = value;
					return;
				}
			}

			try {
				save.sections.push_back({ nameStr, value });
			}
			catch (const std::exception&) {
				save.recordFailed = true; // We're inside the game's save, so nothing can be thrown through it.
				return;
			}
			AddRef(value);
		}

		/// <summary>
		/// Prints the recorded sections straight into a deflate stream, 1MB at a time.
		/// </summary>
		bool StreamSections() {
			const tDeflateInit deflateInit = reinterpret_cast<tDeflateInit>(Offsets::func_zlibDeflateInit.Get());
			const tDeflateEnd deflateEnd = reinterpret_cast<tDeflateEnd>(Offsets::func_zlibDeflateEnd.Get());

			bool streamed = false;
			stream = {};
			save.uncompressedSize = 0;
			streamFailed = !StartSink();

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
						save.uncompressedSize += 1;
						save.compressed.resize(stream.total_out);
						streamed = true;
					}
				}

				deflateEnd(&stream);
			}
			FreeSink();
			return streamed;
		}

		/// <summary>
		/// Replaces "root->Print(writer, stub, out, 0);".
		/// If the profile can't be streamed, the save is cancelled instead of handed to the game's print: that builds the whole
		/// profile in one string, and running out of memory is the usual reason we got here, so it would likely crash.
		/// The file on disk stays as it was, and the next save tries again.
		/// </summary>
		void __stdcall PrintProfile(void* root, EngineString* out) {
			if (!OnSaveThread()) {
				// The clone hook skipped this save's sections, so the root is empty. Leave the string empty; Compress2 fails the write.
				LOG_ERROR("(PROFILE SAVE) A profile save didn't come through SaveWrapper, not writing it" << std::endl);
				return;
			}

			if (save.recordFailed) {
				save.payload = Payload::Cancelled;
				LOG_ERROR("(PROFILE SAVE) Ran out of memory collecting the profile, not saving it this time" << std::endl);
				return;
			}

			if (save.sections.empty()) { // No persistent sections, so the game's root is the whole (tiny) profile.
				Print(root, reinterpret_cast<tJsonWriter>(Offsets::func_profileJsonWriter.Get()), NoIndent, out, 0);
				return;
			}

			if (StreamSections()) {
				save.payload = Payload::Streamed;
				LOG_INFO("(PROFILE SAVE) Streamed profile save: " << save.uncompressedSize / 1024 << " KB of JSON into " << save.compressed.size() / 1024 << " KB" << std::endl);
			}
			else {
				save.payload = Payload::Cancelled;
				std::vector<uint8_t>().swap(save.compressed);
				LOG_ERROR("(PROFILE SAVE) Couldn't stream the profile save (out of memory?), not saving it this time" << std::endl);
			}
		}

		/// <summary>
		/// The PRFLDB writer asks for the worst case compressed size of the JSON before compressing it.
		/// We already have the compressed data, and the game's commit step passes an empty buffer because our JSON string is empty.
		/// </summary>
		uint32_t __cdecl CompressBound(uint32_t sourceLen) {
			if (sourceLen == 0 && OnSaveThread() && save.payload == Payload::Streamed) {
				save.armed = true;
				return static_cast<uint32_t>(save.compressed.size());
			}
			return origCompressBound(sourceLen);
		}

		/// <summary>
		/// Hand over the already compressed profile. The EVAS header was filled from the (empty) source, so fix its uncompressed size, the dword before dest.
		/// Any other empty source is a cancelled save. Failing here takes the writer's own "couldn't write the file" path, so the file isn't touched.
		/// </summary>
		int __cdecl Compress2(uint8_t* dest, uint32_t* destLen, const uint8_t* source, uint32_t sourceLen, int level) {
			if (sourceLen != 0)
				return origCompress2(dest, destLen, source, sourceLen, level);

			if (!save.armed || !OnSaveThread() || save.payload != Payload::Streamed)
				return Z_MEM_ERROR;

			memcpy(dest, save.compressed.data(), save.compressed.size());
			*destLen = static_cast<uint32_t>(save.compressed.size());
			reinterpret_cast<uint32_t*>(dest)[-1] = save.uncompressedSize;

			save.armed = false;
			save.payload = Payload::None;
			std::vector<uint8_t>().swap(save.compressed);
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

				popad
				push offset Offsets::ptr_profileSaveCloneSectionJmpBck // Releases the section, skipping the clone and insert.
				jmp MemUtil::JumpToVersioned
			}
		}

		void __declspec(naked) printRootHook() {
			__asm {
				pushad
				push dword ptr [ebp - 0x3C]		// Output string
				push ebx						// Root
				call PrintProfile

				popad
				push offset Offsets::ptr_profileSavePrintRootJmpBck
				jmp MemUtil::JumpToVersioned
			}
		}

		void __declspec(naked) loadClearDocumentHook() {
			__asm {
				mov byte ptr [ebp + 0xF], al	// The code we are overwriting to place this hook (the profile parse result)

				pushad
				push esi						// The profile's loaded JSON text
				call ReleaseLoadedDocument

				popad
				push offset Offsets::ptr_profileLoadClearDocumentJmpBck
				jmp MemUtil::JumpToVersioned
			}
		}

		uintptr_t CallTarget(VersioningStruct<uintptr_t>& callSite) {
			const uintptr_t site = callSite.Get();
			return site + 5 + *reinterpret_cast<int32_t*>(site + 1);
		}

		/// <summary>
		/// Hooks that only work together. If one can't be placed, Undo puts back the ones that were, so the game never runs with half of them.
		/// </summary>
		class PatchGroup {
			struct Saved {
				uintptr_t address;
				uint8_t bytes[8];
				size_t length;
			};
			std::vector<Saved> saved;

			void Save(uintptr_t address, size_t length) {
				Saved entry{ address, {}, length };
				memcpy(entry.bytes, reinterpret_cast<void*>(address), length);
				saved.push_back(entry);
			}

		public:
			bool Hook(VersioningStruct<uintptr_t>& at, void* hook, int length) {
				Save(at.Get(), length);
				if (!MemUtil::PlaceHook(at, hook, length))
					return false;
				FlushInstructionCache(GetCurrentProcess(), (void*)at.Get(), length);
				return true;
			}

			/// <summary>
			/// Point a CALL rel32 somewhere else. Read the original target with CallTarget first.
			/// </summary>
			bool Redirect(VersioningStruct<uintptr_t>& callSite, void* newTarget) {
				const uintptr_t site = callSite.Get();
				Save(site + 1, sizeof(int32_t));
				const int32_t relative = static_cast<int32_t>(reinterpret_cast<uintptr_t>(newTarget) - (site + 5));
				if (!MemUtil::PatchAdr(site + 1, &relative, sizeof(relative), false))
					return false;
				FlushInstructionCache(GetCurrentProcess(), (void*)site, 5);
				return true;
			}

			void Undo() {
				for (auto it = saved.rbegin(); it != saved.rend(); it++) {
					MemUtil::PatchAdr(reinterpret_cast<LPVOID>(it->address), it->bytes, it->length);
					FlushInstructionCache(GetCurrentProcess(), (void*)it->address, it->length);
				}
				saved.clear();
			}
		};

		// ---- Loading ----

		typedef void(__fastcall* tProfileCall)(void* adapter, void* edx, int index);
		typedef bool(__fastcall* tProfileReady)(void* adapter, void* edx, int index);
		typedef int(__fastcall* tProfileResult)(void* adapter, void* edx, int index);
		typedef void(__cdecl* tJsonParse)(void** out, int flags, const char** cursor);
		typedef void(__fastcall* tSaveCall)(void* profileSave, void* edx);

		// Profile adapter slots the profile tick uses to load a profile.
		constexpr int slotLoadStart = 0x7C / 4;
		constexpr int slotLoadReady = 0x80 / 4;
		constexpr int slotLoadFinish = 0x84 / 4;
		constexpr int slotLoadResult = 0x88 / 4;

		enum class LoadState {
			Idle,
			Reading,	// The profile read started; waiting for its file operation.
			Parsing,	// The JSON is being parsed on parseThread.
		};

		LoadState loadState = LoadState::Idle;
		HANDLE parseThread = nullptr;
		const char* parseText = nullptr;
		void* parsedRoot = nullptr;
		ULONGLONG loadStartTime = 0, parseStartTime = 0, parseEndTime = 0;
		tJsonParse origParse = nullptr;
		tSaveCall origSave = nullptr;

		typedef void* (*tGetAllocatorOwner)();
		typedef void(__fastcall* tAllocatorCall)(void* allocator, void* edx);
		constexpr int slotAllocatorPerThreadInit = 9;
		constexpr int slotAllocatorPerThreadShutdown = 10;

		/// <summary>
		/// Game threads give the game's small object allocator a per thread cache; without one every allocation takes the pool's lock.
		/// </summary>
		void* AllocatorPerThread(int slot) {
			void* allocatorOwner = reinterpret_cast<tGetAllocatorOwner>(Offsets::func_getAllocatorOwner.Get())();
			void* allocator = allocatorOwner ? *reinterpret_cast<void**>(allocatorOwner) : nullptr;
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
		/// Start parsing the profile the game just read, if the game's load step is going to parse it.
		/// </summary>
		bool StartParse(int index) {
			uint8_t* services = *reinterpret_cast<uint8_t**>(Offsets::ptr_gameServices.Get());
			uint8_t* saveManager = *reinterpret_cast<uint8_t**>(services + 0x30);
			uint8_t* profileSave = *reinterpret_cast<uint8_t**>(saveManager + index * 4);

			const int loadResult = *reinterpret_cast<int*>(profileSave + 0x8);
			if (loadResult != 0 && loadResult != 5) // The game only parses the profile on these.
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
		/// Replaces the profile tick's start, wait loop (100ms sleeps) and finish with one step per tick.
		/// The sign-in flow already waits on loadRequested (this + 0x68) every tick, so the game keeps running while the profile loads.
		/// </summary>
		void __stdcall LoadTick(uint8_t* profileService) {
			void* adapter = *reinterpret_cast<void**>(Offsets::ptr_profileAdapter.Get());
			void** vtable = VTable(adapter);
			const int index = *reinterpret_cast<int*>(profileService + 0x5C);

			if (loadState == LoadState::Idle) {
				loadStartTime = GetTickCount64();
				reinterpret_cast<tProfileCall>(vtable[slotLoadStart])(adapter, nullptr, index);
				loadState = LoadState::Reading;
			}

			if (loadState == LoadState::Reading) {
				if (!reinterpret_cast<tProfileReady>(vtable[slotLoadReady])(adapter, nullptr, index))
					return;

				if (StartParse(index)) {
					loadState = LoadState::Parsing;
					return;
				}
			}

			if (loadState == LoadState::Parsing) {
				if (WaitForSingleObject(parseThread, 0) != WAIT_OBJECT_0)
					return;
				SamplingProfiler::Stop("profile_parse");
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
		/// The game's profile parse call. Hand over what ParseProfile already parsed.
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
		/// Starting a profile load empties the profile in memory before reading the file, and the game keeps running while it's parsed.
		/// Anything that saves in the meantime would write that empty profile over the real one.
		///
		/// This call is also the only way into the profile save: everything the save hooks touch is only reached through it,
		/// and it runs the clone, print, commit and the PRFLDB writer before returning.
		/// So the save owns `save` for exactly this call. The game only saves from its main loop, but if a second thread ever
		/// started a save while one is running, it is skipped rather than allowed to mix its sections and data with the first.
		/// </summary>
		void __fastcall SaveWrapper(void* profileSave, void*) {
			if (loadState != LoadState::Idle) {
				LOG_WARNING("(PROFILE SAVE) Skipped a profile save while the profile is loading" << std::endl);
				return;
			}

			DWORD idle = 0;
			if (!saveThread.compare_exchange_strong(idle, GetCurrentThreadId())) {
				LOG_WARNING("(PROFILE SAVE) Skipped a profile save that started while another was running" << std::endl);
				return;
			}
			save = SaveState{};

			const ULONGLONG start = GetTickCount64();
			SamplingProfiler::Start(GetCurrentThreadId(), "profile_save");
			{
				const ProfileBackups::SaveGuard backupGuard;
				origSave(profileSave, nullptr);
			}
			SamplingProfiler::Stop("profile_save");

			// The commit step can return before the writer runs (no Steam account, etc.), so whatever is left is dropped here.
			for (Section& section : save.sections)
				Release(section.value);
			save = SaveState{};
			saveThread = 0;

			LOG_INFO("(PROFILE SAVE) Saved profile in " << (GetTickCount64() - start) / 1000.0 << "s" << std::endl);
		}

		void __declspec(naked) loadTickHook() {
			__asm {
				pushad
				push ebx						// The profile service
				call LoadTick

				popad
				push offset Offsets::ptr_profileLoadTickJmpBck // End of the tick
				jmp MemUtil::JumpToVersioned
			}
		}

		// ---- Number interning ----

		/// <summary>
		/// The game interns every JSON number in a hash set keyed by value. The game's hash truncates the double to an integer first,
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
		/// The game's PlaynextStats trim keeps at most 100 PlaynextStats across the profile's songs, but it removes one per pass,
		/// and each pass walks every song (building the "PlaynextStats" key string, which takes the intern lock, for each one).
		/// A profile with ~25,000 of them makes that ~25,000 passes over ~43,000 songs whenever a song gets new stats (song load
		/// and end), which locks the game up. This reads each song's timestamps once and runs the same selection over plain
		/// arrays, including its comparison (a song's own TimeStamp against the current oldest PlaynextStats' TimeStamp), so on a
		/// normal profile it removes exactly what the game would have, through the game's own remove call.
		/// Profiles far over the cap are left untouched instead of being trimmed down to it.
		/// </summary>
		typedef void* (__fastcall* tJsonFind)(void* object, void* edx, const char* name);
		typedef void* (__fastcall* tJsonCast)(void* value, void* edx);
		typedef uint32_t(__fastcall* tJsonCount)(void* object, void* edx);
		typedef void(__fastcall* tJsonGetAll)(void* object, void* edx, void** elements);
		typedef void(__fastcall* tJsonRemove)(void* object, void* edx, void* child);
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
			void* value = reinterpret_cast<tJsonFind>(VTable(object)[0x14 / 4])(object, nullptr, "TimeStamp");
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

		void* ProfileSongs(void* profile) {
			void* songs = nullptr;
			void** songsOut = &songs;
			const uintptr_t find = Offsets::func_profileSongs.Get();
			__asm {
				push songsOut
				mov eax, profile
				call find
				add esp, 4
			}
			return songs;
		}

		void __cdecl TrimPlaynext(void* profile) {
			void* songs = ProfileSongs(profile);
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
				void* value = reinterpret_cast<tJsonFind>(VTable(song)[0x18 / 4])(song, nullptr, "PlaynextStats");
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
				reinterpret_cast<tJsonRemove>(VTable(parentObject)[0x70 / 4])(parentObject, nullptr, stats);
				Release(parentObject);
			}

			for (PlaynextEntry& entry : entries)
				Release(entry.stats);

			for (void** it = static_cast<void**>(elements[0]); it != static_cast<void**>(elements[1]); it++)
				if (*it)
					Release(*it);

			if (elements[0]) {
				void* allocatorOwner = reinterpret_cast<tGetAllocatorOwner>(Offsets::func_getAllocatorOwner.Get())();
				void* allocator = *reinterpret_cast<void**>(allocatorOwner);
				const size_t size = (std::max)(static_cast<size_t>(static_cast<uint8_t*>(elements[2]) - static_cast<uint8_t*>(elements[0])), static_cast<size_t>(1));
				reinterpret_cast<tAllocatorDeallocate>(VTable(allocator)[2])(allocator, nullptr, elements[0], 9, size);
			}

			Release(songs);

			if (!removals.empty())
				LOG_INFO("(PROFILE SAVE) Trimmed " << removals.size() << " PlaynextStats (kept " << remaining << ")" << std::endl);
		}

		void InitializePlaynextTrim() {
			// The replacement has the same cdecl signature, so the hook just jumps to it.
			if (MemUtil::PlaceHook(Offsets::func_playnextTrim, TrimPlaynext, 6)) {
				FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::func_playnextTrim.Get(), 6);
				LOG_INFO("(PROFILE SAVE) Replaced the PlaynextStats trim" << std::endl);
			}
			else
				LOG_ERROR("(PROFILE SAVE) Failed to replace the PlaynextStats trim" << std::endl);
		}

		/// <summary>
		/// SaveWrapper: the load's save guard, and the owner of each streamed save. Both of the groups below need it.
		/// </summary>
		bool InitializeSaveWrapper() {
			origSave = reinterpret_cast<tSaveCall>(CallTarget(Offsets::ptr_profileSaveCall));
			PatchGroup patches;
			if (!patches.Redirect(Offsets::ptr_profileSaveCall, SaveWrapper)) {
				patches.Undo();
				LOG_ERROR("(PROFILE SAVE) Failed to hook profile saves, not streaming saves or loading profiles in the background" << std::endl);
				return false;
			}
			return true;
		}

		void InitializeSaving() {
			HMODULE msvcr100 = GetModuleHandleA("msvcr100.dll");
			if (msvcr100) {
				gameVsnprintf = reinterpret_cast<tVsnprintf>(GetProcAddress(msvcr100, "_vsnprintf"));
				gameVscprintf = reinterpret_cast<tVscprintf>(GetProcAddress(msvcr100, "_vscprintf"));
			}
			if (!gameVsnprintf || !gameVscprintf) {
				LOG_ERROR("(PROFILE SAVE) Couldn't find the game's printf, not streaming profile saves" << std::endl);
				return;
			}

			origCompressBound = reinterpret_cast<tCompressBound>(CallTarget(Offsets::ptr_profileSaveCompressBoundCall));
			origCompress2 = reinterpret_cast<tCompress2>(CallTarget(Offsets::ptr_profileSaveCompress2Call));

			// All or nothing: the clone hook without the others would save an empty profile.
			PatchGroup patches;
			if (!patches.Redirect(Offsets::ptr_profileSaveCompressBoundCall, CompressBound)
				|| !patches.Redirect(Offsets::ptr_profileSaveCompress2Call, Compress2)
				|| !patches.Hook(Offsets::ptr_profileSavePrintRoot, printRootHook, 5)
				|| !patches.Hook(Offsets::ptr_profileSaveCloneSection, cloneSectionHook, 5)) {
				patches.Undo();
				LOG_ERROR("(PROFILE SAVE) Failed to hook the profile save, not streaming profile saves" << std::endl);
				return;
			}

			LOG_INFO("(PROFILE SAVE) Streaming profile saves" << std::endl);
		}

		void InitializeLoading() {
			origParse = reinterpret_cast<tJsonParse>(CallTarget(Offsets::ptr_profileParseCall));

			// The parse redirect only hands over what the tick parsed, and the tick needs it to use that parse.
			PatchGroup patches;
			if (!patches.Redirect(Offsets::ptr_profileParseCall, Parse)
				|| !patches.Hook(Offsets::ptr_profileLoadTick, loadTickHook, 7)) {
				patches.Undo();
				LOG_ERROR("(PROFILE SAVE) Failed to hook the profile load, loading profiles on the main thread" << std::endl);
				return;
			}

			LOG_INFO("(PROFILE SAVE) Loading profiles in the background" << std::endl);
		}

		void InitializeClearDocument() {
			PatchGroup patches;
			if (!patches.Hook(Offsets::ptr_profileLoadClearDocument, loadClearDocumentHook, 6)) {
				patches.Undo();
				LOG_ERROR("(PROFILE SAVE) Failed to free the loaded profile text" << std::endl);
			}
		}
	}

	/// <summary>
	/// Big profiles (100MB+ of JSON, 10M+ values) crash the game when it saves them, and make every save freeze the game for a while.
	/// The game is 32-bit without large address awareness, so it only has 2GB of address space, and a save used to need:
	///  - a deep copy of every section of the profile, as many small allocations as the profile itself,
	///  - the JSON as one string (grown by doubling, so up to 2x its size), a copy of it passed by value, and another copy for the writer,
	///  - a buffer for zlib's worst case output, the size of the JSON again.
	/// When one of those allocations fails the game writes through a null pointer (in the allocator, for the clone).
	/// This prints each section straight into a deflate stream instead, 1MB at a time, and gives the writer the compressed data.
	/// Only the compressed profile (a few MB) is ever held in memory. The file is the same format; only the compression level changes.
	/// It also frees the loaded JSON text once the profile is parsed, which the game otherwise keeps for the whole session.
	///
	/// Loading: the game reads the profile, waits for it and parses it in one tick, which freezes the game
	/// for minutes on a big profile. This spreads that over ticks and parses on a worker thread, so the game keeps rendering while it loads.
	/// The JSON code is safe to run next to the main thread: interned strings and numbers are locked (the game turns its intern locks on),
	/// ref counts are interlocked, the small object allocator locks, and nothing else can see the parsed tree until Finish attaches it.
	///
	/// This rewrites how the profile is saved, so none of it goes in unless all of these hold at startup:
	///  - FastProfileLoadAndSave is on,
	///  - BackupProfile is on,
	///  - the profiles, as they are right now, have a backup (one is made here if needed).
	/// Otherwise the game loads and saves profiles exactly as it always has.
	/// Both settings are read once at startup, here and in ProfileBackupsMod, so neither can change mid-session.
	/// </summary>
	void Initialize() {
		if (!Settings::IsOn(Settings::Setting::FastProfileLoadAndSave)) {
			LOG_INFO("(PROFILE SAVE) Fast profile load and save is off" << std::endl);
			return;
		}
		if (!Settings::IsOn(Settings::Setting::BackupProfile)) {
			LOG_WARNING("(PROFILE SAVE) Fast profile load and save needs Backup Profile on, leaving it off" << std::endl);
			return;
		}
		if (!ProfileBackups::BackUpBeforeHooking()) {
			LOG_ERROR("(PROFILE SAVE) The profiles have no backup, leaving fast profile load and save off" << std::endl);
			return;
		}

		// The read and decompress before the parse still happen on the main thread and can take a few seconds on a big profile.
		// Without this, Windows swaps the window for a "Not Responding" ghost (and offers to close the game) while they run.
		DisableProcessWindowsGhosting();

		InitializeNumberHash();
		InitializePlaynextTrim();
		InitializeClearDocument();
		if (InitializeSaveWrapper()) {
			InitializeSaving();
			InitializeLoading();
		}
	}
}
