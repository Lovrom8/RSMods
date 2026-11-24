#include "../stdafx.h"
#include "../Framework/InstructionPatch.hpp"
#include "../InputGainPolicy.hpp"
#include "InputGainMod.hpp"

namespace {
	constexpr char iniSection[] = "Mod Settings";
	constexpr char iniName[] = "FixClampedInputGain";

	bool SettingOn() {
		wchar_t executable[MAX_PATH * 2]{};
		const DWORD length = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(std::size(executable)));
		if (!length || length >= std::size(executable))
			return false;

		const auto ini = std::filesystem::path(executable).replace_filename(L"RSMods.ini");
		CSimpleIniA reader;
		if (reader.LoadFile(ini.c_str()) < 0)
			return true;
		return std::string(reader.GetValue(iniSection, iniName, "on")) != "off";
	}

	// Each 2022 span ends at the call to the mode and instrument key selector.
	template <size_t N>
	uintptr_t CheckedFunction(const char* name, uintptr_t va, const std::array<uint8_t, N>& bytes) {
		using namespace Framework::InstructionPatch;
		const VersionedPattern<N> patterns{ ExactPattern(bytes), UnavailablePattern<N>() };
		const auto& pattern = GameVersion::IsSeptember2022() ? patterns.september2022 : patterns.learnAndPlay;
		if (!pattern.available)
			return 0;
		return GameVersion::CheckedSite("INPUT GAIN", { name, va, 0 }, pattern.bytes.data(), pattern.mask.data(), N);
	}
}

Framework::SettingDefs InputGainMod::Settings() const {
	return {
		Framework::SettingDef::Toggle(iniName, iniName, "Fix Clamped Input Gain", iniSection, iniSection, "on")
			.Hint("Restore the default input gain at startup if calibration stored the maximum.\n"
				"Keeps the gain you chose with the in-game input gain override.")
			.Heading(Framework::SettingHeading::Audio),
	};
}

void InputGainMod::OnProfileLoaded() {
	// Only the first profile load belongs to startup, and the native clear always uses player zero.
	// The hook's argument is an untyped uintptr_t the original code called a profile, so nothing is
	// read from it: gating on an unconfirmed player index would silently skip the check.
	static bool checked = false;
	if (checked)
		return;
	checked = true;

	const bool settingOn = SettingOn();
	if (!settingOn) {
		LOG_INFO("(INPUT GAIN) Startup check off" << std::endl);
		return;
	}

	using Framework::InstructionPatch::MakeBytes;
	const auto getGain = CheckedFunction("GetNormalizedMicGain", 0x00866E10, MakeBytes(
		0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x2C, 0xA1, 0x40, 0xB2, 0x25, 0x01, 0x33, 0xC5, 0x89, 0x45, 0xFC,
		0xD9, 0xEE, 0x53, 0x56, 0xD9, 0x5D, 0xE0, 0x57, 0x8B, 0xF9, 0x8D, 0x75, 0xE4, 0x89, 0x7D, 0xD8,
		0xE8, 0x1B, 0xFF, 0xFF, 0xFF));
	const auto getOverride = CheckedFunction("IsMicGainOverridden", 0x00867030, MakeBytes(
		0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38, 0xA1, 0x40, 0xB2, 0x25, 0x01, 0x33, 0xC5, 0x89, 0x45, 0xFC,
		0x56, 0x57, 0x8B, 0xF9, 0x8D, 0x75, 0xCC, 0xC6, 0x45, 0xCB, 0x00, 0xE8, 0x00, 0xFD, 0xFF, 0xFF));
	const auto clearGain = CheckedFunction("ClearNormalizedMicGain", 0x008671D0, MakeBytes(
		0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x28, 0xA1, 0x40, 0xB2, 0x25, 0x01, 0x33, 0xC5, 0x89, 0x45, 0xFC,
		0x56, 0x57, 0x33, 0xFF, 0x8D, 0x75, 0xE4, 0xE8, 0x64, 0xFB, 0xFF, 0xFF));
	if (!getGain || !getOverride || !clearGain) {
		LOG_WARNING("(INPUT GAIN) Startup check unavailable on this executable" << std::endl);
		return;
	}

	// Player zero, to match the gain the native clear acts on.
	using IsOverridden = bool(__fastcall*)(unsigned int);
	const InputGainPolicy::GainReading reading{ true, InputGain::ReadPrimaryGain(getGain),
		reinterpret_cast<IsOverridden>(getOverride)(0) };
	const auto action = InputGainPolicy::Decide(reading, settingOn);
	if (action == InputGainPolicy::GainAction::Clear)
		reinterpret_cast<void(__cdecl*)()>(clearGain)();

	LOG_INFO("(INPUT GAIN) normalized=" << reading.normalized
		<< " displayDb=" << InputGainPolicy::NormalizedToDisplayDb(reading.normalized)
		<< " override=" << (reading.overrideEnabled ? "on" : "off")
		<< " action=" << (action == InputGainPolicy::GainAction::Clear ? "cleared" : "kept") << std::endl);
}

static Framework::ModRegistrar<InputGainMod> _inputGainModReg;
