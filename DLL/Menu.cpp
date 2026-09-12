#include "stdafx.h"
#include "Menu.hpp"
#include "Framework/Framework.hpp"

namespace Menu {
	/// <summary>
	/// Checks if the EndScene call originates from an overlay (e.g., Steam)
	/// instead of the game to prevent rendering our UI twice.
	/// </summary>
	/// <returns>True if the call is from an overlay, false otherwise.</returns>
	bool IsOverlayCall() {
		return (uint32_t)_ReturnAddress() > Offsets::baseEnd.Get();
	}

	/// <summary>
	/// Renders the ImGui frame, including the main mod menu.
	/// </summary>
	void RenderImGuiMenu() {
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
