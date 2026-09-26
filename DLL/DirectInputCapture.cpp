#include "stdafx.h"
#include "DirectInputCapture.hpp"
#include "Framework/InputCapture.hpp"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// Hooking the game's DirectInput8Create never fires: the game creates its interface before we load.
// So we make a throwaway mouse device of our own and patch GetDeviceState / GetDeviceData in
// dinput8.dll's device vtables, which every device object shares, the game's included.
namespace DirectInputCapture
{
	namespace
	{
		constexpr size_t GET_CAPABILITIES_SLOT = 3;
		constexpr size_t GET_DEVICE_STATE_SLOT = 9;
		constexpr size_t GET_DEVICE_DATA_SLOT = 10;

		using DirectInput8CreateFunction = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
		using GetCapabilitiesFunction = HRESULT(STDMETHODCALLTYPE*)(IUnknown*, LPDIDEVCAPS);
		using GetDeviceStateFunction = HRESULT(STDMETHODCALLTYPE*)(IUnknown*, DWORD, LPVOID);
		using GetDeviceDataFunction = HRESULT(STDMETHODCALLTYPE*)(IUnknown*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);

		// Local copies so we don't need dxguid.lib.
		const GUID DIRECT_INPUT8_A_GUID =
		{ 0xbf798031, 0x483a, 0x4da2, { 0xaa, 0x99, 0x5d, 0x64, 0xed, 0x36, 0x97, 0x00 } };
		const GUID DIRECT_INPUT8_W_GUID =
		{ 0xbf798030, 0x483a, 0x4da2, { 0xaa, 0x99, 0x5d, 0x64, 0xed, 0x36, 0x97, 0x00 } };
		const GUID SYS_MOUSE_GUID =
		{ 0x6f1d2b60, 0xd5a0, 0x11cf, { 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

		// The A and W device classes have their own vtables; the hooks go into both.
		MemUtil::VTablePatcher patcher;
		std::atomic<bool> loggedBlock{ false };

		// Cached per device object; the game keeps a handful for its whole life. GetCapabilities isn't
		// patched, so the vtable still holds the real one.
		bool IsMouse(IUnknown* self)
		{
			thread_local struct { IUnknown* device; bool mouse; } cache[8] = {};
			thread_local unsigned next = 0;

			for (const auto& entry : cache)
			{
				if (entry.device == self)
					return entry.mouse;
			}

			const auto getCapabilities = reinterpret_cast<GetCapabilitiesFunction>(MemUtil::GetVTable(self)[GET_CAPABILITIES_SLOT]);
			DIDEVCAPS caps{};
			caps.dwSize = sizeof(caps);
			const bool mouse = SUCCEEDED(getCapabilities(self, &caps)) && GET_DIDEVICE_TYPE(caps.dwDevType) == DI8DEVTYPE_MOUSE;

			cache[next++ % 8] = { self, mouse };
			return mouse;
		}

		void LogFirstBlock()
		{
			if (!loggedBlock.exchange(true, std::memory_order_relaxed))
				LOG_INFO("[InputCapture] Blocking Rocksmith's DirectInput mouse under the overlay." << std::endl);
		}

		HRESULT STDMETHODCALLTYPE HookGetDeviceState(IUnknown* self, DWORD size, LPVOID data)
		{
			const auto original = patcher.Original<GetDeviceStateFunction>(self, GET_DEVICE_STATE_SLOT);
			if (original == nullptr)
				return DIERR_GENERIC;   // only patched vtables point here

			const HRESULT result = original(self, size, data);

			const auto& capture = Framework::Input();
			if (FAILED(result) || !capture.IsMouseCaptured())
				return result;

			if (capture.FilterDeviceState(IsMouse(self), size, data))
				LogFirstBlock();

			return result;
		}

		HRESULT STDMETHODCALLTYPE HookGetDeviceData(IUnknown* self, DWORD size, LPDIDEVICEOBJECTDATA data, LPDWORD itemCount, DWORD flags)
		{
			const auto original = patcher.Original<GetDeviceDataFunction>(self, GET_DEVICE_DATA_SLOT);
			if (original == nullptr)
				return DIERR_GENERIC;

			const HRESULT result = original(self, size, data, itemCount, flags);

			// A null buffer is a peek/flush; leave it alone.
			const auto& capture = Framework::Input();
			if (FAILED(result) || data == nullptr || !capture.IsMouseCaptured())
				return result;

			static_assert(sizeof(DWORD) == sizeof(uint32_t));
			if (capture.FilterDeviceData(IsMouse(self), reinterpret_cast<uint32_t*>(itemCount)))
				LogFirstBlock();

			return result;
		}

		bool PatchCharacterSet(DirectInput8CreateFunction create, const GUID& iid)
		{
			IDirectInput8A* directInput = nullptr;   // A and W share the layout of every slot used here
			if (FAILED(create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, iid, reinterpret_cast<LPVOID*>(&directInput), nullptr)) || directInput == nullptr)
			{
				LOG_ERROR("[InputCapture] Could not create a DirectInput interface to find the mouse vtable." << std::endl);
				return false;
			}

			bool patched = false;
			IDirectInputDevice8A* device = nullptr;

			if (SUCCEEDED(directInput->CreateDevice(SYS_MOUSE_GUID, &device, nullptr)) && device != nullptr)
			{
				void** vtable = MemUtil::GetVTable(device);
				patched = patcher.Patch(vtable, GET_DEVICE_STATE_SLOT, reinterpret_cast<void*>(&HookGetDeviceState))
					&& patcher.Patch(vtable, GET_DEVICE_DATA_SLOT, reinterpret_cast<void*>(&HookGetDeviceData));

				if (!patched)
					LOG_ERROR("[InputCapture] Could not patch the DirectInput mouse vtable." << std::endl);

				device->Release();
			}
			else
			{
				LOG_ERROR("[InputCapture] Could not create a DirectInput mouse device to find its vtable." << std::endl);
			}

			directInput->Release();
			return patched;
		}
	}

	bool Install()
	{
		HMODULE dinput = GetModuleHandleW(L"dinput8.dll");
		if (dinput == nullptr)
			dinput = LoadLibraryW(L"dinput8.dll");

		const auto create = dinput ? reinterpret_cast<DirectInput8CreateFunction>(GetProcAddress(dinput, "DirectInput8Create")) : nullptr;
		if (create == nullptr)
		{
			LOG_ERROR("[InputCapture] dinput8.dll is unavailable; overlay clicks will reach the game." << std::endl);
			return false;
		}

		// Both, not ||: the game may use either character set.
		const bool ansi = PatchCharacterSet(create, DIRECT_INPUT8_A_GUID);
		const bool wide = PatchCharacterSet(create, DIRECT_INPUT8_W_GUID);
		if (!ansi && !wide)
			return false;

		LOG_INFO("[InputCapture] DirectInput mouse capture installed (A: " << ansi << ", W: " << wide << ")." << std::endl);
		return true;
	}

	void Shutdown()
	{
		Framework::Input().SetMouseCaptured(false);
		patcher.RestoreAll();
	}
}
