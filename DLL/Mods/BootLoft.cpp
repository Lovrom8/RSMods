#include "../stdafx.h"
#include "BootLoft.hpp"
#include "UltrafastLoadMod.hpp"

namespace {
	uintptr_t kBootLoftLightSite = 0;
	constexpr unsigned char kBootLoftLightExpected[] = {
		0x83, 0x7F, 0x14, 0x00, 0x75, 0x0C, 0x6A, 0x00, 0x6A, 0x00, 0x53, 0x33, 0xC0, 0xE8, 0x5A, 0x00, 0x00, 0x00
	};
	uintptr_t kBootLoftLightCall = 0;
	uintptr_t loftSetState = 0;
	bool bootLoftLightDone = false;

	// The first stateless loft call sets immediate at stack argument +8.
	void __declspec(naked) BootLoftLightThunk() {
		__asm {
			cmp byte ptr [bootLoftLightDone], 0
			jne original
			mov byte ptr [bootLoftLightDone], 1
			push ecx
			mov ecx, [esp + 8]
			cmp dword ptr [ecx + 0x20], 0
			jne restore
			mov dword ptr [esp + 0x0C], 1
		restore:
			pop ecx
		original:
			jmp dword ptr [loftSetState]
		}
	}
	void InstallLight() {
		kBootLoftLightSite = GameVersion::CheckedSite("LOFT LIGHT", { "first light set", 0x00775304, 0 },
            kBootLoftLightExpected, nullptr, sizeof(kBootLoftLightExpected));
        if (!kBootLoftLightSite) {
			LOG_WARNING("(LOFT LIGHT) Boot light byte span mismatch at 0x00775304; no changes made" << std::endl);
			return;
		}
		kBootLoftLightCall = kBootLoftLightSite + 13;
		loftSetState = kBootLoftLightCall + 5 + 0x5A;
		const int32_t relative = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&BootLoftLightThunk) - (kBootLoftLightCall + 5));
		if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(kBootLoftLightCall + 1), &relative, sizeof(relative))) {
			LOG_ERROR("(LOFT LIGHT) Boot light patch failed" << std::endl);
			return;
		}
		LOG_INFO("(LOFT LIGHT) The boot's first loft light set is immediate; the loft starts fully lit" << std::endl);
	}
    void* frustumTrampoline = nullptr;
    volatile unsigned char clampFarNow = 1;

    // NiCamera receives its caller-owned frustum in EDX.
    void __declspec(naked) FrustumHook() {
        __asm {
            pushfd
            cmp byte ptr [clampFarNow], 0
            je original
            mov dword ptr [edx + 0x14], 0x3F800000
        original:
            popfd
            jmp dword ptr [frustumTrampoline]
        }
    }

    bool installed = false;
    bool hidden = false;
    bool atMainMenu = false;

    void SetVisible(bool visible) {
        const uintptr_t farAddress = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft,
            Offsets::ptr_loft_farOffsets, true);
        if (farAddress)
            *reinterpret_cast<float*>(farAddress) = visible ? 10000.f : 1.f;
    }

    class BootLoftMod : public Framework::IMod {
    public:
        MOD_ID(BootLoftMod)
        bool IsEnabled(const Framework::ModContext&) const override { return installed; }
        void OnTick(Framework::ModContext& c) override {
            const auto& menu = GameState::currentMenu;
            if (menu == "pre_enter_prompt" || menu == "TitleScreen")
                atMainMenu = false;
            else if (menu == "MainMenu" || menu == "UplayLoginDialog" || menu == "ProfileSelect" || menu == "SimpleDialog")
                atMainMenu = true;
            if (!atMainMenu) {
                clampFarNow = 1;
                SetVisible(false);
                if (!hidden) {
                    hidden = true;
                    LOG_INFO("(LOFT) hidden until MainMenu (menu " << menu << ")" << std::endl);
                }
            }
            else if (hidden) {
                hidden = false;
                clampFarNow = 0;
                if (!(c.IsOn(Settings::Setting::ToggleLoftEnabled)
                    && c.When(Settings::Setting::ToggleLoftWhen) == Settings::When::Startup)) {
                    SetVisible(true);
                    LOG_INFO("(LOFT) restored on " << menu << std::endl);
                }
            }
        }
    };
    Framework::ModRegistrar<BootLoftMod> bootLoftReg;
}

void BootLoft::Install() {
    if (!UltrafastLoad::Enabled())
        return;
    if (!GameVersion::IsSeptember2022()) {
        LOG_INFO("(LOFT LIGHT) Boot loft changes disabled on this build" << std::endl);
        return;
    }
    InstallLight();
    constexpr unsigned char expected[] = { 0x55, 0x8B, 0xEC, 0x51, 0xD9, 0x42, 0x10 };
    const auto site = GameVersion::CheckedSite("LOFT", { "SetViewFrustum", 0x009D90C0, 0 },
        expected, nullptr, sizeof(expected));
    if (!site)
        return;
    frustumTrampoline = MemUtil::TrampHook(reinterpret_cast<PBYTE>(site),
        reinterpret_cast<PBYTE>(&FrustumHook), sizeof(expected));
    installed = frustumTrampoline != nullptr;
}
