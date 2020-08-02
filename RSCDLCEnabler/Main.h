// DLL Hijack. Do NOT remove this line!
#include "d3dx9_42.h"

// DLL Settings
#include "MemHelpers.h"
#include "Utils.h"
#include "Functions.h"
#include "detours.h"
#include "Offsets.h"
#include "MemUtil.h"
#include "Settings.h"

// Windows Libraries
#include <gdiplus.h>
#include <process.h>
#include <intrin.h>

// Mods
#include "Mods/Enumeration.h"
#include "D3D.h"
#include "Mods/CustomSongTitles.h"
#include "Mods/ExtendedRangeMode.h"
#include "Mods/CollectColors.h"
#include "Mods/GuitarSkeletons.h"
#include "Mods/GuitarSpeak.h"

// ImGUI
#include "imgui_includeMe.h"

#pragma intrinsic(_ReturnAddress)
#pragma comment (lib, "gdiplus.lib")
#pragma once

/// DLL Main Variables

// Console
const char* windowName = "Rocksmith 2014"; // Console Window Title

// ImGUI Settings
bool menuEnabled = false; // Do we show the user the ImGUI settings menu?
bool enableColorBlindCheckboxGUI = false; // Do we allow the user to change Colorblind mode in the imGUI menu?
std::vector<std::string> GuitarSpeakStartingTexts;

// Gdiplus (String Colors)
Gdiplus::GdiplusStartupInput inp;
Gdiplus::GdiplusStartupOutput outp;
ULONG_PTR token_;

// Mod Settings
bool resetHeadstockCache = true; // Do we want to reset the headstock cache? Triggers when opening tuning menu
bool toggleSkyline = false; // Do we want to toggle the skyline right now? Triggers to false when turned on/ off
int EnumSliderVal = 10000; // Sleep every X ms for enumeration (1000 ms = 1s)
bool LoftOff = false; // Is the loft disabled right now? Toggles when loft turns off (True - No Loft, False - Loft)
bool SkylineOff = false; // Is the skyline disabled right now? Toggles when skyline turns off (True - No Skyline, False - Skyline)
bool DrawSkylineInMenu = false; // If the user is in "Song" mode of Toggle Skyline, should we draw the skyline in this menu (True - Skyline, False - No Skyline)
bool GreenScreenWall = false; // If true, set the Greenscreen wall up. This helps call it in Lesson mode for No-Loft users (True - Black wall, False - Loft)
bool GuitarSpeakPresent = false; // If true, read the notes inputted and press the key combo provided. (True - On, False - Off)
bool RemoveLyrics = false; // If true, remove the lyrics from Learn A Song & Non-stop Play. (True - No Lyrics, False - Keep Lyrics)

// Menus
std::string previousMenu, currentMenu; // What is the last menu, and the current menu?
bool GameLoaded = false; // Has the game gotten to the main menu where you can pick the gamemodes?
bool LessonMode = false; // Is the user in LessonMode?

// Misc
bool lowPerformancePC = false; // Does your game lag with all of our mods? Toggle on to disable us running mods for an "original DLL" experience.
bool setAllToNoteGradientTexture = false; // Should we override the 6-string note textures with the 7-string note textures?

// Dev Functions
bool startLogging = false; // Should we log what's happening in Hook_DIP? Logs to log.txt in your RS2014 directory

/// End DLL Main Variables