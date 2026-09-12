#include "../stdafx.h"
#include "ExtendedRangeMode.hpp"
#include "CollectColors.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHooks.hpp"
#include "../Settings.hpp"
#include "../StringState.h"

#include <array>
#include <atomic>
#include <thread>

using Settings::StringColorMode;
namespace Setting = Settings::Setting;

/// <param name="stringnum"> - Number of string</param>
/// <param name="state"> - Structs::string_state</param>
/// <returns>Pointer to String Color</returns>
uintptr_t GetStringColor(uintptr_t stringnum, int state) {
	uintptr_t currentStringNum = stringnum; // EDX
	uintptr_t currentStringColor = 0; // EAX
	uintptr_t stringColorPointer = Offsets::ptr_stringColor; // Magic Number

	uintptr_t pointerValue = MemUtil::ReadPtr(stringColorPointer); // ECX

	if (!pointerValue)
		return NULL;

	currentStringColor = MemUtil::ReadPtr(pointerValue + currentStringColor * 0x4 + 0x348);

	if (currentStringColor >= 2)
		return NULL;

	currentStringColor = currentStringColor * 0xA8;
	currentStringColor += currentStringNum;

	currentStringColor = MemUtil::ReadPtr(pointerValue + currentStringColor * 0x4 + state);

	return currentStringColor;
}

/// <summary>
/// Store backup of original string color.
/// </summary>
/// <param name="strings"> - Pointers to string colors</param>
/// <param name="state"> - Structs::string_state</param>
void ERMode::InitStrings(std::vector<uintptr_t>& strings, int state) {
	strings.clear();
	for (int strIndex = 0; strIndex < 6;strIndex++)
		strings.push_back(GetStringColor(strIndex, state));
}

/// <summary>
/// Set strings to their respective colors
/// </summary>
/// <param name="strings"> - Pointers to string colors</param>
/// <param name="colors"> - New String Colors</param>
void ERMode::SetColors(std::vector<uintptr_t> strings, std::vector<RSColor> colors) {
	for (int strIndex = 0; strIndex < 6;strIndex++) {
		if (strings[strIndex] == NULL)
			return;
		*(RSColor*)strings[strIndex] = colors[strIndex];
	}
}

void ERMode::Initialize() {
	std::map<std::string, RSColor> emptyMap;

	for (int str = 0; str < 6; str++)
		customColors.emplace_back(emptyMap);
}

void ERMode::SetCustomColors(int strIdx, const ColorMap& customColorMap) {
	customColors[strIdx] = customColorMap;
}

ColorMap ERMode::GetCustomColors(int strIdx, bool CB) {
	RSColor iniColor;
	std::string ext = CB ? "_CB" : "_N";

	// Get user-defined string color
	iniColor = Settings::GetStringColors(CB)[strIdx];
	int H;
	float S;
	float L;
	CollectColors::RGB2HSL(iniColor.r, iniColor.g, iniColor.b, H, S, L);

	// Create different colors from the user-defined color.
	ColorMap customColorsMap = {
		{"Ambient" + ext, CollectColors::GetAmbientStringColor(H, CB)},
		{"Disabled" + ext, CollectColors::GetDisabledStringColor(H, S, L, CB)},
		{"Enabled" + ext, iniColor},
		{"Glow" + ext, CollectColors::GetGlowStringColor(H)},
		{"PegsTuning" + ext, CollectColors::GetTuningPegColor(H)},
		{"PegsReset" + ext, CollectColors::GetPegResetColor()},
		{"PegsSuccess" + ext, CollectColors::GetPegSuccessColor(CB)},
		{"PegsInTune" + ext, CollectColors::GetPegInTuneColor(H, CB)},
		{"PegsOutTune" + ext, CollectColors::GetPegOutTuneColor()},
		{"TextIndicator" + ext, CollectColors::GetRegTextIndicatorColor(H, CB)},
		{"ForkParticles" + ext, CollectColors::GetRegForkParticlesColor(H, CB)},
		{"NotewayNormal" + ext, CollectColors::GetNotewayNormalColor(H, S, L, CB)},
		{"NotewayAccent" + ext, CollectColors::GetNotewayAccentColor(H, CB)},
		{"NotewayPreview" + ext, CollectColors::GetNotewayPreviewColor(H, CB)},
		{"GC_Main" + ext, CollectColors::GetGuitarcadeMainColor(H, strIdx, CB)},
		{"GC_Add" + ext, CollectColors::GetGuitarcadeAdditiveColor(H, strIdx, CB)},
		{"GC_UI" + ext, CollectColors::GetGuitarcadeUIColor(H, strIdx, CB)}
	};

	return customColorsMap;
}

void ERMode::SetCustomColors() {
	if (customColors.size() < 6) {
		customColors.resize(6);
	}
	for (int strIdx = 0; strIdx < 6; strIdx++) {
		ColorMap customColorsFull;

		ColorMap normalColors = GetCustomColors(strIdx, false);
		ColorMap cbColors = GetCustomColors(strIdx, true);

		customColorsFull.insert(normalColors.begin(), normalColors.end());
		customColorsFull.insert(cbColors.begin(), cbColors.end());

		ERMode::SetCustomColors(strIdx, customColorsFull);
	}
}

namespace {
	void ClampColorComponents(RSColor& c) {
		auto clamp = [](float& value) {
			if (value > 1.0f) value = value - (value - 1.0f);
			if (value < 0.0f) value *= -1.0f;
		};

		clamp(c.r);
		clamp(c.g);
		clamp(c.b);
	}

	void GenerateColorTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, const ColorList& colorsNormal, const ColorList& colorsColorBlind) {
		ColorList colorSet;
		colorSet.reserve(colorsNormal.size() + colorsColorBlind.size());
		colorSet.insert(colorSet.end(), colorsNormal.begin(), colorsNormal.end());
		colorSet.insert(colorSet.end(), colorsColorBlind.begin(), colorsColorBlind.end());

		D3D::GenerateGradientTexture(pDevice, ppTexture, colorSet);
	}
}

void ERMode::GenerateStringTextures(IDirect3DDevice9* pDevice) {
	if (!Settings::IsOn(Settings::Setting::ExtendedRangeEnabled) && Settings::GetStringColorMode() != Settings::StringColorMode::Custom) {
		D3D::ReleaseTexture(&customStringColorTexture);
		return;
	}
	GenerateColorTexture(pDevice, &customStringColorTexture, Settings::GetStringColors(false), Settings::GetStringColors(true));
}

void ERMode::GenerateNoteTextures(IDirect3DDevice9* pDevice) {
	if (!Settings::IsOn(Settings::Setting::SeparateNoteColors) || Settings::GetNoteColorMode() != Settings::NoteColorMode::Custom) {
		D3D::ReleaseTexture(&customNoteColorTexture);
		return;
	}
	GenerateColorTexture(pDevice, &customNoteColorTexture, Settings::GetNoteColors(false), Settings::GetNoteColors(true));
}

void ERMode::GenerateRainbowTextures(IDirect3DDevice9* pDevice) {
	if (!RainbowNotesEnabled && !Settings::IsOn(Settings::Setting::RainbowNotesEnabled)) {
		for (auto& tex : rainbowTextures) {
			D3D::ReleaseTexture(&tex);
		}
		return;
	}

	const size_t expectedCount = static_cast<size_t>(360.0f / rainbowSpeed);
	if (rainbowTextures.size() != expectedCount) {
		rainbowTextures.resize(expectedCount, nullptr);
	}

	int currTexture = 0;
	constexpr float stringOffset = 20.0f;
	constexpr int stringCount = 6;

	for (float h = 0.0f; h < 360.0f; h += rainbowSpeed) {
		ColorList colorsRainbow;
		colorsRainbow.reserve(stringCount + 2);

		for (int i = 0; i < stringCount; ++i) {
			RSColor c;
			c.setH(h + (stringOffset * i));
			ClampColorComponents(c);
			colorsRainbow.push_back(c);
		}

		colorsRainbow.insert(colorsRainbow.end(), 2, colorsRainbow.back());

		ColorList colorSet;
		colorSet.reserve(colorsRainbow.size() * 2);
		colorSet.insert(colorSet.end(), colorsRainbow.begin(), colorsRainbow.end());
		colorSet.insert(colorSet.end(), colorsRainbow.begin(), colorsRainbow.end());

		D3D::GenerateGradientTexture(pDevice, &rainbowTextures[currTexture++], colorSet);
	}
}

void ERMode::RegenerateTextures(IDirect3DDevice9* pDevice) {
	if (!pDevice) return;
	GenerateStringTextures(pDevice);
	GenerateNoteTextures(pDevice);
	GenerateRainbowTextures(pDevice);
	SetCustomColors();
}

void ERMode::ReleaseTextures() {
	D3D::ReleaseTexture(&customStringColorTexture);
	D3D::ReleaseTexture(&customNoteColorTexture);
	for (auto& tex : rainbowTextures) {
		D3D::ReleaseTexture(&tex);
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="strings"> - Pointers to string colors</param>
/// <param name="colorType"> - String of color type</param>
void ERMode::SetColors(std::vector<uintptr_t> strings, const std::string& colorType) {
	for (int strIndex = 0; strIndex < 6;strIndex++) {
		if (strings[strIndex] == NULL)
			return;

		*(RSColor*)strings[strIndex] = customColors[strIndex][colorType];
	}
}

std::vector<RSColor> oldNormal, oldDisabled, oldEnabled, oldGlow, oldAmb;

/// <summary>
/// Run ResetString on all six strings.
/// </summary>
bool ERMode::ResetAllStrings() {
	if (!ColorsSaved)
		return false;

	for (int str = 0; str < 6;str++)
		ResetString(str);
	return true;
}

/// <summary>
/// Reset a string back to the original color
/// </summary>
/// <param name="strIndex"> - Current String (zero indexed)</param>
void ERMode::ResetString(int strIndex) {
	std::vector<uintptr_t> stringsGlow, stringsDisabled, stringsAmb, stringsEnabled, stringsPegInTune, stringsPegNotInTune, stringsText, stringsPart, stringsBodyNorm, stringsBodyAcc, stringsBodyPrev;

	// Get the original values for a string.
	InitStrings(stringsGlow, Glow);
	InitStrings(stringsDisabled, Disabled);
	//InitStrings(stringsAmb, Ambient);
	InitStrings(stringsEnabled, Enabled);

	// Set the origial values.
	*(RSColor*)stringsGlow[strIndex] = oldGlow[strIndex];
	*(RSColor*)stringsDisabled[strIndex] = oldDisabled[strIndex];
	//*(Color*)stringsAmb[strIndex] = oldAmb[strIndex];
	*(RSColor*)stringsEnabled[strIndex] = oldEnabled[strIndex];

	//Settings::SetStringColors(strIndex, oldGlow[strIndex], false);
}

std::vector<std::vector<RSColor>> defaultColors;

/// <summary>
/// Set colors of strings depending on INI settings.
/// </summary>
void ERMode::Toggle7StringMode() {
	std::vector<uintptr_t> stringsTest, stringsGlow, stringsDisabled, stringsAmb, stringsEnabled, stringsPegInTune, stringsPegNotInTune, pegsTuning, stringsText, stringsPart, stringsBodyNorm, stringsBodyAcc, stringsBodyPrev;

	// Get the original values for the strings.
	InitStrings(stringsGlow, Glow);
	InitStrings(stringsDisabled, Disabled);
	InitStrings(stringsAmb, Ambient);
	InitStrings(stringsEnabled, Enabled);
	InitStrings(stringsPegInTune, PegsInTune);
	InitStrings(stringsPegNotInTune, PegsNotInTune);
	InitStrings(stringsText, Text);
	//InitStrings(stringsPart, Particles);
	InitStrings(stringsBodyNorm, BodyNorm);
	InitStrings(stringsBodyAcc, BodyAcc);
	InitStrings(pegsTuning, PegsTuning);
	//InitStrings(stringsBodyPrev, BodyPrev);

	if (ColorsSaved && Settings::IsTwitchSettingEnabled(Setting::Twitch::SolidNotes)) {
		if (customSolidColor.size() != 6) // JIC
			return;

		// Don't change string colors with solid notes
		//SetColors(stringsGlow, customSolidColor);
		//SetColors(stringsEnabled, customSolidColor);
		//SetColors(stringsDisabled, customSolidColor);
		//SetColors(stringsAmb, customSolidColor);

		return;
	}

	if ((ERMode::AttemptedERInThisSong && ERMode::UseERExclusivelyInThisSong) || (AttemptedERInTuner && UseERInTuner)) {
		switch (Settings::GetStringColorMode()) {
			case StringColorMode::Default: // User wants original Rocksmith colors
				break;
			case StringColorMode::Zag: // User wants ZZ / Zag's colors (normal colors, but shifted down one string with a dark green on the top for Extended Range).
				SetColors(stringsGlow, colorsGlow); // Zags custom low B color values manually entered; Glowed
				SetColors(stringsDisabled, colorsDisabled); // Zags custom low B color values manually entered; Disabled
				SetColors(stringsEnabled, colorsStrEna); // name="GuitarStringsEnabledColorBlind" id="237528906"
				SetColors(stringsPegNotInTune, colorsPegNotInTune); // name="GuitarPegsTuningBlind" id="1806691030"
				SetColors(stringsPegInTune, colorsPegInTune); // name="GuitarPegsInTuneBlind" id="2547441015"
				SetColors(stringsText, colorsText); // name="GuitarRegistrarTextIndicatorBlind" id="3186002004"
				SetColors(stringsBodyNorm, colorsNormal); // name="NotewayBodyPartsBodyNormBlind" id="3629363565"
				SetColors(stringsBodyAcc, colorsBodyAcc); // name="NotewayBodyPartsAccentBlind" id = "47948252"
				SetColors(pegsTuning, colorsPegsTuning);
				break;
			case StringColorMode::Custom: // User wants their own custom (ER) colors
				SetColors(stringsEnabled, "Enabled_CB");
				SetColors(stringsGlow, "Glow_CB");
				SetColors(stringsDisabled, "Disabled_CB");
				SetColors(stringsText, "TextIndicator_CB");
				SetColors(pegsTuning, "PegsTuning_CB");
				SetColors(stringsPegInTune, "PegsInTune_CB");
				SetColors(stringsPegNotInTune, "PegsOutTune_CB");
				SetColors(stringsBodyNorm, "BodyNorm_CB");
				SetColors(stringsBodyAcc, "BodyAcc_CB");
				break;
			default:
				break;
		}
	}
	else {
		if (!ColorsSaved) { //read only once, so it won't change defaults if you change to CB
			for (int i = 0; i < 6; i++) {
				oldDisabled.push_back(*(RSColor*)stringsDisabled[i]);
				oldEnabled.push_back(*(RSColor*)stringsEnabled[i]);
				oldGlow.push_back(*(RSColor*)stringsGlow[i]);
				oldAmb.push_back(*(RSColor*)stringsAmb[i]);
			}

			ColorsSaved = true;
		}

		if (Settings::GetStringColorMode() == StringColorMode::Custom) { // User wants their own custom (non-ER) colors
			SetColors(stringsEnabled, "Enabled_N");
			SetColors(stringsGlow, "Glow_N");
			SetColors(stringsDisabled, "Disabled_N");
			SetColors(stringsText, "TextIndicator_N");
			SetColors(pegsTuning, "PegsTuning_N");
			SetColors(stringsPegInTune, "PegsInTune_N");
			SetColors(stringsPegNotInTune, "PegsOutTune_N");
			SetColors(stringsBodyNorm, "BodyNorm_N");
			SetColors(stringsBodyAcc, "BodyAcc_N");
			//etc.
		} // If not enabled, colors will auto-reset upon entering a song
	}

	//NOTE: this overrides string colors, no matter if ER song or not
	if (Settings::GetStringColorMode() == StringColorMode::Test) { // If you want the color testing menu to work
		if (saveDefaults) {
			defaultColors.clear();
			for (int idx = 0; idx < 17; idx++) {
				InitStrings(stringsTest, (idx * 0x18 + 0x350));

				std::vector<RSColor> defaults;
				for (int i = 0; i < 6; i++)
					defaults.push_back(*(RSColor*)stringsTest[i]);

				defaultColors.push_back(defaults);
			}

			saveDefaults = false;
		}

		// Restore previously saved defaults.
		if (restoreDefaults) {
			for (int idx = 0; idx < 17; idx++) {
				InitStrings(stringsTest, (idx * 0x18 + 0x350));

				for (int i = 0; i < 6; i++)
					*(RSColor*)stringsTest[i] = defaultColors[idx][i];
			}

			restoreDefaults = false;
		}
		else {
			stringsTest.clear();
			InitStrings(stringsTest, (currentOffsetIdx * 0x18 + 0x350));

			if (currColor == 0)
				SetColors(stringsTest, colorsBlack);
			else
				SetColors(stringsTest, colorsWhite);
		}

		InitStrings(stringsTest, Text);
		SetColors(stringsTest, colorsTest);
		LOG_INFO("Set custom colors" << std::endl);
	}
}

/// <summary>
/// Toggle RainbowEnabled on / off.
/// </summary>
void ERMode::ToggleRainbowMode() {
	RainbowEnabled = !RainbowEnabled.load();
}

/// <summary>
/// Toggle RainbowNotesEnabled on / off.
/// </summary>
void ERMode::ToggleRainbowNotes() {
	RainbowNotesEnabled = !RainbowNotesEnabled.load();
	D3DHooks::RecreateTextures = true;
}

/// <summary>
/// RainbowEnabled Middleware.
/// </summary>
bool ERMode::IsRainbowEnabled() {
	return RainbowEnabled;
}

/// <summary>
/// RainbowNotesEnabled Middleware.
/// </summary>
bool ERMode::IsRainbowNotesEnabled() {
	return RainbowNotesEnabled;
}

namespace {
	// The rainbow animation runs on this dedicated thread instead of the mod MainThread. 
	std::atomic<bool> g_rainbowWorkerRunning{ false };
	std::thread g_rainbowWorker;

	// Resolve the six colour addresses for one string state.
	// Returns false and leaves `out` untouched if any string fails to resolve.
	bool TryResolveStringColors(std::array<uintptr_t, 6>& out, int state) {
		std::array<uintptr_t, 6> resolved{};

		for (int i = 0; i < 6; i++) {
			resolved[i] = GetStringColor(i, state);

			if (!resolved[i]) return false;
		}

		out = resolved;
		return true;
	}
}

// The blocking animation loop. Reads game memory and cycles the string colours 
// until both rainbow flags are cleared, then returns.
static void RainbowWorker() {
	// Clear the running flag on every exit path, so DoRainbow can relaunch the worker 
	// next time the effect is enabled.
	struct RunningGuard {
		~RunningGuard() { g_rainbowWorkerRunning.store(false); }
	} runningGuard;

	// Re-resolved every frame rather than captured once up front.
	std::array<uintptr_t, 6> stringsEnabled{}, stringsHigh{}, stringsDisabled{};

	// Latched once, on the first frame that actually recolours the strings.
	std::array<RSColor, 6> oldEnabledColors{}, oldHigh{}, oldDisabledColors{};

	// Start with Red.
	RSColor c;
	c.r = 1.f;
	c.g = 0.f;
	c.b = 0.f;

	float h = 0.f;
	float speed = 2.f;
	float stringOffset = 20.f;
	bool didWeUseRainbowStrings = false; // If we don't use this, then the strings won't reset to default colors unless you end with rainbow strings.
	while (ERMode::RainbowEnabled || ERMode::RainbowNotesEnabled) {
		// Increment Hue by the speed we are trying to mimick.
		h += speed;

		// Hue can only be a value from 0-360, so reset it to 0 if it's over 360.
		if (h >= 360.f) { h = 0.f; }

		// Only the string writes need these addresses, so a frame we can't resolve still advances
		// the hue below - Rainbow Notes rides on customNoteColorH and keeps animating regardless.
		const bool stringsResolved =
			TryResolveStringColors(stringsEnabled, Enabled) &&
			TryResolveStringColors(stringsHigh, Glow) &&
			TryResolveStringColors(stringsDisabled, Disabled);

		const bool recolourStrings = ERMode::RainbowEnabled && stringsResolved;

		// Save the previous colors, once, on the first frame that actually recolours the strings.
		if (recolourStrings && !didWeUseRainbowStrings) {
			for (int i = 0; i < 6; i++) {
				oldEnabledColors[i] = *(RSColor*)stringsEnabled[i];
				oldHigh[i] = *(RSColor*)stringsHigh[i];
				oldDisabledColors[i] = *(RSColor*)stringsDisabled[i];
			}

			didWeUseRainbowStrings = true;
		}

		// For each string
		for (int i = 0; i < 6; i++) {
			int newH = h + (stringOffset * i);
			c.setH(newH);

			if (newH > 360)
				newH -= 360;

			// Since we only make textures for 180 possibilities, we can narrow out results down to 180 different values (0-179).
			if (newH > 4)
				ERMode::customNoteColorH = (newH / 2) - 1;
			else
				ERMode::customNoteColorH = 1;

			// Set the new rainbow colors.
			if (recolourStrings) {
				*(RSColor*)stringsEnabled[i] = c;
				*(RSColor*)stringsHigh[i] = c;
				*(RSColor*)stringsDisabled[i] = c;
			}
		}

		// Reset back to the original colors. If the strings can't be resolved 
		// right now the restore just waits for a frame where they can.
		if (!ERMode::RainbowEnabled && didWeUseRainbowStrings && stringsResolved) {
			for (int i = 0; i < 6; i++) {
				*(RSColor*)stringsEnabled[i] = oldEnabledColors[i];
				*(RSColor*)stringsHigh[i] = oldHigh[i];
				*(RSColor*)stringsDisabled[i] = oldDisabledColors[i];
			}
		}

		Sleep(16);
	}
}


// Launch the rainbow animation on its own thread if it isn't already running. Non-blocking: returns
// immediately so MainThread command dispatch keeps flowing and the effect can be toggled off again.
void ERMode::DoRainbow() {
	if (!RainbowEnabled && !RainbowNotesEnabled)
		return;

	if (g_rainbowWorkerRunning.exchange(true))
		return; // already animating

	if (g_rainbowWorker.joinable())
		g_rainbowWorker.join(); // reap the previous, already-finished worker before relaunching

	g_rainbowWorker = std::thread(RainbowWorker);
}

// Stop the rainbow worker and wait for it to finish. Called on mod teardown so the thread never
// outlives the DLL. Safe to call when no worker is running.
void ERMode::StopRainbowThread() {
	RainbowEnabled = false;
	RainbowNotesEnabled = false;

	if (g_rainbowWorker.joinable())
		g_rainbowWorker.join();
}


/* IN CASE THIS IS EVER NEEDED */

/// <summary>
/// Do these colors match
/// </summary>
/// <param name="strings"> - List of strings</param>
/// <param name="R"> - Red (Color to look for)</param>
/// <param name="G"> - Green (Color to look for)</param>
/// <param name="B"> - Blue (Color to look for)</param>
/// <returns></returns>
bool IsMatch(std::vector<uintptr_t> strings, int R, int G, int B) {
	if (strings[0] == NULL)
		return false;

	int r = (int)std::round((*(RSColor*)strings[0]).r * 255); // Remember, just casting to int doesn't round up :(
	int g = (int)std::round((*(RSColor*)strings[0]).g * 255);
	int b = (int)std::round((*(RSColor*)strings[0]).b * 255);
	if (R == 63)
		LOG_INFO(std::dec << R << " " << r << " " << (R == r) << " " << G << " " << g << " " << (G == g) << " " << B << " " << b << " " << (B == b) << std::endl);

	if (R == r && G == g && B == b)
		return true;
	return false;
}
