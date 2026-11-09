#include "stdafx.h"
#include "Menu.hpp"
#include "DirectInputCapture.hpp"
#include "Framework/Framework.hpp"
#include "Mods/EnumerationDrain.hpp"
#include "D3DOverlay.hpp"
#include "Lib/ImGUI/imgui_internal.h" // ErrorCheckEndWindowRecover, after a callback throws mid-window

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
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

		// After a callback threw partway through its ImGui calls: close what it left open down to `windowDepth`, so
		// the frame's Begin/End and Push/Pop stacks still balance.
		void RecoverImGuiStack(int windowDepth) {
			ImGuiContext& g = *ImGui::GetCurrentContext();
			while (g.CurrentWindowStack.Size > windowDepth) {
				ImGui::ErrorCheckEndWindowRecover(nullptr);
				if (g.CurrentWindow->Flags & ImGuiWindowFlags_ChildWindow) ImGui::EndChild();
				else ImGui::End();
			}
			ImGui::ErrorCheckEndWindowRecover(nullptr);
		}

		// A mod's throw is handled like a throwing draw callback's: the next MainThread tick faults it (logging why
		// once) and drops its entries; until then it may throw again and is recovered the same way. A host entry has
		// no mod to fault, so it's only logged.
		void ReportThrow(const Framework::IMod* owner, const std::string& id, const char* where, const char* detail) {
			if (owner) {
				Framework::Draw().ReportFault(owner, where, detail);
			}
			else {
				LOG_ERROR("Exception in " << where << " '" << id << "': " << detail << std::endl);
			}
		}

		// Runs one ImGui callback inside the frame; see ReportThrow for what a throw does.
		template <typename Fn>
		void RunGuarded(const Framework::IMod* owner, const std::string& id, const char* where, Fn&& fn) {
			const int windowDepth = ImGui::GetCurrentContext()->CurrentWindowStack.Size;
			try {
				fn();
				return;
			}
			catch (const std::exception& e) {
				ReportThrow(owner, id, where, e.what());
			}
			catch (...) {
				ReportThrow(owner, id, where, "threw an unknown exception");
			}
			RecoverImGuiStack(windowDepth);
		}

		// View over the registry's status snapshot, gated behind the opt-in toggle below. Its only
		// action is retrying faulted mods.
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

			if (counts[static_cast<int>(Framework::ModStatusKind::Faulted)] > 0) {
				ImGui::SameLine();
				if (ImGui::SmallButton("Retry faulted")) Framework::Registry().RequestRetryFaulted();
			}

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

	// Anchor scan progress to the centered 16:9 menu frame.
	void RenderEnumerationProgress() {
		if (!EnumerationDrain::IsInstalled())
			return;

		const auto progress = EnumerationDrain::GetProgress();
		static float visibility = 0.0f;
		const ImGuiIO& io = ImGui::GetIO();
		const double hold = progress.upToDate ? 1.0 : 2.0;
		const bool wanted = progress.active && (!progress.completed || progress.completionElapsedSeconds < hold);
		const float fadeTarget = !wanted ? 0.0f
			: progress.completed ? (std::min)(1.0f, static_cast<float>((hold - progress.completionElapsedSeconds) / 0.8))
			: 1.0f;
		const float step = (std::min)(1.0f, io.DeltaTime / 0.25f);
		visibility = fadeTarget < visibility ? fadeTarget : (std::min)(fadeTarget, visibility + step);
		if (visibility <= 0.001f)
			return;

		const bool indeterminate = !progress.completed && (progress.detected == 0 || progress.processed == 0);
		// EndScene can run repeatedly per frame, so DeltaTime cannot drive fill easing.
		const float fraction = progress.completed ? 1.0f
			: progress.detected ? static_cast<float>(progress.processed) / progress.detected : 0.0f;

		const float menuWidth = (std::min)(io.DisplaySize.x, io.DisplaySize.y * (16.0f / 9.0f));
		const float menuLeft = (io.DisplaySize.x - menuWidth) * 0.5f;
		const float unit = menuWidth / 1366.0f;
		const float width = 376.0f * unit;
		const float right = menuLeft + menuWidth - 24.0f * unit;
		const float top = 24.0f * unit;
		const float fontScale = unit / io.FontGlobalScale;

		ImGui::SetNextWindowPos(ImVec2(right - width, top), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
		constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f * unit, 10.0f * unit));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, visibility);
		ImGui::Begin("##RSModsEnumerationProgress", nullptr, flags);
		ImGui::SetWindowFontScale(fontScale);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const auto alpha = [&](int a) { return static_cast<int>(a * visibility); };

		const ImU32 cyan = IM_COL32(60, 200, 240, alpha(255));
		const ImU32 green = IM_COL32(90, 220, 110, alpha(255));
		const ImU32 greenFlash = IM_COL32(170, 255, 180, alpha(255));
		const ImU32 trough = IM_COL32(24, 26, 28, alpha(255));
		const ImU32 white = IM_COL32(255, 255, 255, alpha(255));
		const ImU32 shadow = IM_COL32(0, 0, 0, alpha(160));
		const ImU32 ink = IM_COL32(16, 22, 26, alpha(255));
		const ImU32 dim = IM_COL32(215, 215, 215, alpha(255));

		const auto shadowedText = [&](ImVec2 at, ImU32 colour, const char* text) {
			drawList->AddText(ImVec2(at.x + 1.5f * unit, at.y + 1.5f * unit), shadow, text);
			drawList->AddText(at, colour, text);
		};

		const char* title = progress.completed ? (progress.upToDate ? "UP TO DATE" : "UPDATED!") : "ENUMERATING DLC";
		const float titleWidth = ImGui::CalcTextSize(title).x;
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
		shadowedText(ImGui::GetCursorScreenPos(), white, title);
		ImGui::Dummy(ImVec2(titleWidth, ImGui::GetTextLineHeight()));

		const float barHeight = 16.0f * unit;
		const float rounding = 5.0f * unit;
		const ImVec2 barMin = ImGui::GetCursorScreenPos();
		const ImVec2 barMax(barMin.x + ImGui::GetContentRegionAvail().x, barMin.y + barHeight);
		const bool flash = progress.completed && progress.completionElapsedSeconds < 0.45 && fmod(progress.completionElapsedSeconds, 0.15) < 0.075;
		const ImU32 fill = progress.completed ? (flash ? greenFlash : green) : cyan;
		drawList->AddRectFilled(barMin, barMax, trough, rounding);
		float fillRight = barMin.x;
		if (indeterminate) {
			const float span = barMax.x - barMin.x;
			const float sweepWidth = span * 0.22f;
			const float phase = static_cast<float>(fmod(ImGui::GetTime(), 1.4) / 1.4);
			const float sweepLeft = barMin.x - sweepWidth + (span + sweepWidth) * phase;
			drawList->PushClipRect(barMin, barMax, true);
			drawList->AddRectFilled(ImVec2(sweepLeft, barMin.y), ImVec2(sweepLeft + sweepWidth, barMax.y), cyan, rounding);
			drawList->PopClipRect();
		}
		else if (fraction > 0.0f) {
			fillRight = barMin.x + (barMax.x - barMin.x) * fraction;
			drawList->PushClipRect(barMin, ImVec2(fillRight, barMax.y), true);
			drawList->AddRectFilled(barMin, barMax, fill, rounding);
			drawList->PopClipRect();
		}

		ImGui::SetWindowFontScale(fontScale * 0.62f);
		const std::string countText = std::to_string(progress.processed) + " / " + std::to_string(progress.detected);
		const ImVec2 countSize = ImGui::CalcTextSize(countText.c_str());
		const ImVec2 countAt((barMin.x + barMax.x - countSize.x) * 0.5f, (barMin.y + barMax.y - countSize.y) * 0.5f);
		drawList->PushClipRect(barMin, ImVec2(fillRight, barMax.y), true);
		drawList->AddText(countAt, ink, countText.c_str());
		drawList->PopClipRect();
		drawList->PushClipRect(ImVec2(fillRight, barMin.y), barMax, true);
		shadowedText(countAt, white, countText.c_str());
		drawList->PopClipRect();
		ImGui::Dummy(ImVec2(barMax.x - barMin.x, barHeight + 6.0f * unit));

		// Not added includes duplicate, unowned and unreadable packages.
		ImGui::SetWindowFontScale(fontScale * 0.72f);
		std::ostringstream stats;
		stats << progress.notRegistered << " NOT ADDED / " << progress.newDlc << " NEW / " << progress.totalDlc << " TOTAL / "
			<< std::fixed << std::setprecision(1) << progress.elapsedSeconds << "s";
		const std::string statsText = stats.str();
		const float statsWidth = ImGui::CalcTextSize(statsText.c_str()).x;
		ImGui::SetCursorPosX((std::max)(0.0f, (ImGui::GetWindowSize().x - statsWidth) * 0.5f));
		shadowedText(ImGui::GetCursorScreenPos(), dim, statsText.c_str());
		ImGui::Dummy(ImVec2(statsWidth, ImGui::GetTextLineHeight()));
		ImGui::SetWindowFontScale(1.0f);

		const ImVec2 windowMin = ImGui::GetWindowPos();
		const ImVec2 windowMax(windowMin.x + ImGui::GetWindowSize().x, windowMin.y + ImGui::GetWindowSize().y);
		const float plateRounding = 6.0f * unit;
		const ImU32 plateTop = IM_COL32(96, 106, 120, alpha(200));
		const ImU32 plateBottom = IM_COL32(38, 44, 54, alpha(215));
		ImDrawList* background = ImGui::GetBackgroundDrawList();
		background->AddRectFilled(windowMin, windowMax, plateTop, plateRounding);
		background->AddRectFilledMultiColor(ImVec2(windowMin.x, windowMin.y + plateRounding), ImVec2(windowMax.x, windowMax.y - plateRounding),
			plateTop, plateTop, plateBottom, plateBottom);
		background->AddRectFilled(ImVec2(windowMin.x, windowMax.y - plateRounding), windowMax, plateBottom, plateRounding, ImDrawFlags_RoundCornersBottom);

		ImGui::End();
		ImGui::PopStyleVar(3);
	}

	/// <summary>
	/// Renders the ImGui frame, including the main mod menu.
	/// </summary>
	void RenderImGuiMenu() {
		// Mods' fonts go in outside the frame, where the atlas isn't locked; NewFrame then re-uploads the texture.
		auto fontLoads = Framework::Menus().TakePendingFonts();
		if (!fontLoads.empty()) {
			// Outside a frame there's no window stack to recover, so not RunGuarded.
			for (const auto& load : fontLoads) {
				try {
					load.fn();
				}
				catch (const std::exception& e) {
					ReportThrow(load.owner, "fonts", "font load", e.what());
				}
				catch (...) {
					ReportThrow(load.owner, "fonts", "font load", "threw an unknown exception");
				}
			}

			ImGui::GetIO().Fonts->Build();
			ImGui_ImplDX9_InvalidateDeviceObjects();
		}

		GameOverlay::PrepareImGuiHud(); // bake/rebuild HUD fonts before NewFrame recreates the atlas texture

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		RenderEnumerationProgress();

		const auto entries = Framework::Menus().GetEntries();

		for (const auto& entry : entries) {
			if (!entry.overlay || !entry.drawFn) continue;

			RunGuarded(entry.owner, entry.id, "overlay", entry.drawFn);
		}

		if (Menu::menuEnabled) {

			// The RS Mods window holds the debug tools and mod status, so release builds only get player-facing windows.
			if (D3DHooks::debug) {
				ImGui::Begin("RS Mods");
				for (const auto& entry : entries) {
					if (entry.standaloneWindow || entry.overlay || !entry.drawFn) continue;

					RunGuarded(entry.owner, entry.id, "menu", [&] {
						if (ImGui::CollapsingHeader(entry.title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
							entry.drawFn();
						}
					});
				}

				ImGui::Separator();
				ImGui::Checkbox("Mod status", &g_showModStatus);
				if (g_showModStatus)
					RunGuarded(nullptr, "status", "mod status", DrawModStatus);
				ImGui::End();
			}

			for (const auto& entry : entries) {
				if (!entry.standaloneWindow || entry.overlay || !entry.drawFn) continue;
				if (!entry.playerFacing && !D3DHooks::debug) continue;

				if (ImGui::Begin(entry.title.c_str()))
					RunGuarded(entry.owner, entry.id, "standalone menu", entry.drawFn);
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

		DirectInputCapture::Install();

		LOG_INFO("ImGUI Init" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] { Settings::UpdateSettings(); });
	}
}
