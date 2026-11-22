#include "../stdafx.h"
#include "UpdateNoticeMod.hpp"

#include <winhttp.h>

#include "../Version.h"
#include "../Framework/UpdateNotice.hpp"
#include "CacheStore.hpp"
#include "EnumerationDrain.hpp"
#include "GameNotification.hpp"

#pragma comment(lib, "winhttp.lib")

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
using namespace std::chrono_literals;
namespace Notice = Framework::UpdateNotice;

namespace {
	constexpr char kEnabled[] = "UpdateNoticeEnabled";

	constexpr wchar_t kRsModsPath[] = L"/repos/Lovrom8/RSMods/releases/latest";
	constexpr wchar_t kRsAsioPath[] = L"/repos/mdias/rs_asio/releases/latest";

	// RS_ASIO is loaded through avrt.dll about 7 s after launch; leave margin before asking it.
	constexpr auto kRsAsioReadyAfter = 12s;

	// Type 0x11, the upper-right news panel: a header and a body that do not block the menu.
	constexpr int kMessageType = 0x11;
	constexpr char kMessageTitle[] = "Update available";
	// Long enough to read two lines with two components and find the releases page.
	constexpr float kMessageSeconds = 20.0f;

	// The menu becomes ready while the title screen is still fading out; wait so the notice lands
	// when the player is looking at the menu.
	constexpr auto kShowDelayAfterMenuReady = 3s;

	// Show fails while the UI service is not up yet; try on a few menu ticks, then give up for this launch.
	constexpr int kMaxShowAttempts = 40;

	constexpr std::size_t kMaxResponseBytes = 256 * 1024;

	// Seconds since this process started, so the RS_ASIO wait counts from launch rather than from the menu.
	std::chrono::milliseconds ProcessAge() {
		FILETIME created{}, exited{}, kernel{}, user{};
		if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
			return 0ms;
		FILETIME now{};
		GetSystemTimeAsFileTime(&now);
		ULARGE_INTEGER start{}, end{};
		start.LowPart = created.dwLowDateTime;
		start.HighPart = created.dwHighDateTime;
		end.LowPart = now.dwLowDateTime;
		end.HighPart = now.dwHighDateTime;
		if (end.QuadPart < start.QuadPart)
			return 0ms;
		return std::chrono::milliseconds((end.QuadPart - start.QuadPart) / 10000);
	}

	// Null when RS_ASIO is absent or predates the export.
	std::optional<Notice::Version> LocalRsAsioVersion() {
		const HMODULE module = GetModuleHandleA("RS_ASIO.dll");
		if (!module)
			return std::nullopt;
		using GetVersionFn = const char*(__cdecl*)();
		const auto getVersion = reinterpret_cast<GetVersionFn>(GetProcAddress(module, "RSASIO_GetVersion"));
		if (!getVersion)
			return std::nullopt;
		const char* text = getVersion();
		return text ? Notice::ParseVersion(text) : std::nullopt;
	}

	uint64_t NowSeconds() {
		return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	}

	// Nothing for a missing, damaged or other-version section.
	std::optional<Notice::CheckRecord> LoadRecord() {
		std::vector<unsigned char> bytes;
		if (!RSModsCache::Shared().Take(Framework::CacheFile::kUpdateCheck, bytes))
			return std::nullopt;
		return Notice::DecodeRecord(bytes);
	}

	// Per install and kept in the record, so one install keeps its slot from month to month.
	uint32_t NewJitter() {
		const uint64_t mixed = GetTickCount64() * 0x9E3779B97F4A7C15ull ^ (static_cast<uint64_t>(GetCurrentProcessId()) << 20) ^ NowSeconds();
		return Notice::JitterFromSeed(static_cast<uint32_t>(mixed ^ (mixed >> 32)));
	}
}

SettingDefs UpdateNoticeMod::Settings() const {
	return {
		SettingDef::Toggle(kEnabled, "Update Notice")
			.Default("on")
			.Heading(Framework::SettingHeading::GameAndWindow)
			.Hint("Shows a notice in the main menu if a newer RSMods or RS_ASIO release is on GitHub.\n"
				"Nothing is updated for you: the notice names what is out of date and where to download it.\n"
				"Checks the GitHub release pages in the background about once a month and stays silent if it cannot reach them.")
	};
}

bool UpdateNoticeMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(kEnabled);
}

UpdateNoticeMod::~UpdateNoticeMod() {
	// A std::thread destroyed while joinable calls std::terminate.
	Stop();
}

void UpdateNoticeMod::OnMenuTick(ModContext&) {
	if (shown_)
		return;
	if (!started_) {
		started_ = true;
		const auto stored = LoadRecord();
		const uint64_t now = NowSeconds();
		if (stored && Notice::IsFresh(*stored, now)) {
			LOG_INFO("(UPDATE NOTICE) using the check from " << Notice::AgeDays(*stored, now) << " day(s) ago" << std::endl);
			Publish(stored->modsTag, stored->asioTag);
		}
		else {
			worker_ = std::thread([this] { Worker(); });
		}
		return;
	}

	std::string text;
	{
		std::lock_guard lock(mutex_);
		if (!message_)
			return;
		text = *message_;
	}

	// The 12 s lifetime would run out behind the loading transition, so wait for the menu.
	if (EnumerationDrain::MainMenuReadySeconds() <= 0.0)
		return;
	const auto now = std::chrono::steady_clock::now();
	if (!menuReadyAt_)
		menuReadyAt_ = now;
	if (now - *menuReadyAt_ < kShowDelayAfterMenuReady)
		return;

	if (GameNotification::Show(kMessageTitle, text.c_str(), kMessageSeconds, kMessageType)) {
		shown_ = true;
		LOG_INFO("(UPDATE NOTICE) shown: " << kMessageTitle << ": " << text << std::endl);
	}
	else if (++attempts_ >= kMaxShowAttempts) {
		shown_ = true;
		LOG_INFO("(UPDATE NOTICE) could not show the notice after " << attempts_ << " attempts" << std::endl);
	}
}

void UpdateNoticeMod::OnShutdown(ModContext&) {
	Stop();
}

void UpdateNoticeMod::Stop() {
	{
		std::lock_guard lock(mutex_);
		stop_ = true;
		// Closing the request fails the call in flight, so the join does not wait out a timeout.
		if (request_) {
			WinHttpCloseHandle(request_);
			request_ = nullptr;
		}
	}
	wake_.notify_all();
	if (worker_.joinable())
		worker_.join();
}

// Returns false once shutdown was requested.
bool UpdateNoticeMod::WaitFor(std::chrono::milliseconds time) {
	std::unique_lock lock(mutex_);
	return !wake_.wait_for(lock, time, [this] { return stop_; });
}

void UpdateNoticeMod::Worker() {
	// Nothing may escape the thread: an exception here would end the process.
	try {
		// Fail-silent: the fetch returns nothing on any network, HTTP or parse problem.
		const auto stored = LoadRecord();
		const auto localMods = Notice::ParseVersion(RSMODS_VERSION_STRING);

		std::optional<std::string> modsTag;
		if (localMods)
			modsTag = FetchLatestTag(kRsModsPath);

		const auto age = ProcessAge();
		if (age < kRsAsioReadyAfter && !WaitFor(kRsAsioReadyAfter - age))
			return;

		std::optional<std::string> asioTag;
		if (LocalRsAsioVersion())
			asioTag = FetchLatestTag(kRsAsioPath);

		// A failed fetch keeps what was stored for that component.
		Notice::CheckRecord record;
		record.jitter = stored ? stored->jitter : NewJitter();
		record.modsTag = modsTag ? *modsTag : (stored ? stored->modsTag : std::string());
		record.asioTag = asioTag ? *asioTag : (stored ? stored->asioTag : std::string());

		// Only a fetched RSMods tag counts as a check; otherwise the next launch tries again.
		if (modsTag) {
			record.checkedAt = NowSeconds();
			DWORD error = 0;
			if (!RSModsCache::Shared().Save(Framework::CacheFile::kUpdateCheck, Notice::EncodeRecord(record), error))
				LOG_INFO("(UPDATE NOTICE) could not store the check, error " << error << std::endl);
		}
		LOG_INFO("(UPDATE NOTICE) checked GitHub" << (modsTag ? "" : ", RSMods release not reachable") << std::endl);
		Publish(record.modsTag, record.asioTag);
	}
	catch (const std::exception& e) {
		LOG_INFO("(UPDATE NOTICE) check failed: " << e.what() << std::endl);
	}
	catch (...) {
		LOG_INFO("(UPDATE NOTICE) check failed: unknown exception" << std::endl);
	}
}

void UpdateNoticeMod::Publish(const std::string& rsModsTag, const std::string& rsAsioTag) {
	const auto updates = Notice::FindUpdates(Notice::ParseVersion(RSMODS_VERSION_STRING), rsModsTag,
		LocalRsAsioVersion(), rsAsioTag);
	const std::string text = Notice::BuildBody(updates);
	LOG_INFO("(UPDATE NOTICE) " << updates.size() << " update(s) found" << std::endl);
	if (text.empty())
		return;

	std::lock_guard lock(mutex_);
	message_ = text;
}

std::optional<std::string> UpdateNoticeMod::FetchLatestTag(const wchar_t* path) {
	struct Closer {
		HINTERNET handle;
		~Closer() { if (handle) WinHttpCloseHandle(handle); }
	};

	// GitHub rejects requests without a User-Agent.
	Closer session{ WinHttpOpen(L"RSMods-UpdateNotice", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0) };
	if (!session.handle)
		return std::nullopt;

	// Windows 7 does not offer TLS 1.2 to WinHTTP by default; GitHub requires it.
	DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
	WinHttpSetOption(session.handle, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
	if (!WinHttpSetTimeouts(session.handle, 3000, 3000, 3000, 5000))
		return std::nullopt;

	Closer connection{ WinHttpConnect(session.handle, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0) };
	if (!connection.handle)
		return std::nullopt;
	HINTERNET request = WinHttpOpenRequest(connection.handle, L"GET", path, nullptr, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if (!request)
		return std::nullopt;
	{
		std::lock_guard lock(mutex_);
		if (stop_) {
			WinHttpCloseHandle(request);
			return std::nullopt;
		}
		request_ = request;
	}

	// OnShutdown may have closed the request already, in which case the slot is empty and the calls below fail.
	struct RequestRelease {
		UpdateNoticeMod& self;
		HINTERNET request;
		~RequestRelease() {
			std::lock_guard lock(self.mutex_);
			if (self.request_ == request) {
				WinHttpCloseHandle(request);
				self.request_ = nullptr;
			}
		}
	} release{ *this, request };

	static constexpr wchar_t kHeaders[] = L"Accept: application/vnd.github+json\r\n";
	if (!WinHttpSendRequest(request, kHeaders, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
		!WinHttpReceiveResponse(request, nullptr))
		return std::nullopt;

	DWORD status = 0;
	DWORD statusSize = sizeof(status);
	if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX) || status != 200) {
		LOG_INFO("(UPDATE NOTICE) release query returned HTTP " << status << std::endl);
		return std::nullopt;
	}

	std::string body;
	for (;;) {
		DWORD available = 0;
		if (!WinHttpQueryDataAvailable(request, &available) || available == 0)
			break;
		const std::size_t offset = body.size();
		if (available > kMaxResponseBytes - offset)
			return std::nullopt;
		body.resize(offset + available);
		DWORD read = 0;
		if (!WinHttpReadData(request, body.data() + offset, available, &read))
			return std::nullopt;
		body.resize(offset + read);
		if (read == 0)
			break;
	}

	return Notice::ExtractJsonString(body, "tag_name");
}

static Framework::ModRegistrar<UpdateNoticeMod> _updateNoticeReg;
