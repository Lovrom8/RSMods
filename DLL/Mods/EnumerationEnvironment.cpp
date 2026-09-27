#include "../stdafx.h"
#include "EnumerationEnvironment.hpp"
#include <algorithm>
#include <memory>
#include <thread>
#include <psapi.h>
#include <winioctl.h>
#include <wscapi.h>
#include <iwscapi.h>

namespace EnumerationEnvironment {
	std::string Narrow(const std::wstring& text) {
		if (text.empty())
			return {};
		const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		std::string out(size > 0 ? size : 0, '\0');
		if (size > 0)
			WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size, nullptr, nullptr);
		return out;
	}

	namespace {
		std::wstring Widen(std::string_view text) {
			if (text.empty())
				return {};
			const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
			std::wstring out(size > 0 ? size : 0, L'\0');
			if (size > 0)
				MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), size);
			return out;
		}

		// GetFinalPathNameByHandle, without the \\?\ prefix. Empty if the path can't be opened.
		std::wstring FinalPath(const std::wstring& path) {
			HANDLE probe = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
			if (probe == INVALID_HANDLE_VALUE)
				return {};
			wchar_t resolved[MAX_PATH * 2]{};
			const DWORD length = GetFinalPathNameByHandleW(probe, resolved, MAX_PATH * 2, VOLUME_NAME_DOS);
			CloseHandle(probe);
			if (length == 0 || length >= MAX_PATH * 2)
				return {};
			if (wcsncmp(resolved, L"\\\\?\\UNC\\", 8) == 0)
				return std::wstring(L"\\\\") + (resolved + 8);
			return wcsncmp(resolved, L"\\\\?\\", 4) == 0 ? std::wstring(resolved + 4) : std::wstring(resolved);
		}

		const char* BusName(STORAGE_BUS_TYPE bus) {
			switch (bus) {
			case BusTypeScsi: return "SCSI";
			case BusTypeAtapi: return "ATAPI";
			case BusTypeAta: return "ATA";
			case BusType1394: return "FireWire";
			case BusTypeSsa: return "SSA";
			case BusTypeFibre: return "Fibre Channel";
			case BusTypeUsb: return "USB";
			case BusTypeRAID: return "RAID";
			case BusTypeiScsi: return "iSCSI";
			case BusTypeSas: return "SAS";
			case BusTypeSata: return "SATA";
			case BusTypeSd: return "SD card";
			case BusTypeMmc: return "MMC";
			case BusTypeVirtual: return "virtual";
			case BusTypeFileBackedVirtual: return "file-backed virtual (VHD)";
			case BusTypeSpaces: return "Storage Spaces";
			case BusTypeNvme: return "NVMe";
			case BusTypeSCM: return "SCM";
			case BusTypeUfs: return "UFS";
			default: return "unknown";
			}
		}

		std::string Trimmed(const char* text) {
			std::string out(text);
			while (!out.empty() && out.back() == ' ')
				out.pop_back();
			const size_t first = out.find_first_not_of(' ');
			return first == std::string::npos ? std::string() : out.substr(first);
		}

		HANDLE OpenVolumeDevice(const std::wstring& mountPoint) {
			wchar_t guid[64]{};
			std::wstring device;
			if (GetVolumeNameForVolumeMountPointW(mountPoint.c_str(), guid, 64))
				device = guid;
			else
				device = std::wstring(L"\\\\.\\") + mountPoint;
			while (!device.empty() && device.back() == L'\\')
				device.pop_back();
			return CreateFileW(device.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		}

		std::string GameFolder() {
			wchar_t exe[MAX_PATH]{};
			GetModuleFileNameW(nullptr, exe, MAX_PATH);
			std::wstring folder(exe);
			const size_t slash = folder.find_last_of(L"\\/");
			return Narrow(slash == std::wstring::npos ? L"" : folder.substr(0, slash));
		}

		std::string WindowsVersion() {
			using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
			const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
			std::ostringstream out;
			const auto rtlGetVersion = ntdll ? reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion")) : nullptr;
			RTL_OSVERSIONINFOW info{ sizeof(info) };
			if (rtlGetVersion && rtlGetVersion(&info) == 0)
				out << "Windows " << info.dwMajorVersion << "." << info.dwMinorVersion << " build " << info.dwBuildNumber;
			else
				out << "Windows (version unknown)";
			using WineVersionFn = const char* (__cdecl*)();
			const auto wineVersion = ntdll ? reinterpret_cast<WineVersionFn>(GetProcAddress(ntdll, "wine_get_version")) : nullptr;
			if (wineVersion)
				out << ", running under Wine/Proton " << wineVersion();
			return out.str();
		}

		// Real-time scanners registered with Windows Security Center. Not available on Server SKUs or under Wine.
		std::vector<std::string> AntivirusProducts() {
			std::vector<std::string> products;
			const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			IWSCProductList* list = nullptr;
			HRESULT hr = CoCreateInstance(__uuidof(WSCProductList), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IWSCProductList), reinterpret_cast<void**>(&list));
			if (SUCCEEDED(hr) && list) {
				hr = list->Initialize(WSC_SECURITY_PROVIDER_ANTIVIRUS);
				LONG count = 0;
				if (SUCCEEDED(hr) && SUCCEEDED(list->get_Count(&count))) {
					for (LONG i = 0; i < count; ++i) {
						IWscProduct* product = nullptr;
						if (FAILED(list->get_Item(static_cast<ULONG>(i), &product)) || !product)
							continue;
						BSTR name = nullptr;
						WSC_SECURITY_PRODUCT_STATE state = WSC_SECURITY_PRODUCT_STATE_OFF;
						WSC_SECURITY_SIGNATURE_STATUS signatures = WSC_SECURITY_PRODUCT_OUT_OF_DATE;
						const bool haveName = SUCCEEDED(product->get_ProductName(&name)) && name;
						const bool haveState = SUCCEEDED(product->get_ProductState(&state));
						const bool haveSignatures = SUCCEEDED(product->get_SignatureStatus(&signatures));
						std::string line = haveName ? Narrow(name) : std::string("(unnamed)");
						const char* stateName = "state unknown";
						if (haveState) {
							switch (state) {
							case WSC_SECURITY_PRODUCT_STATE_ON: stateName = "real-time protection ON"; break;
							case WSC_SECURITY_PRODUCT_STATE_OFF: stateName = "off"; break;
							case WSC_SECURITY_PRODUCT_STATE_SNOOZED: stateName = "snoozed"; break;
							case WSC_SECURITY_PRODUCT_STATE_EXPIRED: stateName = "expired"; break;
							}
						}
						line += std::string(" (") + stateName;
						if (haveSignatures)
							line += signatures == WSC_SECURITY_PRODUCT_UP_TO_DATE ? ", signatures up to date" : ", signatures out of date";
						line += ")";
						products.push_back(std::move(line));
						if (name)
							SysFreeString(name);
						product->Release();
					}
					if (count == 0)
						products.push_back("none registered with Windows Security Center");
				}
				else {
					std::ostringstream out;
					out << "Windows Security Center query failed (0x" << std::hex << hr << ")";
					products.push_back(out.str());
				}
				list->Release();
			}
			else {
				std::ostringstream out;
				out << "Windows Security Center unavailable (0x" << std::hex << hr << ")";
				products.push_back(out.str());
			}
			if (SUCCEEDED(init))
				CoUninitialize();
			return products;
		}

		bool StartsWithInsensitive(const std::wstring& text, const std::wstring& prefix) {
			return !prefix.empty() && text.size() >= prefix.size() && _wcsnicmp(text.c_str(), prefix.c_str(), prefix.size()) == 0;
		}

		// Defender's path and process exclusions. Usually readable only by administrators, so "not readable" is common.
		// Returns a description of whether any of `paths` (or the game exe) is excluded.
		std::string DefenderExclusions(const std::vector<std::wstring>& paths) {
			const wchar_t* roots[] = {
				L"SOFTWARE\\Microsoft\\Windows Defender\\Exclusions",
				L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Exclusions",
			};
			bool anyReadable = false;
			bool denied = false;
			std::vector<std::string> matches;
			for (const wchar_t* root : roots) {
				for (const wchar_t* kind : { L"Paths", L"Processes" }) {
					HKEY key = nullptr;
					const std::wstring subkey = std::wstring(root) + L"\\" + kind;
					const LONG open = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &key);
					if (open == ERROR_ACCESS_DENIED) {
						denied = true;
						continue;
					}
					if (open != ERROR_SUCCESS)
						continue;
					anyReadable = true;
					for (DWORD i = 0;; ++i) {
						wchar_t name[1024]{};
						DWORD length = 1024;
						if (RegEnumValueW(key, i, name, &length, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
							break;
						std::wstring exclusion(name);
						while (!exclusion.empty() && (exclusion.back() == L'\\' || exclusion.back() == L'/'))
							exclusion.pop_back();
						if (wcscmp(kind, L"Processes") == 0) {
							if (_wcsicmp(exclusion.c_str(), L"Rocksmith2014.exe") == 0 || exclusion.find(L"Rocksmith2014.exe") != std::wstring::npos)
								matches.push_back("process " + Narrow(exclusion));
							continue;
						}
						for (const std::wstring& path : paths)
							if (StartsWithInsensitive(path, exclusion)) {
								matches.push_back("path " + Narrow(exclusion));
								break;
							}
					}
					RegCloseKey(key);
				}
			}
			if (!matches.empty()) {
				std::string out = "excluded (";
				for (size_t i = 0; i < matches.size(); ++i)
					out += (i ? ", " : "") + matches[i];
				return out + ")";
			}
			if (anyReadable)
				return "no exclusion covers the game or its dlc folder";
			return denied ? "exclusions not readable without admin rights" : "no exclusion list found";
		}

		struct LogJob {
			std::vector<std::string> samplePaths;
		};

		DWORD WINAPI LogMain(LPVOID param) {
			std::unique_ptr<LogJob> job(static_cast<LogJob*>(param));
			const long long startTicks = GetTickCount64();

			LOG_INFO("(ENUMERATION) Environment: " << WindowsVersion() << ", " << std::thread::hardware_concurrency() << " logical CPUs, " << MemoryLine() << std::endl);

			// The dlc folder itself: a link to another drive is common, and worth seeing at a glance.
			const std::string gameFolder = GameFolder();
			const std::wstring dlcFolder = Widen(gameFolder + "\\dlc");
			const DWORD attributes = GetFileAttributesW(dlcFolder.c_str());
			const std::wstring dlcFinal = FinalPath(dlcFolder);
			if (attributes == INVALID_FILE_ATTRIBUTES)
				LOG_INFO("(ENUMERATION) Environment: dlc folder " << Narrow(dlcFolder) << " not found (error " << GetLastError() << ")" << std::endl);
			else
				LOG_INFO("(ENUMERATION) Environment: dlc folder " << Narrow(dlcFolder) << ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) ? " is a link (symlink or junction)" : " is a plain folder")
					<< ", resolves to " << (dlcFinal.empty() ? std::string("(couldn't resolve)") : Narrow(dlcFinal)) << std::endl);

			// Every volume the sampled packages resolve to, with how many samples landed there and one example.
			struct Seen {
				std::wstring mountPoint;
				unsigned int samples = 0;
				std::wstring example;
			};
			std::vector<Seen> volumes;
			unsigned int unresolved = 0;
			std::vector<std::wstring> checkedPaths{ Widen(gameFolder), dlcFolder };
			if (!dlcFinal.empty())
				checkedPaths.push_back(dlcFinal);
			for (const std::string& path : job->samplePaths) {
				std::wstring finalPath;
				std::wstring mountPoint;
				if (!ResolveMountPoint(path, finalPath, mountPoint)) {
					++unresolved;
					continue;
				}
				checkedPaths.push_back(finalPath);
				auto known = std::find_if(volumes.begin(), volumes.end(), [&](const Seen& v) { return _wcsicmp(v.mountPoint.c_str(), mountPoint.c_str()) == 0; });
				if (known == volumes.end()) {
					volumes.push_back({ mountPoint, 1, finalPath });
				}
				else {
					++known->samples;
				}
			}
			LOG_INFO("(ENUMERATION) Environment: " << job->samplePaths.size() << " sampled packages are on " << volumes.size() << " volume(s)"
				<< (unresolved ? ", " + std::to_string(unresolved) + " couldn't be resolved" : std::string()) << std::endl);
			for (const Seen& seen : volumes) {
				const Volume volume = QueryVolume(seen.mountPoint);
				LOG_INFO("(ENUMERATION) Environment: volume " << Narrow(volume.mountPoint) << " (" << seen.samples << " of the samples, e.g. " << Narrow(seen.example) << "): "
					<< "drive type " << SeekPenaltyName(volume.seekPenalty) << ", bus " << volume.bus << (volume.removable ? " (removable)" : "")
					<< ", device \"" << volume.device << "\", file system " << (volume.fileSystem.empty() ? std::string("unknown") : Narrow(volume.fileSystem))
					<< ", " << (volume.freeBytes >> 30) << " GB free of " << (volume.totalBytes >> 30) << " GB" << std::endl);
			}

			for (const std::string& product : AntivirusProducts())
				LOG_INFO("(ENUMERATION) Environment: antivirus: " << product << std::endl);
			LOG_INFO("(ENUMERATION) Environment: Microsoft Defender exclusions: " << DefenderExclusions(checkedPaths) << std::endl);
			LOG_INFO("(ENUMERATION) Environment: probe took " << (GetTickCount64() - startTicks) << " ms" << std::endl);
			return 0;
		}
	}

	bool ResolveMountPoint(std::string_view path, std::wstring& finalPath, std::wstring& mountPoint) {
		const std::wstring wide = Widen(path);
		finalPath = FinalPath(wide);
		const std::wstring& target = finalPath.empty() ? wide : finalPath;
		if (finalPath.empty())
			finalPath = wide;
		wchar_t volume[MAX_PATH]{};
		if (!GetVolumePathNameW(target.c_str(), volume, MAX_PATH))
			return false;
		mountPoint = volume;
		return true;
	}

	Volume QueryVolume(const std::wstring& mountPoint) {
		Volume volume;
		volume.mountPoint = mountPoint;
		volume.bus = "unknown";

		wchar_t fileSystem[MAX_PATH + 1]{};
		if (GetVolumeInformationW(mountPoint.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fileSystem, MAX_PATH + 1))
			volume.fileSystem = fileSystem;
		ULARGE_INTEGER freeToCaller{}, total{}, totalFree{};
		if (GetDiskFreeSpaceExW(mountPoint.c_str(), &freeToCaller, &total, &totalFree)) {
			volume.freeBytes = freeToCaller.QuadPart;
			volume.totalBytes = total.QuadPart;
		}

		HANDLE device = OpenVolumeDevice(mountPoint);
		if (device == INVALID_HANDLE_VALUE)
			return volume;

		STORAGE_PROPERTY_QUERY seekQuery{ StorageDeviceSeekPenaltyProperty, PropertyStandardQuery };
		DEVICE_SEEK_PENALTY_DESCRIPTOR seek{};
		DWORD bytes = 0;
		if (DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY, &seekQuery, sizeof(seekQuery), &seek, sizeof(seek), &bytes, nullptr) && bytes >= sizeof(seek))
			volume.seekPenalty = seek.IncursSeekPenalty ? SeekPenalty::Yes : SeekPenalty::None;

		STORAGE_PROPERTY_QUERY deviceQuery{ StorageDeviceProperty, PropertyStandardQuery };
		alignas(8) unsigned char buffer[1024]{};
		bytes = 0;
		if (DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY, &deviceQuery, sizeof(deviceQuery), buffer, sizeof(buffer) - 1, &bytes, nullptr) && bytes >= sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
			const auto* descriptor = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(buffer);
			volume.bus = BusName(descriptor->BusType);
			volume.removable = descriptor->RemovableMedia != 0;
			std::string name;
			if (descriptor->VendorIdOffset && descriptor->VendorIdOffset < bytes)
				name = Trimmed(reinterpret_cast<const char*>(buffer + descriptor->VendorIdOffset));
			if (descriptor->ProductIdOffset && descriptor->ProductIdOffset < bytes) {
				const std::string product = Trimmed(reinterpret_cast<const char*>(buffer + descriptor->ProductIdOffset));
				name += (name.empty() || product.empty() ? "" : " ") + product;
			}
			volume.device = name;
		}
		CloseHandle(device);
		return volume;
	}

	const char* SeekPenaltyName(SeekPenalty penalty) {
		switch (penalty) {
		case SeekPenalty::None: return "SSD (no seek penalty)";
		case SeekPenalty::Yes: return "spinning disk (seek penalty)";
		default: return "unknown (the drive didn't say)";
		}
	}

	void LogAsync(std::vector<std::string> samplePaths) {
		auto* job = new LogJob{ std::move(samplePaths) };
		HANDLE thread = CreateThread(nullptr, 0, LogMain, job, 0, nullptr);
		if (!thread) {
			delete job;
			LOG_WARNING("(ENUMERATION) Environment: couldn't start the probe thread" << std::endl);
			return;
		}
		SetThreadPriority(thread, THREAD_PRIORITY_BELOW_NORMAL);
		CloseHandle(thread);
	}

	std::string MemoryLine() {
		std::ostringstream out;
		MEMORYSTATUSEX memory{ sizeof(memory) };
		if (GlobalMemoryStatusEx(&memory))
			out << "RAM " << (memory.ullAvailPhys >> 20) << " MB available of " << (memory.ullTotalPhys >> 20) << " MB";
		else
			out << "RAM unknown";
		PERFORMANCE_INFORMATION performance{ sizeof(performance) };
		if (GetPerformanceInfo(&performance, sizeof(performance)))
			out << ", file cache " << ((static_cast<unsigned long long>(performance.SystemCache) * performance.PageSize) >> 20) << " MB";
		return out.str();
	}
}
