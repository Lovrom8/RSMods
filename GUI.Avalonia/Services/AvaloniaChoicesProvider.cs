#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using Avalonia.Media;
using RSMods.Audio;
using RSMods.Core.Settings;

namespace RSMods.Services;

/// <summary>
/// Avalonia runtime implementation of IChoicesProvider connecting manifest sources
/// to audio devices, MIDI devices, Rocksmith profiles, and system fonts.
/// </summary>
internal sealed class AvaloniaChoicesProvider(ProfileService profiles) : IChoicesProvider
{
    public IReadOnlyList<string> GetChoices(string choicesSource)
    {
        try
        {
            if (choicesSource.Equals("microphones", StringComparison.OrdinalIgnoreCase))
                return InputDevices.FriendlyNames().ToList();

            if (choicesSource.Equals("MidiOutDevices", StringComparison.OrdinalIgnoreCase))
                return MidiDevices.OutputDeviceNames().ToList();

            if (choicesSource.Equals("MidiInDevices", StringComparison.OrdinalIgnoreCase))
                return MidiDevices.InputDeviceNames().ToList();

            if (choicesSource.Equals("RocksmithProfiles", StringComparison.OrdinalIgnoreCase))
                return profiles.GetAvailableProfiles().Keys.ToList();

            if (choicesSource.Equals("SystemFonts", StringComparison.OrdinalIgnoreCase))
                return FontManager.Current?.SystemFonts.Select(f => f.Name).Distinct().OrderBy(n => n).ToList() ?? [];
        }
        catch
        {
            // Device enumeration failed; return empty list gracefully
        }

        return [];
    }
}
