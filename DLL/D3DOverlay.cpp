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

namespace GameOverlay { namespace {
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
}} // namespace GameOverlay::(anonymous)

void GameOverlay::DisplayCurrentNote()
{
	if (Settings::IsOn(Setting::ShowCurrentNoteOnScreen) && GuitarSpeak::GetCurrentNoteName() != (std::string)"") {

		if (GameState::IsInSong()) {
			DX9DrawText(
				GuitarSpeak::GetCurrentNoteName(),
				whiteText,
				static_cast<int>(WindowSize.width / 5.5),		// 349 pixels left of the center in 1920x1080 resolution.
				static_cast<int>(WindowSize.height / 1.75),	// 617 pixels from the top
				static_cast<int>(WindowSize.width / 5.75),		// 334 pixels right of center
				static_cast<int>(WindowSize.height / 8),		// 135 pixels from the top
				pDevice);
		}
		else { // Show outside of the song at the top of the screen.
			DX9DrawText(
				"Current Note: " + GuitarSpeak::GetCurrentNoteName(),
				whiteText,
				static_cast<int>(WindowSize.width / 3.87),		// 496 pixels left of the center in 1920x1080 resolution
				static_cast<int>(WindowSize.height / 30.85),	// 35 pixels from the top
				static_cast<int>(WindowSize.width / 4),		// 480 pixel right of the center
				static_cast<int>(WindowSize.height / 8),		// 135 pixels from the top
				pDevice);
		}
	}
}

void GameOverlay::DisplayRiffRepeaterOverHundredPercentSpeed()
{
	if (Settings::IsOn(Setting::RRSpeedAboveOneHundred) && RiffRepeater::loggedCurrentSongID &&
		(GameState::Menus::IsInModesWithAllowedFastRiffRepeater() || GameState::Menus::IsOnScoreScreens()) || RiffRepeater::currentlyEnabled_Above100) {
		realSongSpeed = RiffRepeater::GetSpeed(true); // While this should almost always be the same value, the user might enable riff repeater, which could cause this number to be wrong.

		DX9DrawText(
			"Song Speed: " + std::to_string(static_cast<int>(roundf(realSongSpeed))) + "%",
			whiteText,
			static_cast<int>(WindowSize.width / 2.0f - WindowSize.width / 38.4f), // 50 pixels left of center in 1920x1080 resolution
			static_cast<int>(WindowSize.height / 54.0f),                          // 20 pixels from top
			static_cast<int>(WindowSize.width / 2.0f + WindowSize.width / 38.4f), // 50 pixels right of center
			static_cast<int>(WindowSize.height / 16.0f),                          // 120 pixels from top
			pDevice,
			{ NULL, NULL },
			DT_CENTER | DT_NOCLIP);
	}
}

void GameOverlay::DisplayCurrentTuningForAutoTune()
{
	if (Settings::IsOn(Setting::AutoTuneForSong) && Settings::GetKeyBind(Setting::Key::TuningOffset) != NULL && GameState::Menus::IsInTuningMenus()) {
		DX9DrawText(
			"Auto Tune For: " + Midi::GetTuningOffsetName(Midi::tuningOffset),
			whiteText,
			static_cast<int>(WindowSize.width / 5.5),		// 349 pixels left of the center in 1920x1080 resolution
			static_cast<int>(WindowSize.height / 30.85),	// 35 pixels from the top
			static_cast<int>(WindowSize.width / 5.65),		// 339 pixels right of center
			static_cast<int>(WindowSize.height / 8),		// 135 pixels from the top
			pDevice);
	}
}

void GameOverlay::DisplayLoopStartEndTimes(float loopStart, float loopEnd)
{
	DX9DrawText(
		"Loop: " + SongTimer::FormatTime(loopStart) + " - " + SongTimer::FormatTime(loopEnd),
		whiteText,
		static_cast<int>(WindowSize.width / 2.0f - WindowSize.width / 38.4f), // 50 pixels left of center in 1920x1080 resolution
		static_cast<int>(WindowSize.height / 21.6f),                          // 50 pixels from top
		static_cast<int>(WindowSize.width / 2.0f + WindowSize.width / 38.4f), // 50 pixels right of center
		static_cast<int>(WindowSize.height / 7.2f),                           // 150 pixels from top
		pDevice,
		{ NULL, NULL },
		DT_CENTER | DT_NOCLIP);
}

void HandleLooping() {
	if (Settings::IsOn(Setting::AllowLooping) && (Keybindings::loopStart != NULL || Keybindings::loopEnd != NULL)) {
		// Only enable looping in learn a song modes (learn a song & non-stop play)
		if (GameState::Menus::IsInLearnASongModes()) {
			GameOverlay::DisplayLoopStartEndTimes(Keybindings::loopStart, Keybindings::loopEnd);

			// Prevent the user from creating a loop that starts at a negative timestamp.
			if ((Settings::GetModSetting(Setting::LoopingLeadUp) / 1000.f) >= Keybindings::loopStart) {
				Keybindings::roughLoopStart = 0.f;
			}
			else {
				Keybindings::roughLoopStart = Keybindings::loopStart - (Settings::GetModSetting(Setting::LoopingLeadUp) / 1000.f);
			}

			// If we are paused, reset the grey note timer.
			if (GameState::Menus::IsInLearnASongPauseModes()) {
				// Resets grey note timer to loopStart. This makes it so notes in the loop are not deactivated.
				// Deactivated notes are greyed out, and do not register with note detection.
				// As an added bonus the game also automatically adds a bit of lead time so the player has some time to prepare.
				if (SongTimer::GetGreyNoteTimer() != Keybindings::loopStart) {
					SongTimer::SetGreyNoteTimer(Keybindings::loopStart);
				}
			}

			// If not paused AND we are at the end of the loop, seek to the start of the loop.
			else if (Keybindings::loopStart != NULL && Keybindings::loopEnd != NULL && (SongTimer::SongTimer() >= Keybindings::loopEnd)) {
				Wwise::SoundEngine::SeekOnEvent(std::string("Play_" + GameState::GetSongKey()).c_str(), 0x1234, (AkTimeMs)(Keybindings::roughLoopStart * 1000), false);
			}
		}
		// Difference between learnASongModes & fastRRModes is the inclusion of RR. This means that this check is only gets the RR menus.
		else if (GameState::Menus::IsInModesWithAllowedFastRiffRepeater()) {
			// Reset loopStart and loopEnd to NULL as the user wants to do a loop with RR, or is changing some settings.
			Keybindings::loopStart = NULL;
			Keybindings::loopEnd = NULL;
		}
	}

}

static int MeasureLineHeight(ID3DXFont* font, const std::string& text, const RECT& rect, DWORD fmt) {
	RECT r = rect;
	int h = font->DrawTextA(nullptr, text.c_str(), -1, &r, fmt | DT_CALCRECT, 0);
	if (h <= 0) h = (r.bottom - r.top);
	return h;
}

static float ReadAccuracy() {
	const bool isLAS = GameState::Menus::IsInLearnASongModes();
	const bool isSA = GameState::Menus::IsInScoreAttackModes();

	uintptr_t addr = 0;
	if (isLAS) {
		addr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_noteData,
			Offsets::ptr_noteDataOffsets);
	}
	else if (isSA) {
		addr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_scoreAttackNoteData,
			Offsets::ptr_scoreAttackNoteDataOffsets);
	}
	else {
		return 0.0f;
	}

	if (!addr) return 0.0f;

	if (isLAS) {
		const LearnASongNoteData* data = reinterpret_cast<LearnASongNoteData*>(addr);

		return data->getAccuracy();
	}
	else if (isSA) {
		const ScoreAttackNoteData* data = reinterpret_cast<ScoreAttackNoteData*>(addr);
		return data->getAccuracy();
	}

	return 0.0f;
}

void GameOverlay::DisplaySongAccuracy() {
	if (Settings::IsOn(Setting::DisplayCurrentAccuracy) &&
		GameState::IsInSong() && SongTimer::SongTimer() != 0.f) {
		auto left = static_cast<int>(WindowSize.width - WindowSize.width / 16.0f);
		auto right = static_cast<int>(WindowSize.width - WindowSize.width / 96.0f);
		auto top = static_cast<int>(WindowSize.height / 54.0f);
		auto bottom = static_cast<int>(WindowSize.height / 16.0f);
		RECT baseRect{ left, top, right, bottom };

		float accuracy = ReadAccuracy();
		std::stringstream ss;
		ss << std::fixed << std::setprecision(2) << accuracy << "%";
		std::string accuracyText = ss.str();

		if (cachedFont) {
			int lh = MeasureLineHeight(cachedFont, accuracyText, baseRect, DT_RIGHT | DT_NOCLIP);
			int gap = (std::max)(1, lh / 4);
			top += lh + 2 * gap;
			bottom += lh + 2 * gap;
		}
		else {
			auto line = static_cast<int>(WindowSize.height / 54.0f);
			top += line;
			bottom += line;
		}

		DX9DrawText(
			accuracyText,
			whiteText,
			left, top, right, bottom,
			pDevice,
			{ NULL, NULL },
			DT_RIGHT | DT_NOCLIP);
	}
}

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
		const float step = fontHeight > 0 ? static_cast<float>(fontHeight) : defaultStep;

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

		DisplayRiffRepeaterOverHundredPercentSpeed();
		DisplayCurrentNote();
		DisplayCurrentTuningForAutoTune();
		DisplaySongAccuracy();
		DrawModHud(device);

		HandleLooping();
	}
}