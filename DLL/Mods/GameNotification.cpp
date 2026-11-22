#include "../stdafx.h"
#include "GameNotification.hpp"

#include <cstring>

namespace {
	struct NotificationTargets {
		uintptr_t showBannerRva;
		uintptr_t stringConstructorRva;
		uintptr_t stringDestructorRva;
		uintptr_t uiServiceRva;
		uintptr_t dismissByKeyRva;
		uintptr_t isNotificationActiveRva;
	};

	std::optional<NotificationTargets> GetTargets() {
		// Offsets are module-relative; any other executable gets no notifications.
		switch (GameVersion::Current()) {
		case Framework::GameVersion::Build::September2022:
			return NotificationTargets{ 0x003524A0, 0x000079D0, 0x0045C810, 0x00F5F62C, 0x00353930, 0x00354E90 };
		case Framework::GameVersion::Build::LearnAndPlayDec2024:
			return NotificationTargets{ 0x00352F80, 0x00007920, 0x00555910, 0x00F6062C, 0x00354410, 0x00355970 };
		default:
			return std::nullopt;
		}
	}

	bool MatchesBytes(uintptr_t address, const unsigned char* expected, size_t size) {
		__try {
			return std::memcmp(reinterpret_cast<const void*>(address), expected, size) == 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	bool ValidateTargets(const NotificationTargets& targets) {
		static constexpr unsigned char kShowPrologue[]{ 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38 };
		static constexpr unsigned char kConstructorPrologue[]{ 0x55, 0x8B, 0xEC, 0x8B, 0x4D, 0x08, 0x53, 0x56, 0x57 };
		static constexpr unsigned char kDestructorPrologue[]{
			0x8B, 0x41, 0x14, 0x8D, 0x51, 0x10, 0x3B, 0xC2, 0x74, 0x3E,
			0x56, 0x8B, 0x31, 0x85, 0xF6, 0x74, 0x36, 0x2B, 0xC6, 0x57
		};
		static constexpr unsigned char kDismissPrologue[]{
			0x56, 0x8D, 0x72, 0x10, 0x57, 0x8B, 0xC2, 0x39,
			0x72, 0x14, 0x74, 0x02, 0x8B, 0x02, 0x3B, 0x06
		};
		static constexpr unsigned char kIsActivePrologue[]{
			0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x10, 0x8B, 0x45, 0x08
		};
		const uintptr_t base = Offsets::baseHandle;
		return MatchesBytes(base + targets.showBannerRva, kShowPrologue, sizeof(kShowPrologue)) &&
			MatchesBytes(base + targets.stringConstructorRva, kConstructorPrologue, sizeof(kConstructorPrologue)) &&
			MatchesBytes(base + targets.stringDestructorRva, kDestructorPrologue, sizeof(kDestructorPrologue)) &&
			MatchesBytes(base + targets.dismissByKeyRva, kDismissPrologue, sizeof(kDismissPrologue)) &&
			MatchesBytes(base + targets.isNotificationActiveRva, kIsActivePrologue, sizeof(kIsActivePrologue));
	}

	struct StlString {
		unsigned char raw[0x18]{};
	};
	static_assert(sizeof(StlString) == 0x18);

	void InitEmpty(StlString& value) {
		std::memset(value.raw, 0, sizeof(value.raw));
		*reinterpret_cast<char**>(value.raw + 0x10) = reinterpret_cast<char*>(&value);
		*reinterpret_cast<char**>(value.raw + 0x14) = reinterpret_cast<char*>(&value) + 0x10;
	}

	bool BuildString(StlString& value, const char* text, uintptr_t constructor) {
		InitEmpty(value);
		const size_t length = text ? std::strlen(text) : 0;
		if (length == 0)
			return true;
		if (length <= 15) {
			std::memcpy(value.raw, text, length);
			*reinterpret_cast<char**>(value.raw + 0x10) = reinterpret_cast<char*>(&value) + length;
			return true;
		}

		const char* begin = text;
		const char* end = text + length;
		StlString* destination = &value;
		__try {
			__asm {
				mov eax, destination
				push end
				push begin
				call constructor
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			InitEmpty(value);
			return false;
		}
		return true;
	}

	void DestroyString(StlString& value, uintptr_t destructor) {
		if (*reinterpret_cast<char**>(value.raw + 0x14) == reinterpret_cast<char*>(&value) + 0x10)
			return;
		StlString* target = &value;
		__try {
			__asm {
				mov ecx, target
				call destructor
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
	}

	std::optional<std::string> CopyString(const StlString& value) {
		const char* self = reinterpret_cast<const char*>(&value);
		const char* end = *reinterpret_cast<char* const*>(value.raw + 0x10);
		const char* eos = *reinterpret_cast<char* const*>(value.raw + 0x14);
		const char* begin = eos == self + 0x10
			? self
			: *reinterpret_cast<char* const*>(value.raw);
		if (!begin || !end || end < begin || static_cast<size_t>(end - begin) > 1024)
			return std::nullopt;
		return std::string(begin, end);
	}

	bool UiReady(uintptr_t serviceAddress) {
		__try {
			void* service = *reinterpret_cast<void**>(serviceAddress);
			return service && !IsBadReadPtr(service, 0x18);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	void* GetNotificationManager(uintptr_t serviceAddress) {
		__try {
			void* service = *reinterpret_cast<void**>(serviceAddress);
			return service ? *reinterpret_cast<void**>(reinterpret_cast<char*>(service) + 0x14) : nullptr;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}

	bool CallBanner(uintptr_t function, int ecxValue, int messageType, StlString* output,
		int playerIndex, StlString* title, StlString* body, unsigned durationBits) {
		bool succeeded = true;
		__try {
			__asm {
				sub esp, 0x18
				xor eax, eax
				mov dword ptr [esp + 0x00], eax
				mov dword ptr [esp + 0x04], eax
				mov dword ptr [esp + 0x08], eax
				mov dword ptr [esp + 0x0C], eax
				lea eax, [esp]
				mov dword ptr [esp + 0x10], eax
				add eax, 0x10
				mov dword ptr [esp + 0x14], eax

				push 0
				push 0
				push durationBits
				push body
				push title
				push playerIndex

				mov ecx, ecxValue
				mov edx, messageType
				mov esi, output
				call function
				add esp, 0x30
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			succeeded = false;
		}
		return succeeded;
	}

	bool CallDismissByKey(uintptr_t function, StlString* instanceKey) {
		bool succeeded = true;
		__try {
			__asm {
				mov edx, instanceKey
				call function
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			succeeded = false;
		}
		return succeeded;
	}

	std::optional<bool> CallIsActive(uintptr_t function, uintptr_t constructor,
		void* manager, const char* begin, const char* end) {
		bool callSucceeded = true;
		bool active = false;
		__try {
			__asm {
				sub esp, 0x18
				mov eax, esp
				lea ecx, [eax + 0x10]
				mov dword ptr [ecx], eax
				mov dword ptr [eax + 0x14], ecx
				push end
				push begin
				call constructor
				push manager
				call function
				mov active, al
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			callSucceeded = false;
		}
		if (!callSucceeded)
			return std::nullopt;
		return active;
	}
}

std::optional<GameNotification::Handle> GameNotification::Show(const char* title, const char* body,
	float seconds, int messageType, int playerIndex) {
	if (!(seconds > 0.0f) || messageType < 1 || messageType > 24)
		return std::nullopt;

	const auto targets = GetTargets();
	if (!targets || !ValidateTargets(*targets)) {
		LOG_ERROR("(NOTIFY) Unsupported executable or notification targets did not validate" << std::endl);
		return std::nullopt;
	}

	const uintptr_t base = Offsets::baseHandle;
	if (!UiReady(base + targets->uiServiceRva))
		return std::nullopt;

	StlString output{};
	StlString titleString{};
	StlString bodyString{};
	InitEmpty(output);
	if (!BuildString(titleString, title, base + targets->stringConstructorRva) ||
		!BuildString(bodyString, body, base + targets->stringConstructorRva)) {
		DestroyString(titleString, base + targets->stringDestructorRva);
		DestroyString(bodyString, base + targets->stringDestructorRva);
		return std::nullopt;
	}

	unsigned durationBits{};
	static_assert(sizeof(durationBits) == sizeof(seconds));
	std::memcpy(&durationBits, &seconds, sizeof(durationBits));
	const int ecxValue = messageType == 0x16 ? 1 : 0;
	const bool sent = CallBanner(base + targets->showBannerRva, ecxValue, messageType,
		&output, playerIndex, &titleString, &bodyString, durationBits);
	const auto instanceKey = sent ? CopyString(output) : std::nullopt;

	DestroyString(titleString, base + targets->stringDestructorRva);
	DestroyString(bodyString, base + targets->stringDestructorRva);
	DestroyString(output, base + targets->stringDestructorRva);

	if (!instanceKey || instanceKey->empty())
		return std::nullopt;
	return Handle{ *instanceKey, messageType };
}

bool GameNotification::Dismiss(const Handle& handle) {
	if (!handle)
		return false;

	const auto targets = GetTargets();
	if (!targets || !ValidateTargets(*targets))
		return false;

	const uintptr_t base = Offsets::baseHandle;
	if (!UiReady(base + targets->uiServiceRva))
		return false;

	StlString instanceKey{};
	if (!BuildString(instanceKey, handle.instanceKey.c_str(), base + targets->stringConstructorRva))
		return false;
	const bool dismissed = CallDismissByKey(base + targets->dismissByKeyRva, &instanceKey);
	DestroyString(instanceKey, base + targets->stringDestructorRva);
	return dismissed;
}

std::optional<bool> GameNotification::IsActive(const Handle& handle) {
	if (!handle)
		return std::nullopt;

	const auto targets = GetTargets();
	if (!targets || !ValidateTargets(*targets))
		return std::nullopt;

	const uintptr_t base = Offsets::baseHandle;
	void* manager = GetNotificationManager(base + targets->uiServiceRva);
	if (!manager)
		return std::nullopt;

	return CallIsActive(base + targets->isNotificationActiveRva,
		base + targets->stringConstructorRva,
		manager,
		handle.instanceKey.data(),
		handle.instanceKey.data() + handle.instanceKey.size());
}
