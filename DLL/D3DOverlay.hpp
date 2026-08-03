#pragma once

#include "stdafx.h"
#include "Mods/GuitarSpeak.hpp"
#include "Mods/RiffRepeater.hpp"
#include "Mods/Midi.hpp"
#include <atlbase.h>
#include <algorithm>
#include "Keybindings.hpp"
#include "NoteData.h"
#include "GameState.hpp"

namespace GameOverlay {
	Resolution GetWindowSize();
	void DX9DrawText(const std::string& textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize = { NULL, NULL }, DWORD format = DT_LEFT | DT_NOCLIP);

	inline HRESULT CustomDX9Font = NULL;
	inline ID3DXFont* DX9FontEncapsulation = NULL;
	inline int fontWidth = NULL, fontHeight = NULL;

	const int whiteText = 0xFFFFFFFF;
	const int greyText = 0xFF9A9A9A;

	// Drop pedal state colours: green shifting down, amber shifting up, white at the
	// player's own tuning, grey when off.
	const int dropPedalDownText = 0xFF6BE06B;
	const int dropPedalUpText = 0xFFFFC24D;
	const int dropPedalDisabledText = 0xFFC8C8C8;

	inline Resolution WindowSize;
	inline IDirect3DDevice9* pDevice;
	void SetPDevice(IDirect3DDevice9* pDevice, Resolution windowSize);
	void DisplayMixer();
	void DisplaySongTimer();
	void DisplayCurrentNote();
	void DisplayRiffRepeaterOverHundredPercentSpeed();
	void DisplayCurrentTuningForAutoTune();
	void DisplayDropPedalTuning();
	void DisplayDropPedalEngine();
	void DisplayLoopStartEndTimes(float loopStart, float loopEnd);
	void DisplaySongAccuracy();
	void CheckCurrentFont();
	void RenderOverlay(IDirect3DDevice9* pDevice);

	const inline std::vector<std::string> mixerInternalNames = { // Needs to be char* as that's what SetRTPCValue needs.
		{"Master_Volume"}, // Master Volume
		{"Mixer_Music"}, // Song Volume
		{"Mixer_Player1"}, // Player 1 Guitar & Bass (both are handled with this singular name)
		{"Mixer_Player2"}, // Player 2 Guitar & Bass (both are handled with this singular name)
		{"Mixer_Mic"}, // My Microphone Volume
		{"Mixer_VO"}, // Rocksmith Dad Voice Over
		{"Mixer_SFX"}, // Menu SFX Volume
	};

	const inline std::vector<std::string> drawMixerTextName = {
		{"Master Volume: "},
		{"Song Volume: "},
		{"Player 1 Volume: "},
		{"Player 2 Volume: "},
		{"Microphone Volume: "},
		{"Voice-Over Volume: "},
		{"SFX Volume: "},
	};

	inline unsigned int currentVolumeIndex = 0; // Mixer volume to change. 0 - Master, 1 - Song, 2 - P1, 3 - P2, 4 - Mic, 5 - VO, 6 - SFX

	// Volume adjustment mod
	inline bool displayMixer = false;
	inline bool displayCurrentVolume = false;
	inline auto displayVolumeStartTime = std::chrono::steady_clock::time_point(); // Defaults to epoch time

	inline static std::string cachedFontName = "";
	inline static int cachedFontSize = 0;
	inline static CComPtr<ID3DXFont> cachedFont;

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
		bool Get(IDirect3DDevice9* dev, const FontKey& key, CComPtr<ID3DXFont>& out) {
			out.Release(); 
			if (auto it = cache.find(key); it != cache.end() && it->second) {
				out = it->second;
				return true;
			}

			CComPtr<ID3DXFont> font;
			HRESULT hr = D3DXCreateFontA(dev, key.height, 0, key.weight, 1, key.italic,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, key.face.c_str(), &font);
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
		std::unordered_map<FontKey, CComPtr<ID3DXFont>, FontKeyHash> cache;
	};

	inline static FontCache fontCache;
}
