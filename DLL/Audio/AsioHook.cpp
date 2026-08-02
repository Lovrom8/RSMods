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
		constexpr long ASIOSTInt16LSB = 16;
		constexpr long ASIOSTInt24LSB = 17;
		constexpr long ASIOSTInt32LSB = 18;
		constexpr long ASIOSTFloat32LSB = 19;
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
		constexpr int MAX_INPUT_CHANNELS = 16;
		constexpr float INT16_TO_FLOAT = 1.0f / 32768.0f;
		constexpr float INT24_TO_FLOAT = 1.0f / 8388608.0f;
		constexpr float INT32_TO_FLOAT = 1.0f / 2147483648.0f;

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
		void* inputBuffers[MAX_INPUT_CHANNELS][2] = {};		// [inputIndex][doubleBufferIndex]
		long inputChannelNumbers[MAX_INPUT_CHANNELS] = {};
		long inputSampleTypes[MAX_INPUT_CHANNELS] = {};
		int discoveredInputChannels = 0;
		long activeBufferFrames = 0;

		ASIOCallbacks originalCallbacks{};
		ASIOCallbacks hookedCallbacks{};

		CaptureFormat format;
		std::vector<float> conversionBuffer;

		std::atomic<IInputProcessor*> activeProcessor{ nullptr };
		std::atomic<int> selectedInputChannel{ -1 };
		std::atomic<bool> processingEnabled{ false };
		bool autoEnabledOnce = false;

		SampleFormat GetSampleFormat(long sampleType)
		{
			switch (sampleType)
			{
			case ASIOSTFloat32LSB:
				return SampleFormat::Float32;
			case ASIOSTInt32LSB:
				return SampleFormat::Int32;
			case ASIOSTInt24LSB:
				return SampleFormat::Int24;
			case ASIOSTInt16LSB:
				return SampleFormat::Int16;
			default:
				return SampleFormat::Unsupported;
			}
		}

		float ClampSample(float value)
		{
			if (!std::isfinite(value)) return 0.0f;
			if (value < -1.0f) return -1.0f;
			if (value > 1.0f) return 1.0f;
			return value;
		}

		int16_t FloatToInt16(float value)
		{
			const float clamped = ClampSample(value);
			if (clamped <= -1.0f) return -32768;
			if (clamped >= 1.0f) return 32767;
			return (int16_t)(clamped * 32768.0f);
		}

		int32_t FloatToInt24(float value)
		{
			const float clamped = ClampSample(value);
			if (clamped <= -1.0f) return -8388608;
			if (clamped >= 1.0f) return 8388607;
			return (int32_t)(clamped * 8388608.0f);
		}

		int32_t FloatToInt32(float value)
		{
			const float clamped = ClampSample(value);
			if (clamped <= -1.0f) return (-2147483647 - 1);
			if (clamped >= 1.0f) return 2147483647;
			return (int32_t)(clamped * 2147483648.0f);
		}

		int32_t ReadInt24(const uint8_t* sample)
		{
			const uint32_t packed = (uint32_t)sample[0]
				| ((uint32_t)sample[1] << 8)
				| ((uint32_t)sample[2] << 16);

			return (packed & 0x00800000u) != 0
				? (int32_t)packed - 0x01000000
				: (int32_t)packed;
		}

		void WriteInt24(int32_t value, uint8_t* sample)
		{
			const uint32_t packed = (uint32_t)value;
			sample[0] = (uint8_t)packed;
			sample[1] = (uint8_t)(packed >> 8);
			sample[2] = (uint8_t)(packed >> 16);
		}

		bool ConvertInputToFloat(void* input, long sampleType, size_t count)
		{
			switch (sampleType)
			{
			case ASIOSTFloat32LSB:
			{
				const float* samples = reinterpret_cast<const float*>(input);
				for (size_t i = 0; i < count; ++i)
					conversionBuffer[i] = samples[i];
				return true;
			}
			case ASIOSTInt32LSB:
			{
				const int32_t* samples = reinterpret_cast<const int32_t*>(input);
				for (size_t i = 0; i < count; ++i)
					conversionBuffer[i] = (float)samples[i] * INT32_TO_FLOAT;
				return true;
			}
			case ASIOSTInt24LSB:
			{
				const uint8_t* samples = reinterpret_cast<const uint8_t*>(input);
				for (size_t i = 0; i < count; ++i)
					conversionBuffer[i] = (float)ReadInt24(samples + i * 3) * INT24_TO_FLOAT;
				return true;
			}
			case ASIOSTInt16LSB:
			{
				const int16_t* samples = reinterpret_cast<const int16_t*>(input);
				for (size_t i = 0; i < count; ++i)
					conversionBuffer[i] = (float)samples[i] * INT16_TO_FLOAT;
				return true;
			}
			default:
				return false;
			}
		}

		void ConvertFloatToInput(void* input, long sampleType, size_t count)
		{
			switch (sampleType)
			{
			case ASIOSTFloat32LSB:
			{
				float* samples = reinterpret_cast<float*>(input);
				for (size_t i = 0; i < count; ++i)
					samples[i] = ClampSample(conversionBuffer[i]);
				break;
			}
			case ASIOSTInt32LSB:
			{
				int32_t* samples = reinterpret_cast<int32_t*>(input);
				for (size_t i = 0; i < count; ++i)
					samples[i] = FloatToInt32(conversionBuffer[i]);
				break;
			}
			case ASIOSTInt24LSB:
			{
				uint8_t* samples = reinterpret_cast<uint8_t*>(input);
				for (size_t i = 0; i < count; ++i)
					WriteInt24(FloatToInt24(conversionBuffer[i]), samples + i * 3);
				break;
			}
			case ASIOSTInt16LSB:
			{
				int16_t* samples = reinterpret_cast<int16_t*>(input);
				for (size_t i = 0; i < count; ++i)
					samples[i] = FloatToInt16(conversionBuffer[i]);
				break;
			}
			}
		}

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

		// RS_ASIO's Channel setting is the ASIO channel number the guitar arrives on. It is
		// matched against the channelNum of the buffers the driver hands out, since RS_ASIO
		// creates buffers for every available input and accepts any nonnegative channel.
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

		int FindInputIndexForChannel(int channelNumber)
		{
			for (int i = 0; i < discoveredInputChannels; ++i)
			{
				if (inputChannelNumbers[i] == channelNumber) return i;
			}

			return -1;
		}

		void ProcessInputBuffer(long doubleBufferIndex)
		{
			const int channel = FindInputIndexForChannel(selectedInputChannel.load(std::memory_order_relaxed));
			if (channel < 0) return;

			IInputProcessor* processor = activeProcessor.load(std::memory_order_relaxed);
			if (!processor) return;

			if (activeBufferFrames <= 0 || activeBufferFrames > MAX_BUFFER_FRAMES) return;

			void* samples = inputBuffers[channel][doubleBufferIndex];
			if (!samples) return;

			const size_t count = (size_t)activeBufferFrames;
			const long sampleType = inputSampleTypes[channel];
			if (!ConvertInputToFloat(samples, sampleType, count)) return;

			processor->Process(conversionBuffer.data(), (uint32_t)activeBufferFrames);
			ConvertFloatToInput(samples, sampleType, count);
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

			for (long i = 0; i < numChannels && discoveredInputChannels < MAX_INPUT_CHANNELS; ++i)
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
					inputSampleTypes[index] = channelInfo.type;
					LOG_INFO("[AsioHook] Input " << index << " channel " << channelInfo.channel
						<< " type " << channelInfo.type << " name " << channelInfo.name << std::endl);
				}
				else
				{
					inputSampleTypes[index] = -1;
				}
			}

			const int selectedIndex = FindInputIndexForChannel(selectedInputChannel.load(std::memory_order_relaxed));
			const long selectedSampleType = selectedIndex >= 0 ? inputSampleTypes[selectedIndex] : -1;
			format.sampleFormat = GetSampleFormat(selectedSampleType);
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

			if (format.sampleFormat == SampleFormat::Unsupported)
				LOG_WARNING("[AsioHook] Selected input sample type " << selectedSampleType << " is unsupported; processing stays off." << std::endl);

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

		if (enabled && !format.IsUsable())
		{
			LOG_ERROR("[AsioHook] Refusing to enable processing before a supported input format is known." << std::endl);
			return;
		}

		const int channel = selectedInputChannel.load(std::memory_order_relaxed);

		if (enabled && FindInputIndexForChannel(channel) < 0)
		{
			LOG_ERROR("[AsioHook] Refusing to enable processing: no discovered input has channel "
				<< channel << "." << std::endl);
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
