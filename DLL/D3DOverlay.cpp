#include "stdafx.h"
#include "D3DOverlay.hpp"
#include "Mods/DropPedal.hpp"

/// <returns>Size of Rocksmith Window</returns>
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

/// <summary>
/// Draw text on screen
/// </summary>
/// <param name="textToDraw"> - What text should be written?</param>
/// <param name="textColorHex"> - What color? Given in hex in the AA,RR,GG,BB format.</param>
/// <param name="topLeftX"> - top LEFT of textbox</param>
/// <param name="topLeftY"> - TOP left of textbox</param>
/// <param name="bottomRightX"> - bottom RIGHT of textbox</param>
/// <param name="bottomRightY"> - BOTTOM right of textbox</param>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="setFontSize"> - Override font size</param>
/// <param name="format"> - DrawText format</param>
void GameOverlay::DX9DrawText(const std::string& textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize, DWORD format)
{
	CComPtr<ID3DXFont> font;
	bool useInputFontSize = setFontSize.height != 0;

	if (useInputFontSize) {
		int targetH = setFontSize.height;
		const std::string face = Settings::ReturnSettingValue("OnScreenFont");
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

	// Preload And Draw The Text (Supposed to reduce the performance hit (It's D3D/DX9 but still good practice))
	font->PreloadTextA(textToDraw.c_str(), textToDraw.length());
	font->DrawTextA(nullptr, textToDraw.c_str(), -1, &TextRectangle, format, textColorHex);
}

/// <summary>
/// Draw a filled, alpha blended rectangle, for use as a backing plate behind text
/// that would otherwise be unreadable over the game's artwork.
/// </summary>
void GameOverlay::DX9DrawFilledRectangle(int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, D3DCOLOR color, LPDIRECT3DDEVICE9 pDevice)
{
	struct Vertex
	{
		float x, y, z, rhw;
		D3DCOLOR color;
	};

	const float left = (float)topLeftX;
	const float top = (float)topLeftY;
	const float right = (float)bottomRightX;
	const float bottom = (float)bottomRightY;

	Vertex quad[4] =
	{
		{ left,  bottom, 0.0f, 1.0f, color },
		{ left,  top,    0.0f, 1.0f, color },
		{ right, bottom, 0.0f, 1.0f, color },
		{ right, top,    0.0f, 1.0f, color },
	};

	DWORD previousFvf = 0;
	DWORD previousAlphaBlend = 0;
	DWORD previousSourceBlend = 0;
	DWORD previousDestBlend = 0;
	DWORD previousTexture = 0;

	pDevice->GetFVF(&previousFvf);
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &previousAlphaBlend);
	pDevice->GetRenderState(D3DRS_SRCBLEND, &previousSourceBlend);
	pDevice->GetRenderState(D3DRS_DESTBLEND, &previousDestBlend);
	pDevice->GetTextureStageState(0, D3DTSS_COLOROP, &previousTexture);

	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->SetTexture(0, nullptr);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));

	pDevice->SetTextureStageState(0, D3DTSS_COLOROP, previousTexture);
	pDevice->SetRenderState(D3DRS_DESTBLEND, previousDestBlend);
	pDevice->SetRenderState(D3DRS_SRCBLEND, previousSourceBlend);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, previousAlphaBlend);
	pDevice->SetFVF(previousFvf);
}

/// <summary>
/// Measure the pixel width of a string in the default overlay font, without drawing.
/// Returns 0 if the font is not ready, which the caller treats as "do not draw".
/// </summary>
int GameOverlay::MeasureTextWidth(const std::string& textToDraw, LPDIRECT3DDEVICE9 pDevice)
{
	if (!cachedFont)
	{
		return 0;
	}

	RECT measured{ 0, 0, 0, 0 };
	cachedFont->DrawTextA(nullptr, textToDraw.c_str(), -1, &measured, DT_CALCRECT | DT_NOCLIP, whiteText);
	return measured.right - measured.left;
}

void GameOverlay::DisplayMixer() {
	// Display the whole mixer if displayMixer is true
	if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && displayMixer) {

		float offset = 0;
		for (int volumeIndex = 0; volumeIndex < mixerInternalNames.size(); ++volumeIndex) {

			float volume = 0;
			RTPCValue_type type = RTPCValue_GameObject;
			Wwise::SoundEngine::Query::GetRTPCValue(mixerInternalNames[volumeIndex].c_str(), AK_INVALID_GAME_OBJECT, &volume, &type);

			DX9DrawText(
				drawMixerTextName[volumeIndex] + std::to_string(static_cast<int>(volume)) + "%",
				whiteText,
				static_cast<int>(WindowSize.width / 96.0f),  // 20 pixels from left in 1920x1080 resolution
				static_cast<int>(WindowSize.height / 54.0f + offset), // 20 pixels from top (plus an offset to display multiple values)
				static_cast<int>(WindowSize.width / 19.2f),  // 120 pixels from left
				static_cast<int>(WindowSize.height / 16.0f), // 120 pixels from top
				pDevice);

			// Adjust the offset to display the next value
			offset += WindowSize.height / 54.0f;
		}
	}
	// Display just the current volume based on context (This will display the last volume that was adjusted for a few seconds after adjusting it)
	else if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && displayCurrentVolume) {
		float volume = 0;
		RTPCValue_type type = RTPCValue_GameObject;
		Wwise::SoundEngine::Query::GetRTPCValue(mixerInternalNames[currentVolumeIndex].c_str(), AK_INVALID_GAME_OBJECT, &volume, &type);

		DX9DrawText(
			drawMixerTextName[currentVolumeIndex] + std::to_string(static_cast<int>(volume)) + "%",
			whiteText,
			static_cast<int>(WindowSize.width / 96.0f),  // 20 pixels from left in 1920x1080 resolution
			static_cast<int>(WindowSize.height / 54.0f), // 20 pixels from top 
			static_cast<int>(WindowSize.width / 19.2f),  // 120 pixels from left
			static_cast<int>(WindowSize.height / 16.0f), // 120 pixels from top
			pDevice);
	}
}

void GameOverlay::DisplaySongTimer()
{
	if (D3DHooks::showSongTimerOnScreen && SongTimer::SongTimer() != 0.f) {
		DX9DrawText(
			D3DHooks::ConvertFloatTimeToStringTime(SongTimer::SongTimer()),
			whiteText,
			static_cast<int>(WindowSize.width - WindowSize.width / 16.0f), // 120 pixels left from right edge in 1920x1080 resolution
			static_cast<int>(WindowSize.height / 54.0f),                   // 20 pixels from top
			static_cast<int>(WindowSize.width - WindowSize.width / 96.0f), // 20 left from right edge
			static_cast<int>(WindowSize.height / 16.0f),                   // 120 pixels from top
			pDevice,
			{ NULL, NULL },
			DT_RIGHT | DT_NOCLIP);
	}
}

void GameOverlay::DisplayCurrentNote()
{
	if (Settings::ReturnSettingValue("ShowCurrentNoteOnScreen") == "on" && GuitarSpeak::GetCurrentNoteName() != (std::string)"") {

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
	if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on" && RiffRepeater::loggedCurrentSongID &&
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

void GameOverlay::DisplayDropPedalTuning()
{
	if (Settings::GetKeyBind("DropPedalToggleKey") == NULL)
		return;

	const std::string state = DropPedal::IsEnabled()
		? DropPedal::GetTuningName()
		: "off";

	const std::string line = "Drop Pedal: " + state;

	// Measuring also serves as the readiness gate: if the font is not cached the width
	// is 0, and we skip drawing the plate too. This keeps the box and the text in step,
	// which is what stops the plate flickering on its own during menu transitions.
	const int textWidth = MeasureTextWidth(line, pDevice);
	if (textWidth == 0)
	{
		return;
	}

	int textColor = greyText;
	if (DropPedal::IsEnabled())
	{
		const int direction = DropPedal::GetShiftDirection();
		textColor = direction < 0 ? dropPedalDownText : (direction > 0 ? dropPedalUpText : whiteText);
	}

	// Top left, clear of the game's own corner furniture. The plate is sized to the
	// text rather than a fixed width, so short tunings do not leave a long empty box.
	const int left = static_cast<int>(WindowSize.width / 96.0f);			// 20 pixels in at 1920x1080
	const int top = static_cast<int>(WindowSize.height / 54.0f);			// 20 pixels down
	const int padding = static_cast<int>(WindowSize.width / 192.0f);		// 10 pixels

	const int right = left + textWidth;
	const int bottom = top + static_cast<int>(WindowSize.height / 27.0f);	// 40 pixels tall

	DX9DrawFilledRectangle(left - padding, top - padding / 2, right + padding, bottom, D3DCOLOR_ARGB(150, 0, 0, 0), pDevice);

	DX9DrawText(
		line,
		textColor,
		left,
		top,
		right + padding,
		bottom,
		pDevice,
		{ NULL, NULL },
		DT_LEFT | DT_NOCLIP);
}

void GameOverlay::DisplayCurrentTuningForAutoTune()
{
	if (Settings::ReturnSettingValue("AutoTuneForSong") == "on" && Settings::GetKeyBind("TuningOffsetKey") != NULL && GameState::Menus::IsInTuningMenus()) {
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
		"Loop: " + D3DHooks::ConvertFloatTimeToStringTime(loopStart) + " - " + D3DHooks::ConvertFloatTimeToStringTime(loopEnd),
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
	if (Settings::ReturnSettingValue("AllowLooping") == "on" && (Keybindings::loopStart != NULL || Keybindings::loopEnd != NULL)) {
		// Only enable looping in learn a song modes (learn a song & non-stop play)
		if (GameState::Menus::IsInLearnASongModes()) {
			GameOverlay::DisplayLoopStartEndTimes(Keybindings::loopStart, Keybindings::loopEnd);

			// Prevent the user from creating a loop that starts at a negative timestamp.
			if ((Settings::GetModSetting("LoopingLeadUp") / 1000.f) >= Keybindings::loopStart) {
				Keybindings::roughLoopStart = 0.f;
			}
			else {
				Keybindings::roughLoopStart = Keybindings::loopStart - (Settings::GetModSetting("LoopingLeadUp") / 1000.f);
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
	if (Settings::ReturnSettingValue("DisplayCurrentAccuracy") == "on" &&
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
		else { //JIC
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
	const std::string currentFontName = Settings::ReturnSettingValue("OnScreenFont");
	const int currentFontSize = Settings::GetModSetting("OnScreenFontSize");

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

void GameOverlay::RenderOverlay(IDirect3DDevice9* device) {
	// Draw text on screen
	// NOTE: NEVER USE SET VALUES. Always do division of WindowSize width AND heigh so every resolution should have the text in around the same spot.
	if (GameState::GameLoaded) {
		WindowSize = GetWindowSize();
		pDevice = device;

		CheckCurrentFont();

		DisplayMixer();
		DisplaySongTimer();
		DisplayRiffRepeaterOverHundredPercentSpeed();
		DisplayCurrentNote();
		DisplayCurrentTuningForAutoTune();
		DisplayDropPedalTuning();
		DisplaySongAccuracy();

		HandleLooping();
	}
}