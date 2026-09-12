#include "stdafx.h"
#include "D3DOverlay.hpp"
#include "Framework/HudRegistry.hpp"
#include "GameState.hpp"

#include <windows.h>
#include <wrl/client.h>
#include <algorithm>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

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

	Resolution GetWindowSize() {
		RECT windowSize;
		Resolution currentSize{};
		if (GetWindowRect(D3DHooks::GetGameWindow(), &windowSize))
		{
			currentSize.width = windowSize.right - windowSize.left;
			currentSize.height = windowSize.bottom - windowSize.top;
		}
		return currentSize;
	}

	struct FontKey {
		std::string face;
		int height;
		int width;
		int weight;
		bool italic;

		static std::string NormalizeFace(std::string s) {
			auto not_space = [](int c) { return !std::isspace(c); };
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
			s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
			std::transform(s.begin(), s.end(), s.begin(),
				[](unsigned char c) { return char(std::tolower(c)); });
			return s;
		}

		bool operator==(const FontKey& o) const {
			return height == o.height && weight == o.weight && italic == o.italic && face == o.face;
		}

		static FontKey Make(std::string face, int h, int w, int wt, bool it) {
			return FontKey{ NormalizeFace(std::move(face)), h, w, wt, it };
		}
	};

	struct FontKeyHash {
		size_t operator()(const FontKey& k) const {
			size_t h = std::hash<std::string>()(k.face);
			h ^= static_cast<size_t>(k.height) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= static_cast<size_t>(k.weight) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= static_cast<size_t>(k.italic) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};

	class FontCache {
	public:
		bool Get(IDirect3DDevice9* dev, const FontKey& key, ComPtr<ID3DXFont>& out) {
			out.Reset();
			if (auto it = cache.find(key); it != cache.end() && it->second) {
				out = it->second;
				return true;
			}

			ComPtr<ID3DXFont> font;
			HRESULT hr = D3DXCreateFontA(dev, key.height, 0, key.weight, 1, key.italic,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, key.face.c_str(), font.GetAddressOf());
			if (FAILED(hr) || !font) return false;

			auto [iter, inserted] = cache.try_emplace(key, font);
			out = iter->second;

			return true;
		}

		void OnLostDevice() const {
			for (const auto& [key, fontPtr] : cache) if (fontPtr) fontPtr->OnLostDevice();
		}

		void OnResetDevice() const {
			for (const auto& [key, fontPtr] : cache) if (fontPtr) fontPtr->OnResetDevice();
		}
	private:
		std::unordered_map<FontKey, ComPtr<ID3DXFont>, FontKeyHash> cache;
	};

	FontCache fontCache;
	std::string cachedFontName = "";
	int cachedFontSize = 0;
	ComPtr<ID3DXFont> cachedFont;

	void DX9DrawText(const std::string& textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize = { 0u, 0u }, DWORD format = DT_LEFT | DT_NOCLIP)
	{
		ComPtr<ID3DXFont> font;
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

	void CheckCurrentFont(IDirect3DDevice9* device) {
		const std::string currentFontName = Settings::ReturnSettingValue(Setting::OnScreenFont);
		const int currentFontSize = Settings::GetModSetting(Setting::OnScreenFontSize);

		if (cachedFontName != currentFontName || cachedFontSize != currentFontSize || !cachedFont) {
			LOG_INFO("Font settings changed. Re-caching default font..." << std::endl);

			FontKey newKey = FontKey::Make(currentFontName, currentFontSize, 0, FW_NORMAL, false);
			ComPtr<ID3DXFont> newFont;

			if (fontCache.Get(device, newKey, newFont)) {
				cachedFont = newFont;
				cachedFontName = currentFontName;
				cachedFontSize = currentFontSize;
			}
			else {
				LOG_ERROR("Failed to create and cache new default font!" << std::endl);
			}
		}
	}

	// SnapshotVisible() returns copies, so no lock is held across the DX9 draw calls and mod code
	// is never re-entered on the render thread.
	void DrawModHud(IDirect3DDevice9* device, const Resolution& windowSize) {
		std::vector<Framework::HudElement> elements = Framework::Hud().SnapshotVisible();

		// Deterministic stacking: by anchor, then order, then id; owner pointer only as a final stable tiebreak.
		std::sort(elements.begin(), elements.end(),
			[](const Framework::HudElement& a, const Framework::HudElement& b) {
				if (a.anchor != b.anchor) return a.anchor < b.anchor;
				if (a.order != b.order)   return a.order < b.order;
				if (a.id != b.id)         return a.id < b.id;
				return a.owner < b.owner;
			});

		const float defaultStep = windowSize.height / 54.0f; // legacy per-line spacing when no custom height
		bool haveAnchor = false;
		Framework::HudAnchor anchor{};
		AnchorLayout layout{};
		float cursorY = 0;

		for (const Framework::HudElement& element : elements) {
			if (!haveAnchor || element.anchor != anchor) {
				layout = AnchorStart(element.anchor, windowSize);
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
}

// ID3DXFont holds a D3DPOOL_DEFAULT glyph atlas that must be released before an
// IDirect3DDevice9::Reset and rebuilt after, or draws through it corrupt the frame once
// the device is back.
void GameOverlay::OnLostDevice() {
	fontCache.OnLostDevice();
}

void GameOverlay::OnResetDevice() {
	fontCache.OnResetDevice();
}

void GameOverlay::RenderOverlay(IDirect3DDevice9* device) {
	// Always derive positions from windowSize fractions — never hardcode pixels — so every resolution places text consistently.
	if (GameState::GameLoaded) {
		const Resolution windowSize = GetWindowSize();
		CheckCurrentFont(device);
		DrawModHud(device, windowSize);
	}
}
