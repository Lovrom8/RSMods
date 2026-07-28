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
			else if (formatTag == WAVE_FORMAT_PCM && waveFormat->wBitsPerSample == 16)
				format.sampleFormat = SampleFormat::Int16;

			return format;
		}

		// Recomputes which capture client the audio thread should process. Called under
		// discoveryMutex whenever the discovered set or the requested index changes.
		void RefreshSelection()
		{
			if (requestedCaptureClientIndex < 0 || requestedCaptureClientIndex >= (int)discoveredCaptureClients.size())
			{
				selectedCaptureClient.store(nullptr, std::memory_order_relaxed);
				processingEnabled.store(false, std::memory_order_release);
				return;
			}

			const DiscoveredCaptureClient& selected = discoveredCaptureClients[requestedCaptureClientIndex];
			activeFormat = selected.format;
			selectedCaptureClient.store(selected.client, std::memory_order_relaxed);
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

			processor->Process(reinterpret_cast<float*>(*data), *frameCount);
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
