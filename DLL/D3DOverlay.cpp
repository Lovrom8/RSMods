#include "stdafx.h"
#include "D3DOverlay.hpp"
#include "Framework/HudRegistry.hpp"
#include "GameState.hpp"
#include "ImGuiFontManager.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>
#include <vector>

namespace Setting = Settings::Setting;

namespace {
	struct PreparedLine {
		std::string text;
		Framework::HudAnchor anchor{};
		int fontHeight = 0;      // 0 => shared default size
		ImFont* font = nullptr;  // nullptr => ImGui default font
		ImU32 color = 0;
	};

	std::vector<PreparedLine> g_prepared;

	ImU32 ArgbToImU32(int argb) {
		unsigned a = (argb >> 24) & 0xFF;
		unsigned r = (argb >> 16) & 0xFF;
		unsigned g = (argb >> 8) & 0xFF;
		unsigned b = argb & 0xFF;
		if (a == 0) a = 255; // HUD text is opaque; guard a color with no alpha
		return IM_COL32(r, g, b, a);
	}
}

void GameOverlay::PrepareImGuiHud() {
	// Before NewFrame: bake any newly needed fonts and rebuild the atlas off-frame, so this frame's
	// NewFrame re-uploads the texture and the glyphs are ready in time to draw.
	// SnapshotVisible() returns copies, so no lock is held across ImGui calls.
	g_prepared.clear();
	if (!GameState::GameLoaded) return;

	std::vector<Framework::HudElement> elements = Framework::Hud().SnapshotVisible();

	// Deterministic stacking; owner pointer only as a final stable tiebreak.
	std::sort(elements.begin(), elements.end(),
		[](const Framework::HudElement& a, const Framework::HudElement& b) {
			if (a.anchor != b.anchor) return a.anchor < b.anchor;
			if (a.order != b.order)   return a.order < b.order;
			if (a.id != b.id)         return a.id < b.id;
			return a.owner < b.owner;
		});

	const std::string face = Settings::ReturnSettingValue(Setting::OnScreenFont);
	const int defaultSize = Settings::GetModSetting(Setting::OnScreenFontSize);

	g_prepared.reserve(elements.size());
	for (const Framework::HudElement& e : elements) {
		const int size = e.snapshot.fontHeight > 0 ? e.snapshot.fontHeight : defaultSize;
		ImFont* font = ImGuiHud::AcquireFont(face, size);
		g_prepared.push_back(PreparedLine{
			e.snapshot.text, e.anchor, e.snapshot.fontHeight, font, ArgbToImU32(e.snapshot.colorHex) });
	}

	if (ImGuiHud::ConsumeAtlasDirty()) {
		ImGui::GetIO().Fonts->Build();
		ImGui_ImplDX9_InvalidateDeviceObjects(); // NewFrame recreates the font texture
	}
}

void GameOverlay::DrawImGuiHud() {
	// Inside the frame. Background draw list keeps HUD text under the mod menu window.
	if (g_prepared.empty()) return;

	const ImVec2 display = ImGui::GetIO().DisplaySize;
	if (display.x <= 0 || display.y <= 0) return;
	const Framework::HudArea area = Framework::Hud().LayoutArea(std::floor(display.x), std::floor(display.y));

	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	ImFont* defaultFont = ImGui::GetFont();
	const float defaultStep = area.height / 54.0f; // legacy per-line spacing when no custom height

	bool haveAnchor = false;
	Framework::HudAnchor anchor{};
	Framework::HudBand band{};
	float cursorY = 0.0f;

	for (const PreparedLine& line : g_prepared) {
		if (!haveAnchor || line.anchor != anchor) {
			band = Framework::AnchorBand(line.anchor, area);
			cursorY = band.top;
			anchor = line.anchor;
			haveAnchor = true;
		}

		ImFont* font = line.font ? line.font : defaultFont;
		const float sizePx = line.fontHeight > 0
			? static_cast<float>(line.fontHeight)
			: font->FontSize;

		const ImVec2 extent = font->CalcTextSizeA(sizePx, FLT_MAX, 0.0f, line.text.c_str());

		float x = band.left;
		if (band.align == Framework::HudAlign::Right)
			x = band.right - extent.x;
		else if (band.align == Framework::HudAlign::Center)
			x = (band.left + band.right) * 0.5f - extent.x * 0.5f;

		// Legacy per-line advance: custom height when set, else measured height plus a quarter-line gap.
		float step = defaultStep;
		if (line.fontHeight > 0) {
			step = static_cast<float>(line.fontHeight);
		}
		else if (extent.y > 0.0f) {
			const float gap = (std::max)(1.0f, extent.y / 4.0f);
			step = (std::max)(defaultStep, extent.y + 2.0f * gap);
		}

		dl->AddText(font, sizePx, ImVec2(x, cursorY), line.color, line.text.c_str());
		cursorY += step;
	}
}
