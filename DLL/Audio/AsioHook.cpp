#include "stdafx.h"
#include "AsioHook.hpp"
#include "ComVTable.hpp"

namespace Audio::AsioHook
{
	namespace
	{
		// Minimal slice of the ASIO SDK. Pulling in the real headers means agreeing to
		// Steinberg's licence for a handful of structures, so the ones we touch are declared
		// here instead. Layout must match the SDK exactly or the driver will write past us.
		typedef long ASIOBool;
		typedef long ASIOError;
		typedef double ASIOSampleRate;

		constexpr ASIOError ASE_OK = 0;
		constexpr long ASIOSTInt32LSB = 18;
		constexpr long MAX_ASIO_CHANNEL_NAME = 32;

		struct ASIOBufferInfo
		{
			ASIOBool isInput;
			long channelNum;
			void* buffers[2];
		};

		struct ASIOChannelInfo
		{
			long channel;
			ASIOBool isInput;
			ASIOBool isActive;
			long channelGroup;
			long type;
			char name[MAX_ASIO_CHANNEL_NAME];
		};

		struct ASIOTime;

		struct ASIOCallbacks
		{
			void (*bufferSwitch)(long doubleBufferIndex, ASIOBool directProcess);
			void (*sampleRateDidChange)(ASIOSampleRate sRate);
			long (*asioMessage)(long selector, long value, void* message, double* opt);
			ASIOTime* (*bufferSwitchTimeInfo)(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
		};

		// IUnknown occupies slots 0-2; the ASIO methods follow in declaration order.
		constexpr size_t SLOT_ASIO_GET_SAMPLE_RATE = 13;
		constexpr size_t SLOT_ASIO_GET_CHANNEL_INFO = 18;
		constexpr size_t SLOT_ASIO_CREATE_BUFFERS = 19;
		constexpr size_t SLOT_CLASS_FACTORY_CREATE_INSTANCE = 3;

		constexpr long MAX_BUFFER_FRAMES = 4096;
		constexpr float INT32_TO_FLOAT = 1.0f / 2147483648.0f;
		constexpr float FLOAT_TO_INT32 = 2147483647.0f;

		typedef HRESULT(STDMETHODCALLTYPE* DllGetClassObject_t)(REFCLSID, REFIID, LPVOID*);
		typedef HRESULT(STDMETHODCALLTYPE* CreateInstance_t)(IClassFactory*, IUnknown*, REFIID, void**);
		typedef ASIOError(__fastcall* CreateBuffers_t)(void* self, void* unused, ASIOBufferInfo*, long, long, ASIOCallbacks*);
		typedef ASIOError(__fastcall* GetChannelInfo_t)(void* self, void* unused, ASIOChannelInfo*);
		typedef ASIOError(__fastcall* GetSampleRate_t)(void* self, void* unused, ASIOSampleRate*);

		DllGetClassObject_t original_DllGetClassObject = nullptr;
		CreateInstance_t original_CreateInstance = nullptr;
		CreateBuffers_t original_CreateBuffers = nullptr;

		GUID driverClassId{};
		std::string driverName;

		// Written once during createBuffers, read on the ASIO callback thread afterwards.
		void* inputBuffers[2][2] = {};		// [channelIndex][doubleBufferIndex]
		long inputChannelNumbers[2] = { -1, -1 };
		int discoveredInputChannels = 0;
		long activeBufferFrames = 0;
		long activeSampleType = -1;

		ASIOCallbacks originalCallbacks{};
		ASIOCallbacks hookedCallbacks{};

		CaptureFormat format;
		std::vector<float> conversionBuffer;

		std::atomic<IInputProcessor*> activeProcessor{ nullptr };
		std::atomic<int> selectedInputChannel{ -1 };
		std::atomic<bool> processingEnabled{ false };
		bool autoEnabledOnce = false;

		std::string ReadDriverNameFromRsAsioIni()
		{
			CSimpleIniA reader;
			if (reader.LoadFile("RS_ASIO.ini") < 0) return {};

			// Input.0 is the guitar RS_ASIO feeds the game. Falling back to the output driver
			// covers configs that only name it once, since one interface usually serves both.
			const char* input = reader.GetValue("Asio.Input.0", "Driver", "");
			if (input && *input) return input;

			const char* output = reader.GetValue("Asio.Output", "Driver", "");
			if (output && *output) return output;

			return {};
		}

		// RS_ASIO's Channel setting is the ASIO channel number the guitar arrives on, which
		// is also the order channels appear in createBuffers, so it maps directly onto our
		// discovered-input index.
		int ReadInputChannelFromRsAsioIni()
		{
			CSimpleIniA reader;
			if (reader.LoadFile("RS_ASIO.ini") < 0) return 0;

			return (int)reader.GetLongValue("Asio.Input.0", "Channel", 0);
		}

		bool ReadDriverClassId(const std::string& name, GUID& classId)
		{
			std::string keyPath = "SOFTWARE\\ASIO\\" + name;

			HKEY driverKey = nullptr;
			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath.c_str(), 0, KEY_READ, &driverKey) != ERROR_SUCCESS)
				return false;

			char clsidText[64] = {};
			DWORD clsidSize = sizeof(clsidText);
			DWORD valueType = 0;

			const bool read = RegQueryValueExA(driverKey, "CLSID", nullptr, &valueType, (LPBYTE)clsidText, &clsidSize) == ERROR_SUCCESS
				&& valueType == REG_SZ;

			RegCloseKey(driverKey);
			if (!read) return false;

			wchar_t wideClsid[64] = {};
			MultiByteToWideChar(CP_ACP, 0, clsidText, -1, wideClsid, ARRAYSIZE(wideClsid));

			return SUCCEEDED(CLSIDFromString(wideClsid, &classId));
		}

		std::wstring ReadDriverModulePath(const GUID& classId)
		{
			wchar_t clsidText[64] = {};
			if (StringFromGUID2(classId, clsidText, ARRAYSIZE(clsidText)) == 0) return {};

			std::wstring keyPath = L"CLSID\\";
			keyPath += clsidText;
			keyPath += L"\\InprocServer32";

			HKEY serverKey = nullptr;
			if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, KEY_READ, &serverKey) != ERROR_SUCCESS)
				return {};

			wchar_t path[MAX_PATH] = {};
			DWORD pathSize = sizeof(path);
			DWORD valueType = 0;

			const bool read = RegQueryValueExW(serverKey, nullptr, nullptr, &valueType, (LPBYTE)path, &pathSize) == ERROR_SUCCESS
				&& (valueType == REG_SZ || valueType == REG_EXPAND_SZ);

			RegCloseKey(serverKey);
			if (!read) return {};

			return path;
		}

		void ProcessInputBuffer(long doubleBufferIndex)
		{
			const int channel = selectedInputChannel.load(std::memory_order_relaxed);
			if (channel < 0 || channel >= discoveredInputChannels) return;

			IInputProcessor* processor = activeProcessor.load(std::memory_order_relaxed);
			if (!processor) return;

			if (activeSampleType != ASIOSTInt32LSB) return;
			if (activeBufferFrames <= 0 || activeBufferFrames > MAX_BUFFER_FRAMES) return;

			int32_t* samples = reinterpret_cast<int32_t*>(inputBuffers[channel][doubleBufferIndex]);
			if (!samples) return;

			const size_t count = (size_t)activeBufferFrames;

			for (size_t i = 0; i < count; ++i)
				conversionBuffer[i] = (float)samples[i] * INT32_TO_FLOAT;

			processor->Process(conversionBuffer.data(), (uint32_t)activeBufferFrames);

			for (size_t i = 0; i < count; ++i)
			{
				const float value = conversionBuffer[i];
				const float clamped = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
				samples[i] = (int32_t)(clamped * FLOAT_TO_INT32);
			}
		}

		// The driver fills the input buffers before calling this, and RS_ASIO copies them out
		// inside the original callback, so editing here lands ahead of everything downstream.
		void Hook_BufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
		{
			if (processingEnabled.load(std::memory_order_acquire))
				ProcessInputBuffer(doubleBufferIndex);

			if (originalCallbacks.bufferSwitch)
				originalCallbacks.bufferSwitch(doubleBufferIndex, directProcess);
		}

		ASIOTime* Hook_BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
		{
			if (processingEnabled.load(std::memory_order_acquire))
				ProcessInputBuffer(doubleBufferIndex);

			if (originalCallbacks.bufferSwitchTimeInfo)
				return originalCallbacks.bufferSwitchTimeInfo(params, doubleBufferIndex, directProcess);

			return params;
		}

		ASIOError __fastcall Hook_CreateBuffers(void* self, void* unused, ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks)
		{
			// Swap in our own callback struct before the driver stores it. The driver keeps the
			// pointer, so ours has to outlive the call, hence the file scope copy.
			if (callbacks)
			{
				originalCallbacks = *callbacks;

				hookedCallbacks.bufferSwitch = Hook_BufferSwitch;
				hookedCallbacks.sampleRateDidChange = originalCallbacks.sampleRateDidChange;
				hookedCallbacks.asioMessage = originalCallbacks.asioMessage;
				hookedCallbacks.bufferSwitchTimeInfo = originalCallbacks.bufferSwitchTimeInfo ? Hook_BufferSwitchTimeInfo : nullptr;
			}

			const ASIOError result = original_CreateBuffers(self, unused, bufferInfos, numChannels, bufferSize, callbacks ? &hookedCallbacks : nullptr);

			if (result != ASE_OK)
			{
				LOG_ERROR("[AsioHook] createBuffers failed with " << result << std::endl);
				return result;
			}

			activeBufferFrames = bufferSize;
			discoveredInputChannels = 0;

			for (long i = 0; i < numChannels && discoveredInputChannels < 2; ++i)
			{
				if (!bufferInfos[i].isInput) continue;

				const int index = discoveredInputChannels++;
				inputChannelNumbers[index] = bufferInfos[i].channelNum;
				inputBuffers[index][0] = bufferInfos[i].buffers[0];
				inputBuffers[index][1] = bufferInfos[i].buffers[1];

				ASIOChannelInfo channelInfo{};
				channelInfo.channel = bufferInfos[i].channelNum;
				channelInfo.isInput = 1;

				GetChannelInfo_t getChannelInfo = (GetChannelInfo_t)ComVTable::GetVTable(self)[SLOT_ASIO_GET_CHANNEL_INFO];
				if (getChannelInfo(self, nullptr, &channelInfo) == ASE_OK)
				{
					activeSampleType = channelInfo.type;
					LOG_INFO("[AsioHook] Input " << index << " channel " << channelInfo.channel
						<< " type " << channelInfo.type << " name " << channelInfo.name << std::endl);
				}
			}

			format.sampleFormat = activeSampleType == ASIOSTInt32LSB ? SampleFormat::Int32 : SampleFormat::Unsupported;
			format.channelCount = 1;		// ASIO buffers are per channel, never interleaved.

			ASIOSampleRate sampleRate = 0;
			GetSampleRate_t getSampleRate = (GetSampleRate_t)ComVTable::GetVTable(self)[SLOT_ASIO_GET_SAMPLE_RATE];
			if (getSampleRate(self, nullptr, &sampleRate) == ASE_OK && sampleRate > 0)
			{
				format.sampleRate = (uint32_t)sampleRate;
			}
			else
			{
				LOG_WARNING("[AsioHook] Driver did not report a sample rate; processors cannot be prepared." << std::endl);
			}

			conversionBuffer.assign((size_t)MAX_BUFFER_FRAMES, 0.0f);

			LOG_INFO("[AsioHook] createBuffers: " << numChannels << " channels, " << bufferSize
				<< " frames, " << discoveredInputChannels << " input(s) captured, " << format.sampleRate << " Hz" << std::endl);

			if (format.sampleFormat != SampleFormat::Int32)
				LOG_WARNING("[AsioHook] Input sample type " << activeSampleType << " is not ASIOSTInt32LSB; processing stays off." << std::endl);

			return result;
		}

		HRESULT STDMETHODCALLTYPE Hook_CreateInstance(IClassFactory* self, IUnknown* outer, REFIID riid, void** created)
		{
			const HRESULT result = original_CreateInstance(self, outer, riid, created);
			if (FAILED(result) || !created || !*created) return result;

			static bool hooked = false;
			if (hooked) return result;

			original_CreateBuffers = (CreateBuffers_t)ComVTable::PatchSlot(*created, SLOT_ASIO_CREATE_BUFFERS, Hook_CreateBuffers);

			if (!original_CreateBuffers)
			{
				LOG_ERROR("[AsioHook] Could not patch IASIO::createBuffers." << std::endl);
				return result;
			}

			hooked = true;
			LOG_INFO("[AsioHook] IASIO instance at " << *created << ", createBuffers hooked." << std::endl);
			return result;
		}

		HRESULT STDMETHODCALLTYPE Hook_DllGetClassObject(REFCLSID classId, REFIID riid, LPVOID* factory)
		{
			const HRESULT result = original_DllGetClassObject(classId, riid, factory);
			if (FAILED(result) || !factory || !*factory) return result;

			if (!IsEqualCLSID(classId, driverClassId)) return result;

			static bool hooked = false;
			if (hooked) return result;

			original_CreateInstance = (CreateInstance_t)ComVTable::PatchSlot(*factory, SLOT_CLASS_FACTORY_CREATE_INSTANCE, Hook_CreateInstance);

			if (!original_CreateInstance)
			{
				LOG_ERROR("[AsioHook] Could not patch IClassFactory::CreateInstance." << std::endl);
				return result;
			}

			hooked = true;
			LOG_INFO("[AsioHook] Class factory for " << driverName << " hooked." << std::endl);
			return result;
		}
	}

	void Install()
	{
		static bool installed = false;
		if (installed) return;
		installed = true;

		driverName = ReadDriverNameFromRsAsioIni();

		if (driverName.empty())
		{
			LOG_INFO("[AsioHook] No ASIO driver named in RS_ASIO.ini; nothing to hook." << std::endl);
			return;
		}

		if (!ReadDriverClassId(driverName, driverClassId))
		{
			LOG_ERROR("[AsioHook] No CLSID under HKLM\\SOFTWARE\\ASIO for \"" << driverName << "\"" << std::endl);
			return;
		}

		const std::wstring modulePath = ReadDriverModulePath(driverClassId);

		if (modulePath.empty())
		{
			LOG_ERROR("[AsioHook] No InprocServer32 path for \"" << driverName << "\"" << std::endl);
			return;
		}

		// Loading it now means RS_ASIO's own load later returns this same module, already
		// detoured. The driver is loaded either way; we are only changing when.
		HMODULE driverModule = LoadLibraryW(modulePath.c_str());

		if (!driverModule)
		{
			LOG_ERROR("[AsioHook] LoadLibrary failed for the ASIO driver, error " << GetLastError() << std::endl);
			return;
		}

		FARPROC classObjectEntry = GetProcAddress(driverModule, "DllGetClassObject");

		if (!classObjectEntry)
		{
			LOG_ERROR("[AsioHook] ASIO driver exports no DllGetClassObject." << std::endl);
			return;
		}

		original_DllGetClassObject = (DllGetClassObject_t)DetourFunction((byte*)classObjectEntry, (byte*)Hook_DllGetClassObject);

		if (!original_DllGetClassObject)
		{
			LOG_ERROR("[AsioHook] Failed to detour DllGetClassObject." << std::endl);
			return;
		}

		LOG_INFO("[AsioHook] Watching \"" << driverName << "\" for instantiation." << std::endl);

		selectedInputChannel.store(ReadInputChannelFromRsAsioIni(), std::memory_order_relaxed);
	}

	void Poll()
	{
		// One-shot: enables when the driver comes up, but never fights a manual disable.
		if (autoEnabledOnce) return;
		if (processingEnabled.load(std::memory_order_relaxed)) return;
		if (discoveredInputChannels == 0) return;
		if (!format.IsUsable()) return;

		IInputProcessor* processor = activeProcessor.load(std::memory_order_relaxed);
		if (!processor) return;

		autoEnabledOnce = true;

		// Runs on the game thread while processing is still disabled, so the processor is
		// free to allocate here before the audio thread ever calls Process.
		processor->Prepare(format);
		LOG_INFO("[AsioHook] Processor latency " << processor->GetLatencyFrames() << " frames" << std::endl);

		SetProcessingEnabled(true);
	}

	void SetProcessor(IInputProcessor* inputProcessor)
	{
		activeProcessor.store(inputProcessor, std::memory_order_relaxed);
	}

	void SetInputChannel(int channelIndex)
	{
		processingEnabled.store(false, std::memory_order_release);
		selectedInputChannel.store(channelIndex, std::memory_order_relaxed);
	}

	void SetProcessingEnabled(bool enabled)
	{
		if (enabled && !activeProcessor.load(std::memory_order_relaxed))
		{
			LOG_ERROR("[AsioHook] Refusing to enable processing with no processor set." << std::endl);
			return;
		}

		if (enabled && format.sampleFormat != SampleFormat::Int32)
		{
			LOG_ERROR("[AsioHook] Refusing to enable processing before an int32 input is known." << std::endl);
			return;
		}

		const int channel = selectedInputChannel.load(std::memory_order_relaxed);

		if (enabled && (channel < 0 || channel >= discoveredInputChannels))
		{
			LOG_ERROR("[AsioHook] Refusing to enable processing without a valid input channel." << std::endl);
			return;
		}

		processingEnabled.store(enabled, std::memory_order_release);
		LOG_INFO("[AsioHook] Processing " << (enabled ? "enabled" : "disabled") << std::endl);
	}

	bool IsProcessingEnabled()
	{
		return processingEnabled.load(std::memory_order_acquire);
	}

	int GetInputChannelCount()
	{
		return discoveredInputChannels;
	}

	long GetBufferSizeFrames()
	{
		return activeBufferFrames;
	}

	const CaptureFormat& GetFormat()
	{
		return format;
	}
}
