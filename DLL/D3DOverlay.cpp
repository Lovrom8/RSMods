#include "stdafx.h"
#include "D3DOverlay.hpp"
#include "Framework/HudRegistry.hpp"

namespace Setting = Settings::Setting;

namespace {
	// Pixel band per anchor, derived from the live window size. Insets deliberately match the old
	// hand-written overlays so single-occupant stacks land pixel-for-pixel where they always did.
	struct AnchorLayout {
		LONG left;
		LONG right;
		LONG top;
		DWORD format;
	};

	AnchorLayout AnchorStart(Framework::HudAnchor anchor, const Resolution& window) {
		const float w = static_cast<float>(window.width);
		const float h = static_cast<float>(window.height);

		switch (anchor) {
		case Framework::HudAnchor::TopRight:
			return { static_cast<LONG>(w - w / 16.0f), static_cast<LONG>(w - w / 96.0f),
					 static_cast<LONG>(h / 54.0f), DT_RIGHT | DT_NOCLIP };
		case Framework::HudAnchor::TopCenter:
			return { static_cast<LONG>(w / 2.0f - w / 38.4f), static_cast<LONG>(w / 2.0f + w / 38.4f),
					 static_cast<LONG>(h / 54.0f), DT_CENTER | DT_NOCLIP };
		case Framework::HudAnchor::TopTuning:
			return { static_cast<LONG>(w / 5.5f), static_cast<LONG>(w / 5.65f),
					 static_cast<LONG>(h / 30.85f), DT_LEFT | DT_NOCLIP };
		case Framework::HudAnchor::HighwayLeft:
			return { static_cast<LONG>(w / 5.5f), static_cast<LONG>(w / 5.75f),
					 static_cast<LONG>(h / 1.75f), DT_LEFT | DT_NOCLIP };
		case Framework::HudAnchor::MenuBanner:
			return { static_cast<LONG>(w / 3.87f), static_cast<LONG>(w / 4.0f),
					 static_cast<LONG>(h / 30.85f), DT_LEFT | DT_NOCLIP };
		case Framework::HudAnchor::TopLeft:
		default:
			return { static_cast<LONG>(w / 96.0f), static_cast<LONG>(w / 19.2f),
					 static_cast<LONG>(h / 54.0f), DT_LEFT | DT_NOCLIP };
		}
	}
}

Resolution GameOverlay::GetWindowSize() {
	RECT windowSize;

	Resolution currentSize;
	if (GetWindowRect(D3DHooks::GetGameWindow(), &windowSize))
	{
		currentSize.width = windowSize.right - windowSize.left;
		currentSize.height = windowSize.bottom - windowSize.top;
	}

	return currentSize;
}

namespace GameOverlay {
	namespace {
		void DX9DrawText(const std::string& textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize = { 0u, 0u }, DWORD format = DT_LEFT | DT_NOCLIP)
		{
			CComPtr<ID3DXFont> font;
			bool useInputFontSize = setFontSize.height != 0;

			if (useInputFontSize) {
				int targetH = setFontSize.height;
				const std::string face = Settings::ReturnSettingValue(Setting::OnScreenFont);
				FontKey key = FontKey::Make(face, targetH, 0, FW_NORMAL, false);

				if (!fontCache.Get(pDevice, key, font)) {
					LOG_ERROR("Could not acquire custom-sized font." << std::endl);
					return;
				}
			}
			else {
				if (cachedFont) {
					font = cachedFont;
				}
				else {
					LOG_ERROR("Default font is not cached!" << std::endl);
					return;
				}
			}

			RECT TextRectangle{ topLeftX, topLeftY, bottomRightX, bottomRightY }; // Left, Top, Right, Bottom

			font->PreloadTextA(textToDraw.c_str(), textToDraw.length());
			font->DrawTextA(nullptr, textToDraw.c_str(), -1, &TextRectangle, format, textColorHex);
		}
	}
} // namespace GameOverlay::(anonymous)


void GameOverlay::CheckCurrentFont() {
	const std::string currentFontName = Settings::ReturnSettingValue(Setting::OnScreenFont);
	const int currentFontSize = Settings::GetModSetting(Setting::OnScreenFontSize);

	if (cachedFontName != currentFontName || cachedFontSize != currentFontSize || !cachedFont) {
		LOG_INFO("Font settings changed. Re-caching default font..." << std::endl);

		FontKey newKey = FontKey::Make(currentFontName, currentFontSize, 0, FW_NORMAL, false);
		CComPtr<ID3DXFont> newFont;

		if (fontCache.Get(pDevice, newKey, newFont)) {
			cachedFont = newFont;
			cachedFontName = currentFontName;
			cachedFontSize = currentFontSize;
		}
		else {
			LOG_ERROR("Failed to create and cache new default font!" << std::endl);
		}
	}
}

// ID3DXFont holds a D3DPOOL_DEFAULT glyph atlas that must be released before an
// IDirect3DDevice9::Reset and rebuilt after, or draws through it corrupt the frame once
// the device is back (the Alt+Tab white-screen when "show current note" had drawn a glyph).
void GameOverlay::OnLostDevice() {
	if (DX9FontEncapsulation)
		DX9FontEncapsulation->OnLostDevice();
	fontCache.OnLostDevice();
}

void GameOverlay::OnResetDevice() {
	if (DX9FontEncapsulation)
		DX9FontEncapsulation->OnResetDevice();
	fontCache.OnResetDevice();
}

// SnapshotVisible() returns copies, so no lock is held across the DX9 draw calls and mod code
// is never re-entered on the render thread.
void GameOverlay::DrawModHud(IDirect3DDevice9* device) {
	std::vector<Framework::HudElement> elements = Framework::Hud().SnapshotVisible();

	// Deterministic stacking: by anchor, then order, then id; owner pointer only as a final stable tiebreak.
	std::sort(elements.begin(), elements.end(),
		[](const Framework::HudElement& a, const Framework::HudElement& b) {
			if (a.anchor != b.anchor) return a.anchor < b.anchor;
			if (a.order != b.order)   return a.order < b.order;
			if (a.id != b.id)         return a.id < b.id;
			return a.owner < b.owner;
		});

	const float defaultStep = WindowSize.height / 54.0f; // legacy per-line spacing when no custom height
	bool haveAnchor = false;
	Framework::HudAnchor anchor{};
	AnchorLayout layout{};
	float cursorY = 0;

	for (const Framework::HudElement& element : elements) {
		if (!haveAnchor || element.anchor != anchor) {
			layout = AnchorStart(element.anchor, WindowSize);
			cursorY = static_cast<float>(layout.top);
			anchor = element.anchor;
			haveAnchor = true;
		}

		const int fontHeight = element.snapshot.fontHeight;
		float step = defaultStep;

		if (fontHeight > 0) {
			step = static_cast<float>(fontHeight);
		}
		else if (cachedFont) {
			RECT r{ layout.left, 0, layout.right, 0 };
			int h = cachedFont->DrawTextA(nullptr, element.snapshot.text.c_str(), -1, &r, layout.format | DT_CALCRECT, 0);
			if (h <= 0) h = (r.bottom - r.top);
			if (h > 0) {
				int gap = (std::max)(1, h / 4);
				step = (std::max)(defaultStep, static_cast<float>(h + 2 * gap));
			}
		}

		DX9DrawText(element.snapshot.text, element.snapshot.colorHex,
			layout.left, static_cast<int>(cursorY), layout.right, static_cast<int>(cursorY + step),
			device, { 0u, static_cast<unsigned int>(fontHeight) }, layout.format);

		cursorY += step;
	}
}

void GameOverlay::RenderOverlay(IDirect3DDevice9* device) {
	// Always derive positions from WindowSize fractions — never hardcode pixels — so every resolution places text consistently.
	if (GameState::GameLoaded) {
		WindowSize = GetWindowSize();
		pDevice = device;

		CheckCurrentFont();
		DrawModHud(device);
	}
}