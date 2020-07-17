#include "d3dx9_42.h"
#include "MemHelpers.h"
#include <gdiplus.h>
#include "Enumeration.h"
#include "detours.h"
#include "Functions.h"
#include "Utils.h"
#include "D3D.h"
#include "CustomSongTitles.h"
#include "Offsets.h"
#include "MemUtil.h"
#include "Settings.h"
#include "ExtendedRangeMode.h"
#include <process.h>

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_dx9.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/RobotoFont.cpp""

#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#pragma comment (lib, "gdiplus.lib")

#pragma once

/// Set Global Variables
	// Console
		const char* windowName = "Rocksmith 2014"; // Console Window Title
	// GUI Settings
		bool menuEnabled = false; // Do we show the user the imGUI settings menu?
		bool enableColorBlindCheckboxGUI = false; // Do we allow the user to change Colorblind mode in the imGUI menu?
	// Mod Settings
		bool resetHeadstockCache = true; // Do we want to reset the headstock cache? Triggers when opening tuning menu
		bool toggleSkyline = false; // Do we want to toggle the skyline right now? Triggers to false when turned on/ off
		int EnumSliderVal = 10000; // Sleep every X ms for enumeration
		bool LoftOff = false; // Is the loft disabled right now? Toggles when loft turns off (True - No Loft, False - Loft)
		bool SkylineOff = false; // Is the skyline disabled right now? Toggles when skyline turns off (True - No Skyline, False - Skyline)
		bool DrawSkylineInMenu = false; // If the user is in "Song" mode of Toggle Skyline, should we draw the skyline in this menu (True - Skyline, False - No Skyline)
	// Misc
		std::string previousMenu, currentMenu; // What is the last menu, and the current menu?
		bool GameLoaded = false; // Has the game gotten to the main menu where you can pick the gamemodes?
		bool lowPerformancePC = false; // Does your game lag with all of our mods? Toggle on to disable us running mods for an original DLL experience.
		bool setAllToNoteGradientTexture = false; // Should we override the 6-string note textures with the 7-string note textures?
		bool startLogging = false; // Should we log what's happening in Hook_DIP? Logs to log.txt in your RS2014 directory
	// Gdiplus
		Gdiplus::GdiplusStartupInput inp;
		Gdiplus::GdiplusStartupOutput outp;
		ULONG_PTR token_;
/// End Global Variable Section

//Sorry Ffio, I have a tendency of pressing CTRL+K, D a lot and that keeps on messing up your formatting, so I figured I would move this stuff out