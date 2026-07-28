#include "stdafx.h"
#include "AudioCaptureHook.hpp"
#include "ComVTable.hpp"

namespace Audio
{
	std::string DescribeFormat(const CaptureFormat& format)
	{
		std::stringstream description;

		switch (format.sampleFormat)
		{
			case SampleFormat::Float32: description << "float32"; break;
			case SampleFormat::Int32: description << "int32"; break;
			case SampleFormat::Int16: description << "int16"; break;
			default: description << "unsupported"; break;
		}

		description << " " << format.sampleRate << "Hz " << format.channelCount << "ch";
		return description.str();
	}
}

namespace Audio::CaptureHook
{
	namespace
	{
		// IUnknown occupies slots 0-2 in every COM interface.
		constexpr size_t SLOT_ENUMERATOR_ENUM_AUDIO_ENDPOINTS = 3;
		constexpr size_t SLOT_ENUMERATOR_GET_DEFAULT_AUDIO_ENDPOINT = 4;
		constexpr size_t SLOT_COLLECTION_ITEM = 4;
		constexpr size_t SLOT_DEVICE_ACTIVATE = 3;
		constexpr size_t SLOT_AUDIO_CLIENT_INITIALIZE = 3;
		constexpr size_t SLOT_AUDIO_CLIENT_GET_SERVICE = 14;
		constexpr size_t SLOT_CAPTURE_CLIENT_GET_BUFFER = 3;

		// Declared here rather than pulled from ksmedia.h, which drags in a large and
		// fussy dependency for two values.
		const GUID SUBFORMAT_IEEE_FLOAT = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };
		const GUID SUBFORMAT_PCM = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

		typedef HRESULT(WINAPI* CoCreateInstance_t)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
		typedef HRESULT(STDMETHODCALLTYPE* EnumAudioEndpoints_t)(IMMDeviceEnumerator*, EDataFlow, DWORD, IMMDeviceCollection**);
		typedef HRESULT(STDMETHODCALLTYPE* GetDefaultAudioEndpoint_t)(IMMDeviceEnumerator*, EDataFlow, ERole, IMMDevice**);
		typedef HRESULT(STDMETHODCALLTYPE* CollectionItem_t)(IMMDeviceCollection*, UINT, IMMDevice**);
		typedef HRESULT(STDMETHODCALLTYPE* DeviceActivate_t)(IMMDevice*, REFIID, DWORD, PROPVARIANT*, void**);
		typedef HRESULT(STDMETHODCALLTYPE* AudioClientInitialize_t)(IAudioClient*, AUDCLNT_SHAREMODE, DWORD, REFERENCE_TIME, REFERENCE_TIME, const WAVEFORMATEX*, LPCGUID);
		typedef HRESULT(STDMETHODCALLTYPE* AudioClientGetService_t)(IAudioClient*, REFIID, void**);
		typedef HRESULT(STDMETHODCALLTYPE* CaptureGetBuffer_t)(IAudioCaptureClient*, BYTE**, UINT32*, DWORD*, UINT64*, UINT64*);

		CoCreateInstance_t original_CoCreateInstance = nullptr;
		EnumAudioEndpoints_t original_EnumAudioEndpoints = nullptr;
		GetDefaultAudioEndpoint_t original_GetDefaultAudioEndpoint = nullptr;
		CollectionItem_t original_CollectionItem = nullptr;
		DeviceActivate_t original_DeviceActivate = nullptr;
		AudioClientInitialize_t original_AudioClientInitialize = nullptr;
		AudioClientGetService_t original_AudioClientGetService = nullptr;
		CaptureGetBuffer_t original_CaptureGetBuffer = nullptr;

		struct DiscoveredCaptureClient
		{
			IAudioCaptureClient* client = nullptr;
			CaptureFormat format;
		};

		// Discovery runs on whichever thread the game builds its audio graph on. The audio
		// thread only ever reads the atomics below, so it never contends for this.
		std::mutex discoveryMutex;
		std::map<IAudioClient*, CaptureFormat> formatsByClient;
		std::vector<DiscoveredCaptureClient> discoveredCaptureClients;

		std::atomic<IInputProcessor*> activeProcessor{ nullptr };
		std::atomic<IAudioCaptureClient*> selectedCaptureClient{ nullptr };
		std::atomic<bool> processingEnabled{ false };
		int requestedCaptureClientIndex = -1;
		CaptureFormat activeFormat;

		// ASIO tops out at a 2048 frame buffer, so this covers any packet the game can ask for.
		constexpr uint32_t MAX_CONVERSION_FRAMES = 4096;
		constexpr float INT32_TO_FLOAT = 1.0f / 2147483648.0f;
		constexpr float FLOAT_TO_INT32 = 2147483647.0f;

		// The audio thread reads these rather than activeFormat, which is only safe to touch
		// while holding discoveryMutex.
		std::atomic<SampleFormat> processingSampleFormat{ SampleFormat::Unsupported };
		std::atomic<uint32_t> processingChannelCount{ 0 };
		std::vector<float> conversionBuffer;

		struct RegisteredAsioDriver
		{
			GUID classId{};
			std::string name;
		};

		std::vector<RegisteredAsioDriver> registeredAsioDrivers;
		std::vector<std::string> loggedCreations;

		std::string GuidToString(const GUID& guid)
		{
			wchar_t wideText[64] = {};
			if (StringFromGUID2(guid, wideText, ARRAYSIZE(wideText)) == 0) return "{?}";

			char text[64] = {};
			WideCharToMultiByte(CP_UTF8, 0, wideText, -1, text, sizeof(text), nullptr, nullptr);
			return text;
		}

		// ASIO drivers register themselves under HKLM\SOFTWARE\ASIO, one subkey per driver,
		// each holding the CLSID that RS_ASIO will pass to CoCreateInstance. Reading it up
		// front turns an anonymous CLSID in the log into a driver name. This is a 32-bit
		// process, so the open redirects to WOW6432Node, which is the view RS_ASIO uses too.
		void ReadRegisteredAsioDrivers()
		{
			HKEY asioKey = nullptr;
			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\ASIO", 0, KEY_READ, &asioKey) != ERROR_SUCCESS)
			{
				LOG_WARNING("[CaptureHook] No ASIO drivers registered under HKLM\\SOFTWARE\\ASIO." << std::endl);
				return;
			}

			for (DWORD index = 0;; ++index)
			{
				char subKeyName[256] = {};
				DWORD nameLength = ARRAYSIZE(subKeyName);

				if (RegEnumKeyExA(asioKey, index, subKeyName, &nameLength, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
					break;

				HKEY driverKey = nullptr;
				if (RegOpenKeyExA(asioKey, subKeyName, 0, KEY_READ, &driverKey) != ERROR_SUCCESS) continue;

				char clsidText[64] = {};
				DWORD clsidSize = sizeof(clsidText);
				DWORD valueType = 0;

				if (RegQueryValueExA(driverKey, "CLSID", nullptr, &valueType, (LPBYTE)clsidText, &clsidSize) == ERROR_SUCCESS && valueType == REG_SZ)
				{
					wchar_t wideClsid[64] = {};
					MultiByteToWideChar(CP_ACP, 0, clsidText, -1, wideClsid, ARRAYSIZE(wideClsid));

					GUID classId{};
					if (SUCCEEDED(CLSIDFromString(wideClsid, &classId)))
					{
						registeredAsioDrivers.push_back({ classId, subKeyName });
						LOG_INFO("[CaptureHook] ASIO driver registered: " << subKeyName << " " << clsidText << std::endl);
					}
				}

				RegCloseKey(driverKey);
			}

			RegCloseKey(asioKey);
		}

		std::string FindAsioDriverName(const GUID& classId)
		{
			for (const RegisteredAsioDriver& driver : registeredAsioDrivers)
			{
				if (IsEqualCLSID(driver.classId, classId)) return driver.name;
			}

			return {};
		}

		// One line per unique class/interface pair. CoCreateInstance is called throughout the
		// game's life and repeats heavily, so logging every call buries the one we care about.
		void LogCreation(const GUID& classId, const GUID& interfaceId, HRESULT result)
		{
			const std::string classText = GuidToString(classId);
			const std::string interfaceText = GuidToString(interfaceId);

			{
				std::lock_guard<std::mutex> lock(discoveryMutex);

				const std::string key = classText + interfaceText;
				if (std::find(loggedCreations.begin(), loggedCreations.end(), key) != loggedCreations.end()) return;

				loggedCreations.push_back(key);
			}

			const std::string driverName = FindAsioDriverName(classId);

			if (!driverName.empty())
			{
				LOG_INFO("[CaptureHook] ASIO driver instantiated: " << driverName << " clsid=" << classText
					<< " iid=" << interfaceText << " hr=0x" << std::hex << result << std::dec << std::endl);
				return;
			}

			LOG_INFO("[CaptureHook] CoCreateInstance clsid=" << classText << " iid=" << interfaceText
				<< " hr=0x" << std::hex << result << std::dec << std::endl);
		}

		void HookCaptureClient(IAudioCaptureClient* captureClient);
		void HookAudioClient(IAudioClient* audioClient);
		void HookDevice(IMMDevice* device);
		void HookCollection(IMMDeviceCollection* collection);
		void HookEnumerator(IMMDeviceEnumerator* enumerator);

		CaptureFormat ReadFormat(const WAVEFORMATEX* waveFormat)
		{
			CaptureFormat format;
			if (!waveFormat) return format;

			format.sampleRate = waveFormat->nSamplesPerSec;
			format.channelCount = waveFormat->nChannels;

			WORD formatTag = waveFormat->wFormatTag;

			if (formatTag == WAVE_FORMAT_EXTENSIBLE && waveFormat->cbSize >= 22)
			{
				const WAVEFORMATEXTENSIBLE* extensible = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(waveFormat);

				if (IsEqualGUID(extensible->SubFormat, SUBFORMAT_IEEE_FLOAT))
					formatTag = WAVE_FORMAT_IEEE_FLOAT;
				else if (IsEqualGUID(extensible->SubFormat, SUBFORMAT_PCM))
					formatTag = WAVE_FORMAT_PCM;
			}

			if (formatTag == WAVE_FORMAT_IEEE_FLOAT && waveFormat->wBitsPerSample == 32)
				format.sampleFormat = SampleFormat::Float32;
			else if (formatTag == WAVE_FORMAT_PCM && waveFormat->wBitsPerSample == 32)
				format.sampleFormat = SampleFormat::Int32;
			else if (formatTag == WAVE_FORMAT_PCM && waveFormat->wBitsPerSample == 16)
				format.sampleFormat = SampleFormat::Int16;

			return format;
		}

		// Recomputes which capture client the audio thread should process. Called under
		// discoveryMutex whenever the discovered set or the requested index changes.
		//
		// Any change stops processing, and the caller re-enables it once happy with what was
		// selected. That is also what makes the buffer resize below safe: the audio thread
		// cannot be inside Process while processing is disabled.
		void RefreshSelection()
		{
			processingEnabled.store(false, std::memory_order_release);

			if (requestedCaptureClientIndex < 0 || requestedCaptureClientIndex >= (int)discoveredCaptureClients.size())
			{
				selectedCaptureClient.store(nullptr, std::memory_order_relaxed);
				processingSampleFormat.store(SampleFormat::Unsupported, std::memory_order_relaxed);
				processingChannelCount.store(0, std::memory_order_relaxed);
				return;
			}

			const DiscoveredCaptureClient& selected = discoveredCaptureClients[requestedCaptureClientIndex];
			activeFormat = selected.format;

			if (selected.format.channelCount > 0)
				conversionBuffer.assign((size_t)MAX_CONVERSION_FRAMES * selected.format.channelCount, 0.0f);

			processingSampleFormat.store(selected.format.sampleFormat, std::memory_order_relaxed);
			processingChannelCount.store(selected.format.channelCount, std::memory_order_relaxed);
			selectedCaptureClient.store(selected.client, std::memory_order_release);
		}

		HRESULT STDMETHODCALLTYPE Hook_CaptureGetBuffer(IAudioCaptureClient* self, BYTE** data, UINT32* frameCount, DWORD* flags, UINT64* devicePosition, UINT64* qpcPosition)
		{
			const HRESULT result = original_CaptureGetBuffer(self, data, frameCount, flags, devicePosition, qpcPosition);

			if (FAILED(result)) return result;
			if (!processingEnabled.load(std::memory_order_acquire)) return result;
			if (self != selectedCaptureClient.load(std::memory_order_relaxed)) return result;
			if (!data || !*data || !frameCount || *frameCount == 0) return result;
			if (flags && (*flags & AUDCLNT_BUFFERFLAGS_SILENT)) return result;

			IInputProcessor* processor = activeProcessor.load(std::memory_order_relaxed);
			if (!processor) return result;

			const uint32_t channels = processingChannelCount.load(std::memory_order_relaxed);
			const uint32_t frames = *frameCount;
			if (channels == 0 || frames > MAX_CONVERSION_FRAMES) return result;

			const size_t sampleCount = (size_t)frames * channels;

			switch (processingSampleFormat.load(std::memory_order_relaxed))
			{
				case SampleFormat::Float32:
					processor->Process(reinterpret_cast<float*>(*data), frames);
					break;

				// RS_ASIO negotiates 32 bit PCM because the M-Track reports ASIOSTInt32LSB and
				// it rejects float outright, so this is the path that actually runs. Converting
				// here keeps sample format out of the processors entirely.
				case SampleFormat::Int32:
				{
					int32_t* samples = reinterpret_cast<int32_t*>(*data);

					for (size_t i = 0; i < sampleCount; ++i)
						conversionBuffer[i] = (float)samples[i] * INT32_TO_FLOAT;

					processor->Process(conversionBuffer.data(), frames);

					for (size_t i = 0; i < sampleCount; ++i)
					{
						const float value = conversionBuffer[i];
						const float clamped = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
						samples[i] = (int32_t)(clamped * FLOAT_TO_INT32);
					}
					break;
				}

				default:
					break;
			}

			return result;
		}

		HRESULT STDMETHODCALLTYPE Hook_AudioClientInitialize(IAudioClient* self, AUDCLNT_SHAREMODE shareMode, DWORD streamFlags, REFERENCE_TIME bufferDuration, REFERENCE_TIME periodicity, const WAVEFORMATEX* waveFormat, LPCGUID audioSessionGuid)
		{
			const HRESULT result = original_AudioClientInitialize(self, shareMode, streamFlags, bufferDuration, periodicity, waveFormat, audioSessionGuid);
			if (FAILED(result)) return result;

			const CaptureFormat format = ReadFormat(waveFormat);

			{
				std::lock_guard<std::mutex> lock(discoveryMutex);
				formatsByClient[self] = format;
			}

			LOG_INFO("[CaptureHook] IAudioClient " << self << " initialized as " << DescribeFormat(format) << std::endl);
			return result;
		}

		HRESULT STDMETHODCALLTYPE Hook_AudioClientGetService(IAudioClient* self, REFIID riid, void** service)
		{
			const HRESULT result = original_AudioClientGetService(self, riid, service);
			if (FAILED(result) || !service || !*service) return result;
			if (!IsEqualIID(riid, __uuidof(IAudioCaptureClient))) return result;

			IAudioCaptureClient* captureClient = static_cast<IAudioCaptureClient*>(*service);
			int index = -1;
			CaptureFormat format;

			{
				std::lock_guard<std::mutex> lock(discoveryMutex);

				auto alreadySeen = std::find_if(discoveredCaptureClients.begin(), discoveredCaptureClients.end(),
					[captureClient](const DiscoveredCaptureClient& entry) { return entry.client == captureClient; });

				if (alreadySeen != discoveredCaptureClients.end()) return result;

				auto recordedFormat = formatsByClient.find(self);
				if (recordedFormat != formatsByClient.end()) format = recordedFormat->second;

				discoveredCaptureClients.push_back({ captureClient, format });
				index = (int)discoveredCaptureClients.size() - 1;

				RefreshSelection();
			}

			LOG_INFO("[CaptureHook] Capture client " << index << " at " << captureClient << ", format " << DescribeFormat(format) << std::endl);

			if (format.sampleFormat != SampleFormat::Float32)
				LOG_WARNING("[CaptureHook] Capture client " << index << " is not float32. Processing currently only handles float32." << std::endl);

			HookCaptureClient(captureClient);
			return result;
		}

		HRESULT STDMETHODCALLTYPE Hook_DeviceActivate(IMMDevice* self, REFIID riid, DWORD classContext, PROPVARIANT* activationParams, void** activated)
		{
			const HRESULT result = original_DeviceActivate(self, riid, classContext, activationParams, activated);
			if (FAILED(result) || !activated || !*activated) return result;

			if (IsEqualIID(riid, __uuidof(IAudioClient)))
				HookAudioClient(static_cast<IAudioClient*>(*activated));

			return result;
		}

		HRESULT STDMETHODCALLTYPE Hook_CollectionItem(IMMDeviceCollection* self, UINT deviceIndex, IMMDevice** device)
		{
			const HRESULT result = original_CollectionItem(self, deviceIndex, device);
			if (SUCCEEDED(result) && device && *device) HookDevice(*device);
			return result;
		}

		HRESULT STDMETHODCALLTYPE Hook_EnumAudioEndpoints(IMMDeviceEnumerator* self, EDataFlow dataFlow, DWORD stateMask, IMMDeviceCollection** collection)
		{
			const HRESULT result = original_EnumAudioEndpoints(self, dataFlow, stateMask, collection);
			if (SUCCEEDED(result) && collection && *collection) HookCollection(*collection);
			return result;
		}

		HRESULT STDMETHODCALLTYPE Hook_GetDefaultAudioEndpoint(IMMDeviceEnumerator* self, EDataFlow dataFlow, ERole role, IMMDevice** device)
		{
			const HRESULT result = original_GetDefaultAudioEndpoint(self, dataFlow, role, device);
			if (SUCCEEDED(result) && device && *device) HookDevice(*device);
			return result;
		}

		HRESULT WINAPI Hook_CoCreateInstance(REFCLSID classId, LPUNKNOWN outer, DWORD classContext, REFIID riid, LPVOID* created)
		{
			const HRESULT result = original_CoCreateInstance(classId, outer, classContext, riid, created);

			LogCreation(classId, riid, result);

			if (FAILED(result) || !created || !*created) return result;

			if (IsEqualIID(riid, __uuidof(IMMDeviceEnumerator)))
				HookEnumerator(static_cast<IMMDeviceEnumerator*>(*created));

			return result;
		}

		void HookCaptureClient(IAudioCaptureClient* captureClient)
		{
			static bool hooked = false;
			if (hooked) return;

			original_CaptureGetBuffer = (CaptureGetBuffer_t)ComVTable::PatchSlot(captureClient, SLOT_CAPTURE_CLIENT_GET_BUFFER, Hook_CaptureGetBuffer);

			if (!original_CaptureGetBuffer)
			{
				LOG_ERROR("[CaptureHook] Could not patch IAudioCaptureClient::GetBuffer." << std::endl);
				return;
			}

			hooked = true;
			LOG_INFO("[CaptureHook] IAudioCaptureClient::GetBuffer hooked." << std::endl);
		}

		void HookAudioClient(IAudioClient* audioClient)
		{
			static bool hooked = false;
			if (hooked) return;

			original_AudioClientInitialize = (AudioClientInitialize_t)ComVTable::PatchSlot(audioClient, SLOT_AUDIO_CLIENT_INITIALIZE, Hook_AudioClientInitialize);
			original_AudioClientGetService = (AudioClientGetService_t)ComVTable::PatchSlot(audioClient, SLOT_AUDIO_CLIENT_GET_SERVICE, Hook_AudioClientGetService);

			if (!original_AudioClientInitialize || !original_AudioClientGetService)
			{
				LOG_ERROR("[CaptureHook] Could not patch IAudioClient vtable." << std::endl);
				return;
			}

			hooked = true;
			LOG_INFO("[CaptureHook] IAudioClient hooked." << std::endl);
		}

		void HookDevice(IMMDevice* device)
		{
			static bool hooked = false;
			if (hooked) return;

			original_DeviceActivate = (DeviceActivate_t)ComVTable::PatchSlot(device, SLOT_DEVICE_ACTIVATE, Hook_DeviceActivate);

			if (!original_DeviceActivate)
			{
				LOG_ERROR("[CaptureHook] Could not patch IMMDevice::Activate." << std::endl);
				return;
			}

			hooked = true;
			LOG_INFO("[CaptureHook] IMMDevice::Activate hooked." << std::endl);
		}

		void HookCollection(IMMDeviceCollection* collection)
		{
			static bool hooked = false;
			if (hooked) return;

			original_CollectionItem = (CollectionItem_t)ComVTable::PatchSlot(collection, SLOT_COLLECTION_ITEM, Hook_CollectionItem);

			if (!original_CollectionItem)
			{
				LOG_ERROR("[CaptureHook] Could not patch IMMDeviceCollection::Item." << std::endl);
				return;
			}

			hooked = true;
			LOG_INFO("[CaptureHook] IMMDeviceCollection::Item hooked." << std::endl);
		}

		void HookEnumerator(IMMDeviceEnumerator* enumerator)
		{
			static bool hooked = false;
			if (hooked) return;

			original_EnumAudioEndpoints = (EnumAudioEndpoints_t)ComVTable::PatchSlot(enumerator, SLOT_ENUMERATOR_ENUM_AUDIO_ENDPOINTS, Hook_EnumAudioEndpoints);
			original_GetDefaultAudioEndpoint = (GetDefaultAudioEndpoint_t)ComVTable::PatchSlot(enumerator, SLOT_ENUMERATOR_GET_DEFAULT_AUDIO_ENDPOINT, Hook_GetDefaultAudioEndpoint);

			if (!original_EnumAudioEndpoints || !original_GetDefaultAudioEndpoint)
			{
				LOG_ERROR("[CaptureHook] Could not patch IMMDeviceEnumerator vtable." << std::endl);
				return;
			}

			hooked = true;
			LOG_INFO("[CaptureHook] IMMDeviceEnumerator hooked." << std::endl);
		}
	}

	void Install()
	{
		static bool installed = false;
		if (installed) return;
		installed = true;

		if (GetModuleHandleA("RS_ASIO.dll"))
		{
			LOG_WARNING("[CaptureHook] RS_ASIO is loaded. It replaces the game's enumerator at the call site,"
				" so the CoCreateInstance detour will not see it. Expect no chain below to be reported." << std::endl);
		}

		ReadRegisteredAsioDrivers();

		original_CoCreateInstance = (CoCreateInstance_t)DetourFunction((byte*)CoCreateInstance, (byte*)Hook_CoCreateInstance);

		if (!original_CoCreateInstance)
		{
			LOG_ERROR("[CaptureHook] Failed to detour CoCreateInstance. Input capture is unavailable." << std::endl);
			return;
		}

		LOG_INFO("[CaptureHook] Installed. Watching for the game's audio chain." << std::endl);
	}

	void SetProcessor(IInputProcessor* inputProcessor)
	{
		activeProcessor.store(inputProcessor, std::memory_order_relaxed);
	}

	void SetCaptureClientIndex(int index)
	{
		std::lock_guard<std::mutex> lock(discoveryMutex);
		requestedCaptureClientIndex = index;
		RefreshSelection();
	}

	void SetProcessingEnabled(bool enabled)
	{
		if (enabled && !activeProcessor.load(std::memory_order_relaxed))
		{
			LOG_ERROR("[CaptureHook] Refusing to enable processing with no processor set." << std::endl);
			return;
		}

		if (enabled && !selectedCaptureClient.load(std::memory_order_relaxed))
		{
			LOG_ERROR("[CaptureHook] Refusing to enable processing before a capture client is selected." << std::endl);
			return;
		}

		processingEnabled.store(enabled, std::memory_order_release);
		LOG_INFO("[CaptureHook] Processing " << (enabled ? "enabled" : "disabled") << std::endl);
	}

	bool IsProcessingEnabled()
	{
		return processingEnabled.load(std::memory_order_acquire);
	}

	int GetDiscoveredCaptureClientCount()
	{
		std::lock_guard<std::mutex> lock(discoveryMutex);
		return (int)discoveredCaptureClients.size();
	}

	const CaptureFormat& GetActiveFormat()
	{
		return activeFormat;
	}
}
