#include "../stdafx.h"
#include "UplayLoginMod.hpp"
#include "../GameState.hpp"
#include "../SettingEdits.hpp"
#include <atomic>

using Framework::Enum;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

namespace {
	// Login dialog slots: +0x18 Tick, +0x110 cancel; cancel pops the dialog stage.
	constexpr uintptr_t dialogVtable = 0x011C2F00;
	constexpr uintptr_t dialogTickSlot = dialogVtable + 0x18;
	constexpr uintptr_t dialogTick = 0x0074D250;
	constexpr uintptr_t dialogCancelSlot = dialogVtable + 0x110;
	constexpr uintptr_t dialogCancel = 0x006A9080;
	constexpr uintptr_t transitionBeginSlot = dialogVtable + 0x6C;
	constexpr uintptr_t transitionBegin = 0x006A85B0;
	constexpr unsigned char transitionBeginBytes[] = {
		0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08, 0x8D, 0x45,
		0xF8, 0x50, 0x81, 0xC1, 0xC0, 0x02, 0x00, 0x00
	};

	// Input-ready state is 0x12; action 3 awaits confirmation, listener +0x23 records cancellation.
	constexpr uintptr_t off_inputEnabled = 0x0C;
	constexpr uintptr_t off_state = 0x10C;
	constexpr int stateInputReady = 0x12;
	constexpr uintptr_t off_pendingAction = 0x2FC;
	constexpr int actionSkipConfirm = 3;
	constexpr uintptr_t off_confirmListener = 0x318;
	constexpr uintptr_t off_listenerCancelReceived = 0x23;

	// Native skip cancels the dialog, then invokes profile-service slot +0x64 with (0, 0).
	// Connect service in ECX; plain ret.
	uintptr_t connectSingleton = 0;
	uintptr_t connectVtable = 0;
	constexpr uintptr_t connectSkip = 0x008B8E10;
	constexpr unsigned char connectSkipBytes[] = { 0x8B, 0x01, 0x8B, 0x90, 0x00, 0x02, 0x00, 0x00, 0xFF, 0xD2 };

	constexpr const char* settingAsk = "ask";
	constexpr const char* settingSkip = "skip";

	using TickFn = int(__thiscall*)(void* dialog);
	using CancelFn = void(__thiscall*)(void* dialog);
	using SkipFn = void(__thiscall*)(void* connect);
	using TransitionBeginFn = void(__thiscall*)(void* dialog, void* args);
	uintptr_t transitionBeginTrampoline = 0;

	// Re-arm once the dialog transitions in for a new sign-in pass.
	bool skipped = false;
	std::atomic<bool> skippedThisSession{ false };
	constexpr uintptr_t downloadLatestMetadata = 0x008C4AB0;
	constexpr unsigned char downloadEntryBytes[] = { 0x51, 0x83, 0x7E, 0x04, 0x03 };
	constexpr uintptr_t downloadNoop = 0x008C4B0C;
	constexpr unsigned char downloadNoopBytes[] = { 0xC6, 0x44, 0x37, 0x0A, 0x01, 0x59, 0xC3 };
	uintptr_t downloadTrampoline = 0;
	uintptr_t downloadNoopJump = downloadNoop;

	bool __cdecl SkipMetadataForSession() {
		if (!skippedThisSession.load(std::memory_order_acquire)) {
			return false;
		}
		return true;
	}

	// Metadata entry ABI: coordinator ESI, index EDI, no stack arguments.
	// The no-download branch completes the index and restores ECX.
	__declspec(naked) void DownloadLatestMetadataHook() {
		__asm {
			push ecx
			call SkipMetadataForSession
			test al, al
			jnz noDownload
			pop ecx
			jmp dword ptr [downloadTrampoline]
		noDownload:
			jmp dword ptr [downloadNoopJump]
		}
	}

	template <typename T>
	bool Read(uintptr_t address, T& value) {
		if (!address || MemUtil::IsBadReadPtr(reinterpret_cast<void*>(address)))
			return false;
		value = *reinterpret_cast<const T*>(address);
		return true;
	}

	bool SkipUplay() {
		return Settings::ReturnSettingValue(Setting::UplayLogin) != settingAsk;
	}

	// Override only RendezVous.SkipLogin when skipping; other bool queries remain native.
	// Getter ABI: (const string* key, bool default), ret 8.
	uintptr_t configBoolSlot = 0;
	uintptr_t configBool = 0;
	constexpr char skipLoginKey[] = "RendezVous.SkipLogin";
	using ConfigBoolFn = bool(__thiscall*)(void* gameInterface, const void* key, bool fallback);

	// STLport string: data +0, end +0x10, storage end +0x14; inline storage ends at +0x10.
	bool KeyIs(const void* key, const char* expected, size_t length) {
		const uintptr_t self = reinterpret_cast<uintptr_t>(key);
		uintptr_t heap = 0, finish = 0, storageEnd = 0;
		if (!Read(self, heap) || !Read(self + 0x10, finish) || !Read(self + 0x14, storageEnd))
			return false;
		const uintptr_t start = storageEnd == self + 0x10 ? self : heap;
		if (finish < start || finish - start != length || MemUtil::IsBadReadPtr(reinterpret_cast<void*>(start)))
			return false;
		return memcmp(reinterpret_cast<const void*>(start), expected, length) == 0;
	}

	bool __fastcall ConfigBoolHook(void* gameInterface, void*, const void* key, bool fallback) {
		if (KeyIs(key, skipLoginKey, sizeof(skipLoginKey) - 1)) {
			const bool skip = SkipUplay();
			if (!skip) {
				const bool nativeSkip = reinterpret_cast<ConfigBoolFn>(configBool)(gameInterface, key, fallback);
				skippedThisSession.store(nativeSkip, std::memory_order_release);
				return nativeSkip;
			}
			skippedThisSession.store(true, std::memory_order_release);
			static bool logged = false;
			if (!logged) {
				logged = true;
				LOG_INFO("(UPLAY) RendezVous.SkipLogin answered true (Uplay skipped)" << std::endl);
			}
			return true;
		}
		return reinterpret_cast<ConfigBoolFn>(configBool)(gameInterface, key, fallback);
	}

	// Cancel ConnectingToRDV only during saved-credentials login; dialog id is at +0x2C0.
	constexpr uintptr_t listVtable = 0x011BBF88;
	constexpr uintptr_t listTickSlot = listVtable + 0x18;
	constexpr uintptr_t listTick = 0x005DB180;
	constexpr uintptr_t listCancelSlot = listVtable + 0x110;
	constexpr uintptr_t listTransitionBeginSlot = listVtable + 0x6C;
	constexpr uintptr_t off_dialogId = 0x2C0;
	constexpr char connectingId[] = "ConnectingToRDV";
	constexpr char loadingProfileId[] = "RS2LoadingProfile";
	constexpr uintptr_t hideDialog = 0x005A32C0;
	constexpr unsigned char hideDialogBytes[] = {
		0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x08, 0x8B, 0x0D, 0x2C,
		0xF6, 0x35, 0x01, 0x8B, 0x51, 0x10, 0x50, 0x52, 0xE8,
		0xFA, 0xA1, 0x03, 0x00, 0x5D, 0xC2, 0x04, 0x00
	};
	constexpr uintptr_t uiDialogManagerHide = 0x005DD4D0;
	constexpr unsigned char uiDialogManagerHideBytes[] = {
		0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC,
		0x24, 0xA1, 0x40, 0xB2, 0x25, 0x01, 0x33, 0xC4
	};
	using HideDialogFn = void(__stdcall*)(const void* id);
	bool hideLoadingDialogReady = false;

	// Connect +0xC holds the profile service; slot +0x64 skips login, +0xCC holds substate.
	// Substate 3 denotes saved-credentials login.
	constexpr uintptr_t off_profileService = 0x0C;
	constexpr uintptr_t win32SkipSlot = 0x64;
	constexpr uintptr_t win32Skip = 0x008CB580;
	constexpr uintptr_t off_win32Substate = 0xCC;
	constexpr int substateSavedLogin = 3;

	// Re-arm once the dialog transitions in for a new sign-in pass.
	bool connectingCancelled = false;

	bool SavedLoginPending() {
		uintptr_t connect = 0, connectTable = 0, service = 0, serviceTable = 0, skip = 0;
		int substate = 0;
		return Read(connectSingleton, connect) && Read(connect, connectTable) && connectTable == connectVtable
			&& Read(connect + off_profileService, service) && Read(service, serviceTable)
			&& Read(serviceTable + win32SkipSlot, skip) && skip == win32Skip
			&& Read(service + off_win32Substate, substate) && substate == substateSavedLogin;
	}

	using PlatformTickFn = uintptr_t(__thiscall*)(void*);
	using FlowUpdateFn = void(__thiscall*)(void*);
	using ProfileLoadedFn = void(__thiscall*)(void*, uintptr_t);
	PlatformTickFn sourceTick = nullptr;
	FlowUpdateFn sourceFlow = nullptr;
	ProfileLoadedFn sourceLoaded = nullptr;
	ULONGLONG sourceSignInStarted = 0;

	bool SourceSkipEnabled() {
		return SkipUplay();
	}

	uintptr_t __fastcall SourceTick(void* service, void*) {
		const uintptr_t self = reinterpret_cast<uintptr_t>(service);
		uint32_t state = 0, platformState = 0;
		uintptr_t listener = 0;
		// Restrict source bypass to boot sign-in; later profile flows retain native dialogs.
		if (SourceSkipEnabled() && !GameState::BootComplete
			&& Read(self + 0x2C, state) && state == 1
			&& Read(self + off_win32Substate, platformState) && platformState == 0
			&& Read(self + 0x40, listener) && listener == 0) {
			// Set native skip substate 4 before substate 0 can read credentials or create dialogs.
			*reinterpret_cast<uint32_t*>(self + off_win32Substate) = 4;
			skippedThisSession.store(true, std::memory_order_release);
			sourceSignInStarted = GetTickCount64();
			LOG_INFO("(UPLAY) Source sign-in bypassed saved credentials and login dialogs" << std::endl);
		}
		return sourceTick(service);
	}

	void __fastcall SourceFlow(void* service, void*) {
		const uintptr_t self = reinterpret_cast<uintptr_t>(service);
		uint32_t state = 0;
		unsigned char shown = 0;
		// Bypass state 18 only inside boot sign-in; other flows require its panel.
		if (SourceSkipEnabled() && sourceSignInStarted != 0 && Read(self + 0x2C, state) && state == 18
			&& Read(self + 0x70, shown) && shown == 0) {
			// State 19 starts native profile loading; state 18 only displays its waiting panel.
			*reinterpret_cast<uint32_t*>(self + 0x2C) = 19;
			LOG_INFO("(UPLAY) Source profile load bypassed loading-dialog creation and display timers" << std::endl);
		}
		sourceFlow(service);
	}

	void __fastcall SourceLoaded(void* gameInterface, void*, uintptr_t profile) {
		if (sourceSignInStarted != 0) {
			LOG_INFO("(UPLAY) Profile loaded after source sign-in " << GetTickCount64() - sourceSignInStarted << " ms" << std::endl);
			sourceSignInStarted = 0;
		}
		sourceLoaded(gameInterface, profile);
	}

	void InstallSourceFlow() {
		const uintptr_t slots[] = { Offsets::startupProfileTickSlot, Offsets::startupProfileFlowSlot, Offsets::startupProfileLoadedSlot };
		const uintptr_t originals[] = { Offsets::startupProfileTick, Offsets::startupProfileFlow, Offsets::startupProfileLoaded };
		const unsigned char tickBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8 };
		const unsigned char flowBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xC0 };
		const unsigned char loadedBody[] = { 0x53, 0x8B, 0x5D, 0x08, 0x56, 0x57, 0x53 };
		for (size_t i = 0; i < std::size(slots); ++i) {
			uintptr_t value = 0;
			if (!slots[i] || !originals[i] || !Read(slots[i], value) || value != originals[i]) {
				LOG_WARNING("(UPLAY) Source sign-in hooks not installed: vtable mismatch" << std::endl);
				return;
			}
		}
		if (!MemUtil::MatchesBytes(originals[0], tickBytes) || !MemUtil::MatchesBytes(originals[1], flowBytes)
			|| !MemUtil::MatchesBytes(originals[2] + 0x10, loadedBody)) {
			LOG_WARNING("(UPLAY) Source sign-in hooks not installed: native byte mismatch" << std::endl);
			return;
		}
		sourceTick = reinterpret_cast<PlatformTickFn>(originals[0]);
		sourceFlow = reinterpret_cast<FlowUpdateFn>(originals[1]);
		sourceLoaded = reinterpret_cast<ProfileLoadedFn>(originals[2]);
		const uintptr_t hooks[] = { reinterpret_cast<uintptr_t>(&SourceTick), reinterpret_cast<uintptr_t>(&SourceFlow), reinterpret_cast<uintptr_t>(&SourceLoaded) };
		for (size_t i = 0; i < std::size(slots); ++i) {
			LOG_INFO("(UPLAY) Source hook original 0x" << std::hex << slots[i] << " = 0x" << originals[i] << std::dec << std::endl);
			if (MemUtil::PatchAdr(reinterpret_cast<void*>(slots[i]), &hooks[i], sizeof(hooks[i]))) continue;
			for (size_t n = 0; n <= i; ++n)
				if (!MemUtil::PatchAdr(reinterpret_cast<void*>(slots[n]), &originals[n], sizeof(originals[n])))
					LOG_ERROR("(UPLAY) Source hook rollback failed" << std::endl);
			return;
		}
		LOG_INFO("(UPLAY) Source sign-in hooks installed; Uplay Login set to Ask keeps the dialog path" << std::endl);
	}

	void TryCancelConnecting(uintptr_t dialog) {
		uintptr_t vtable = 0;
		int state = 0;
		unsigned char input = 0;
		if (!Read(dialog, vtable) || vtable != listVtable
			|| !Read(dialog + off_state, state) || state != stateInputReady
			|| !Read(dialog + off_inputEnabled, input) || input == 0
			|| !KeyIs(reinterpret_cast<const void*>(dialog + off_dialogId), connectingId, sizeof(connectingId) - 1)
			|| !SavedLoginPending())
			return;

		connectingCancelled = true;
		// Cancellation pops the dialog; do not access it afterward.
		reinterpret_cast<CancelFn>(dialogCancel)(reinterpret_cast<void*>(dialog));
		LOG_INFO("(UPLAY) Cancelled the saved-credentials login, as the player skips Uplay" << std::endl);
	}

	int __fastcall ListTick(void* dialog, void*) {
		const int result = reinterpret_cast<TickFn>(listTick)(dialog);
		if (!connectingCancelled && SkipUplay())
			TryCancelConnecting(reinterpret_cast<uintptr_t>(dialog));
		return result;
	}

	// Apply saved choices before rendering the first dialog frame.
	void __fastcall TransitionInBegin(void* dialog, void*, void* args) {
		const uintptr_t self = reinterpret_cast<uintptr_t>(dialog);
		uintptr_t vtable = 0;
		if (Read(self, vtable)) {
			// Hide this dialog id before transition because state 18 does not consume its listener.
			if (hideLoadingDialogReady && vtable == listVtable
				&& KeyIs(reinterpret_cast<const void*>(self + off_dialogId), loadingProfileId,
					sizeof(loadingProfileId) - 1)) {
				reinterpret_cast<HideDialogFn>(hideDialog)(reinterpret_cast<const void*>(self + off_dialogId));
				LOG_INFO("(UPLAY) Hid the profile-loading dialog without resolving a listener" << std::endl);
				return;
			}
			// Each dialog appearance starts a new sign-in pass; re-arm skips.
			if (vtable == dialogVtable) {
				skipped = false;
				if (!SkipUplay())
					skippedThisSession.store(false, std::memory_order_release);
			} else if (vtable == listVtable
				&& KeyIs(reinterpret_cast<const void*>(self + off_dialogId), connectingId, sizeof(connectingId) - 1))
				connectingCancelled = false;
		}
		if (SkipUplay() && Read(self, vtable)) {
			if (vtable == listVtable && !connectingCancelled
				&& KeyIs(reinterpret_cast<const void*>(self + off_dialogId), connectingId, sizeof(connectingId) - 1)
				&& SavedLoginPending()) {
				connectingCancelled = true;
				reinterpret_cast<CancelFn>(dialogCancel)(dialog);
				LOG_INFO("(UPLAY) Cancelled saved-credential login before its dialog appeared" << std::endl);
				return;
			}
			if (vtable == dialogVtable && !skipped) {
				uintptr_t connect = 0, connectTable = 0;
				if (Read(connectSingleton, connect) && Read(connect, connectTable) && connectTable == connectVtable) {
					skipped = true;
					reinterpret_cast<CancelFn>(dialogCancel)(dialog);
					reinterpret_cast<SkipFn>(connectSkip)(reinterpret_cast<void*>(connect));
					skippedThisSession.store(true, std::memory_order_release);
					LOG_INFO("(UPLAY) Skipped the remembered Uplay login before its dialog appeared" << std::endl);
					return;
				}
			}
		}
		reinterpret_cast<TransitionBeginFn>(transitionBeginTrampoline)(dialog, args);
	}

	void NoteSkipChoice(uintptr_t dialog) {
		static bool noted = false;
		int action = 0;
		uintptr_t listener = 0;
		unsigned char cancelReceived = 0;
		if (noted || !Read(dialog + off_pendingAction, action) || action != actionSkipConfirm
			|| !Read(dialog + off_confirmListener, listener) || !listener || !Read(listener + off_listenerCancelReceived, cancelReceived) || !cancelReceived)
			return;
		noted = true;
		if (Settings::ReturnSettingValue(Setting::UplayLogin) != settingAsk)
			return;
		SettingEdits::Apply({ Setting::UplayLogin, "Mod Settings", "UplayLogin", settingSkip, "Uplay Login", false });
		skippedThisSession.store(true, std::memory_order_release);
		LOG_INFO("(UPLAY) The player skipped the Uplay login; later launches skip it" << std::endl);
	}

	void TrySkip(uintptr_t dialog) {
		uintptr_t vtable = 0, listener = 0;
		int state = 0, action = 0;
		unsigned char input = 0;
		const bool idle = Read(dialog, vtable) && vtable == dialogVtable
			&& Read(dialog + off_state, state) && state == stateInputReady
			&& Read(dialog + off_inputEnabled, input) && input != 0
			&& Read(dialog + off_pendingAction, action) && action == 0
			&& Read(dialog + off_confirmListener, listener) && listener == 0;
		if (!idle)
			return;

		uintptr_t connect = 0, connectTable = 0;
		if (!Read(connectSingleton, connect) || !Read(connect, connectTable) || connectTable != connectVtable)
			return;

		skipped = true;
		// Cancellation pops the dialog; do not access it afterward.
		reinterpret_cast<CancelFn>(dialogCancel)(reinterpret_cast<void*>(dialog));
		reinterpret_cast<SkipFn>(connectSkip)(reinterpret_cast<void*>(connect));
		skippedThisSession.store(true, std::memory_order_release);
		LOG_INFO("(UPLAY) Skipped the Uplay login, as the player chose before" << std::endl);
	}

	int __fastcall DialogTick(void* dialog, void*) {
		const uintptr_t self = reinterpret_cast<uintptr_t>(dialog);
		NoteSkipChoice(self);
		const int result = reinterpret_cast<TickFn>(dialogTick)(dialog);
		if (!skipped && SkipUplay())
			TrySkip(self);
		return result;
	}
}

SettingDefs UplayLoginMod::Settings() const {
	return {
		Enum(Setting::UplayLogin, "Uplay Login")
			.Hint("Skip goes straight to your profile at startup, without the Uplay sign-in, which no longer works.\n"
				"Set it to Ask if you see problems signing in; Ask shows Rocksmith's own login.")
			.Ini("Mod Settings", "UplayLogin")
			.Choices({ settingAsk, settingSkip }, settingSkip)
			.Heading(Framework::SettingHeading::SongsAndProfiles),
	};
}

bool UplayLoginMod::IsSourceProfileLoading() { return sourceSignInStarted != 0; }

namespace {
	// Learn & Play supports source sign-in and SkipLogin hooks only; unmapped dialog/RDV hooks stay off.
	void InstallLnp() {
		const struct { uintptr_t site; const char* role; } unmapped[] = {
			{ 0x006A9080, "UIMenu_SelectionListDialog::OnDialogCanceled: Uplay and ConnectingToRDV dialog skips off" },
			{ 0x006A9095, "the dialog ID read inside it: ConnectingToRDV cancel off" },
			{ 0x00961350, "the connect service's profile-service getter: saved-login detection off" },
			{ 0x008C4AB0, "the RDV metadata download entry: RDV metadata guard off" },
			{ 0x008C4B0C, "its no-download branch: RDV metadata guard off" },
		};
		for (const auto& site : unmapped)
			LOG_INFO("(UPLAY) Learn & Play: 2022 site 0x" << std::hex << site.site << std::dec << " is unmapped, " << site.role << std::endl);

		uintptr_t config = 0;
		const uintptr_t gameService = Offsets::startupGameService;
		unsigned char configBytes[] = { 0x55, 0x8B, 0xEC, 0x51, 0xA1, 0, 0, 0, 0 };
		memcpy(configBytes + 5, &gameService, sizeof(gameService));
		if (!Read(configBoolSlot, config) || config != configBool || !MemUtil::MatchesBytes(configBool, configBytes)) {
			LOG_WARNING("(UPLAY) Learn & Play: config bool slot or bytes did not match; source sign-in not installed" << std::endl);
			return;
		}
		const uintptr_t configHook = reinterpret_cast<uintptr_t>(&ConfigBoolHook);
		if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(configBoolSlot), &configHook, sizeof(configHook))) {
			LOG_WARNING("(UPLAY) Learn & Play: config bool slot write failed; source sign-in not installed" << std::endl);
			return;
		}
		LOG_INFO("(UPLAY) Learn & Play: RendezVous.SkipLogin answer installed" << std::endl);
		InstallSourceFlow();
	}
}

void UplayLoginMod::Install() {
	connectSingleton = Offsets::startupConnectSingleton;
	connectVtable = Offsets::startupConnectVtable;
	configBoolSlot = Offsets::startupConfigBoolSlot;
	configBool = Offsets::startupConfigBool;
	if (GameVersion::IsLearnAndPlay()) {
		InstallLnp();
		return;
	}
	if (!GameVersion::IsSeptember2022()) {
		LOG_WARNING("(UPLAY) Not installed: the executable is not a supported build" << std::endl);
		return;
	}
	uintptr_t tick = 0, cancel = 0, config = 0, listTickNow = 0, listCancel = 0;
	uintptr_t dialogTransition = 0, listTransition = 0;
	if (!Read(dialogTickSlot, tick) || tick != dialogTick || !Read(dialogCancelSlot, cancel) || cancel != dialogCancel
		|| !MemUtil::MatchesBytes(connectSkip, connectSkipBytes) || !Read(configBoolSlot, config) || config != configBool
		|| !Read(listTickSlot, listTickNow) || listTickNow != listTick || !Read(listCancelSlot, listCancel) || listCancel != dialogCancel) {
		LOG_WARNING("(UPLAY) Not installed: this isn't the Sept 2022 build" << std::endl);
		return;
	}
	if (!Read(transitionBeginSlot, dialogTransition) || dialogTransition != transitionBegin
		|| !Read(listTransitionBeginSlot, listTransition) || listTransition != transitionBegin
		|| !MemUtil::MatchesBytes(transitionBegin, transitionBeginBytes)) {
		LOG_WARNING("(UPLAY) Early dialog skip not installed: transition callback did not match" << std::endl);
		return;
	}
	if (MemUtil::MatchesBytes(downloadLatestMetadata, downloadEntryBytes)
		&& MemUtil::MatchesBytes(downloadNoop, downloadNoopBytes)) {
		downloadTrampoline = reinterpret_cast<uintptr_t>(MemUtil::TrampHook(
			reinterpret_cast<PBYTE>(downloadLatestMetadata),
			reinterpret_cast<PBYTE>(&DownloadLatestMetadataHook), sizeof(downloadEntryBytes)));
		if (downloadTrampoline)
			LOG_INFO("(UPLAY) RDV metadata guard installed" << std::endl);
		else
			LOG_WARNING("(UPLAY) RDV metadata guard hook failed; Uplay dialog hooks still install" << std::endl);
	} else {
		LOG_WARNING("(UPLAY) RDV metadata guard bytes did not match; Uplay dialog hooks still install" << std::endl);
	}
	hideLoadingDialogReady = MemUtil::MatchesBytes(hideDialog, hideDialogBytes)
		&& MemUtil::MatchesBytes(uiDialogManagerHide, uiDialogManagerHideBytes);
	if (!hideLoadingDialogReady)
		LOG_WARNING("(UPLAY) Profile-loading dialog left visible: native hide bytes did not match" << std::endl);
	transitionBeginTrampoline = reinterpret_cast<uintptr_t>(MemUtil::TrampHook(
		reinterpret_cast<PBYTE>(transitionBegin), reinterpret_cast<PBYTE>(&TransitionInBegin),
		sizeof(transitionBeginBytes)));
	if (!transitionBeginTrampoline) {
		LOG_WARNING("(UPLAY) Early dialog skip not installed: transition hook failed" << std::endl);
		return;
	}
	const uintptr_t hook = reinterpret_cast<uintptr_t>(&DialogTick);
	MemUtil::PatchAdr(reinterpret_cast<LPVOID>(dialogTickSlot), &hook, sizeof(hook));
	const uintptr_t configHook = reinterpret_cast<uintptr_t>(&ConfigBoolHook);
	MemUtil::PatchAdr(reinterpret_cast<LPVOID>(configBoolSlot), &configHook, sizeof(configHook));
	const uintptr_t listHook = reinterpret_cast<uintptr_t>(&ListTick);
	MemUtil::PatchAdr(reinterpret_cast<LPVOID>(listTickSlot), &listHook, sizeof(listHook));
	LOG_INFO("(UPLAY) Installed" << std::endl);
	InstallSourceFlow();
}

static Framework::ModRegistrar<UplayLoginMod> _uplayLoginReg;
