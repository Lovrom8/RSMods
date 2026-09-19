#include "stdafx.h"
#include "Menu.hpp"
#include "Framework/Framework.hpp"
#include "D3DOverlay.hpp"

#include <string>
#include <vector>

namespace Menu {
	/// <summary>
	/// Checks if the EndScene call originates from an overlay (e.g., Steam)
	/// instead of the game to prevent rendering our UI twice.
	/// </summary>
	/// <returns>True if the call is from an overlay, false otherwise.</returns>
	bool IsOverlayCall() {
		return (uint32_t)_ReturnAddress() > Offsets::baseEnd.Get();
	}

	namespace {
		bool g_showModStatus = false;   // Session-only; the diagnostics view is opt-in per launch.

		ImVec4 StatusColor(Framework::ModStatusKind kind) {
			switch (kind) {
			case Framework::ModStatusKind::Active:     return ImVec4(0.40f, 0.85f, 0.40f, 1.0f); // green
			case Framework::ModStatusKind::Suppressed: return ImVec4(0.95f, 0.75f, 0.30f, 1.0f); // amber
			case Framework::ModStatusKind::Faulted:    return ImVec4(0.95f, 0.40f, 0.40f, 1.0f); // red
			case Framework::ModStatusKind::Disabled:   return ImVec4(0.60f, 0.60f, 0.60f, 1.0f); // gray
			case Framework::ModStatusKind::Registered: return ImVec4(0.60f, 0.70f, 0.90f, 1.0f); // blue
			}
			return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		// Read-only view over the registry's status snapshot, gated behind the opt-in toggle below.
		void DrawModStatus() {
			const auto snapshot = Framework::Registry().StatusSnapshot();
			if (snapshot.empty()) {
				ImGui::TextDisabled("No mods registered.");
				return;
			}

			int counts[5] = { 0, 0, 0, 0, 0 };
			for (const auto& s : snapshot) counts[static_cast<int>(s.kind)]++;
			ImGui::Text("%d mods:", static_cast<int>(snapshot.size()));
			ImGui::SameLine();
			ImGui::TextColored(StatusColor(Framework::ModStatusKind::Active), "%d active", counts[static_cast<int>(Framework::ModStatusKind::Active)]);
			ImGui::SameLine(); ImGui::TextUnformatted(",");
			ImGui::SameLine();
			ImGui::TextColored(StatusColor(Framework::ModStatusKind::Suppressed), "%d suppressed", counts[static_cast<int>(Framework::ModStatusKind::Suppressed)]);
			ImGui::SameLine(); ImGui::TextUnformatted(",");
			ImGui::SameLine();
			ImGui::TextColored(StatusColor(Framework::ModStatusKind::Faulted), "%d faulted", counts[static_cast<int>(Framework::ModStatusKind::Faulted)]);

			if (ImGui::BeginTable("mod_status", 3,
				ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Mod");
				ImGui::TableSetupColumn("State");
				ImGui::TableSetupColumn("Detail");
				ImGui::TableHeadersRow();

				for (const auto& s : snapshot) {
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(s.id.c_str());

					ImGui::TableSetColumnIndex(1);
					ImGui::TextColored(StatusColor(s.kind), "%s", Framework::ToString(s.kind));

					ImGui::TableSetColumnIndex(2);
					std::string detail;
					if (s.inSong) detail += "in song";
					if (!s.claimsExclusive.empty()) {
						if (!detail.empty()) detail += "  ";
						detail += "claims ";
						for (size_t i = 0; i < s.claimsExclusive.size(); ++i) {
							if (i) detail += ", ";
							detail += s.claimsExclusive[i];
						}
					}
					if (detail.empty()) ImGui::TextDisabled("-");
					else ImGui::TextDisabled("%s", detail.c_str());
				}

				ImGui::EndTable();
			}
		}
	}

	/// <summary>
	/// Renders the ImGui frame, including the main mod menu.
	/// </summary>
	void RenderImGuiMenu() {
		GameOverlay::PrepareImGuiHud(); // bake/rebuild HUD fonts before NewFrame recreates the atlas texture

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (Menu::menuEnabled) {
			const auto entries = Framework::Menus().GetEntries();

			ImGui::Begin("RS Mods");
			for (const auto& entry : entries) {
				if (entry.standaloneWindow || !entry.drawFn) continue;

				try {
					if (ImGui::CollapsingHeader(entry.title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
						entry.drawFn();
					}
				}
				catch (const std::exception& e) {
					LOG_ERROR("Exception rendering menu '" << entry.id << "': " << e.what() << std::endl);
				}
				catch (...) {
					LOG_ERROR("Unknown exception rendering menu '" << entry.id << "'" << std::endl);
				}
			}

			ImGui::Separator();
			ImGui::Checkbox("Mod status", &g_showModStatus);
			if (g_showModStatus) {
				try {
					DrawModStatus();
				}
				catch (const std::exception& e) {
					LOG_ERROR("Exception rendering mod status: " << e.what() << std::endl);
				}
				catch (...) {
					LOG_ERROR("Unknown exception rendering mod status" << std::endl);
				}
			}
			ImGui::End();

			for (const auto& entry : entries) {
				if (!entry.standaloneWindow || !entry.drawFn) continue;

				if (ImGui::Begin(entry.title.c_str())) {
					try {
						entry.drawFn();
					}
					catch (const std::exception& e) {
						LOG_ERROR("Exception rendering standalone menu '" << entry.id << "': " << e.what() << std::endl);
					}
					catch (...) {
						LOG_ERROR("Unknown exception rendering standalone menu '" << entry.id << "'" << std::endl);
					}
				}
				ImGui::End();
			}
		}

		GameOverlay::DrawImGuiHud(); // HUD draws regardless of whether the settings menu is open

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

		ImGui::CaptureKeyboardFromApp(false);
		ImGui::CaptureMouseFromApp(false);
	}

	void Init(IDirect3DDevice9* pDevice, LONG_PTR WndProc) {
		if (ImGuiInit) {
			return;
		}

		ImGuiInit = true;

		// Create ImGUI
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);
		io.FontDefault = font;

		// Hook WndProc (Keypress manager)
		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		pDevice->GetCreationParameters(&d3dcp);
		D3DHooks::hThisWnd = d3dcp.hFocusWindow;
		D3DHooks::oWndProc = (WNDPROC)SetWindowLongPtr(D3DHooks::hThisWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

		ImGui_ImplWin32_Init(D3DHooks::hThisWnd);
		ImGui_ImplDX9_Init(pDevice);
		ImGui::GetIO().ImeWindowHandle = D3DHooks::hThisWnd;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		LOG_INFO("ImGUI Init" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] { Settings::UpdateSettings(); });
	}
}
