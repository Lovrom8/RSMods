#include "../../stdafx.h"
#include "WavyNotesEffect.hpp"
#include "../../SitePatchGroup.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects {
	namespace {
		using namespace Framework::InstructionPatch;

		constexpr auto nativeNotePosition = MakeBytes(
			0xD9, 0x00, 0xD9, 0x5C, 0x24, 0x24, 0xD9, 0x40, 0x04, 0xD9, 0x5C, 0x24, 0x28,
			0xD9, 0x40, 0x08, 0xD9, 0x5C, 0x24, 0x2C, 0xD9, 0x44, 0x24, 0x24, 0xD9, 0x5B,
			0x70, 0xD9, 0x44, 0x24, 0x28, 0xD9, 0x5B, 0x74, 0xD9, 0x44, 0x24, 0x2C, 0xD9,
			0x5B, 0x78);
		static_assert(nativeNotePosition.size() == 41);
		// RunCommand holds effectsMutex around Test and Start, which serializes group access.
		SitePatch::PatchedSiteGroup wavyNotesPatch("CROWD CONTROL");
		bool wavyNotesUnavailable = false;

		MemUtil::PatchResult InstallWavyNotesHook() {
			if (wavyNotesUnavailable)
				return MemUtil::PatchResult::Original;

			const auto address = Offsets::ptr_wavyNotesHook.GetValue();
			std::array<std::uint8_t, 41> replacement{};
			if (!BuildRelativeJump(address, reinterpret_cast<std::uintptr_t>(&NotePositionHook), replacement)) {
				SitePatch::LogUnavailable("CROWD CONTROL", "Wavy Notes", "note-position hook");
				wavyNotesUnavailable = true;
				return MemUtil::PatchResult::Original;
			}

			const auto site = SitePatch::MakeSite("note-position hook", address,
				VersionedPattern<41>{ ExactPattern(nativeNotePosition), ExactPattern(nativeNotePosition) }, replacement);
			const auto result = wavyNotesPatch.Install("Wavy Notes", SitePatch::ReadSite, site);
			wavyNotesUnavailable = result == MemUtil::PatchResult::Original;
			return result;
		}
	}

	extern "C" static void ApplyWavyNotes()
	{
		if (wavyNotesEnabled && vec->y > -5.0f) {
			vec->y = vec->y + (sin(vec->z / 40.0f) * 4.0f);
		}
	}

	void __declspec(naked) NotePositionHook()
	{
		/*
			007AE0AD | D900			| fld st(0),dword ptr ds:[eax]		| load z
			007AE0AF | D95C24 24	| fstp dword ptr ss:[esp+24],st(0)	| store z to tmp
			007AE0B3 | D940 04		| fld st(0),dword ptr ds:[eax+4]	| load x
			007AE0B6 | D95C24 28	| fstp dword ptr ss:[esp+28],st(0)	| store x to tmp
			007AE0BA | D940 08		| fld st(0),dword ptr ds:[eax+8]	| load y
			007AE0BD | D95C24 2C	| fstp dword ptr ss:[esp+2C],st(0)	| store y to tmp
			007AE0C1 | D94424 24	| fld st(0),dword ptr ss:[esp+24]	| load z from tmp
			007AE0C5 | D95B 70		| fstp dword ptr ds:[ebx+70],st(0)	| store z to object
			007AE0C8 | D94424 28	| fld st(0),dword ptr ss:[esp+28]	| load x from tmp
			007AE0CC | D95B 74		| fstp dword ptr ds:[ebx+74],st(0)	| store x to object
			007AE0CF | D94424 2C	| fld st(0),dword ptr ss:[esp+2C]	| load y from tmp
			007AE0D3 | D95B 78		| fstp dword ptr ds:[ebx+78],st(0)	| store y to object
		*/
		__asm {
			mov vec, eax
			pushad
			call ApplyWavyNotes
			popad
			fld dword ptr[eax]
			fstp dword ptr[esp + 0x24]
			fld dword ptr[eax + 0x04]
			fstp dword ptr[esp + 0x28]
			fld dword ptr[eax + 0x08]
			fstp dword ptr[esp + 0x2C]
			fld dword ptr[esp + 0x24]
			fstp dword ptr[ebx + 0x70]
			fld dword ptr[esp + 0x28]
			fstp dword ptr[ebx + 0x74]
			fld dword ptr[esp + 0x2C]
			fstp dword ptr[ebx + 0x78]
			test esp, esp // ZF = 0: the game's jz right after isn't taken (ApplyWavyNotes leaves the flags unknown)

			push offset Offsets::ptr_wavyNotesJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	Enums::EffectStatus WavyNotesEffect::Test(const Structs::Request&)
	{
		LOG_INFO("WavyNotesEffect::Test()" << std::endl);

		if (!CanStart())
			return Enums::EffectStatus::Retry;
		if (InstallWavyNotesHook() != MemUtil::PatchResult::Installed)
			return Enums::EffectStatus::Unavailable;

		return Enums::EffectStatus::Success;
	}


	Enums::EffectStatus WavyNotesEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("WavyNotesEffect::Start()" << std::endl);
		if (InstallWavyNotesHook() != MemUtil::PatchResult::Installed)
			return Enums::EffectStatus::Unavailable;

		SetDuration(request);
		running = true;
		wavyNotesEnabled = true;

		return Enums::EffectStatus::Success;
	}

	Enums::EffectStatus WavyNotesEffect::OnStop()
	{
		LOG_INFO("WavyNotesEffect::Stop()" << std::endl);

		wavyNotesEnabled = false;
		running = false;

		return Enums::EffectStatus::Success;
	}
}
