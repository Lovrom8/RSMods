using RSMods.Util;
using System.Collections.Generic;
using static RSMods.RsModsSettings;

namespace RSMods
{

    static class Dictionaries
    {
        public static readonly List<KeybindItem> ModKeybinds =
        [
            new("Toggle Loft",          k => Keybinds.ToggleLoftKey = k, () => Keybinds.ToggleLoftKey),
            new("Show Song Timer",      k => Keybinds.ShowSongTimerKey = k, () => Keybinds.ShowSongTimerKey),
            new("Force Re-Enumeration", k => Keybinds.ForceReEnumerationKey = k, () => Keybinds.ForceReEnumerationKey),
            new("Rainbow Strings",      k => Keybinds.RainbowStringsKey = k, () => Keybinds.RainbowStringsKey),
            new("Rainbow Notes",        k => Keybinds.RainbowNotesKey = k, () => Keybinds.RainbowNotesKey),
            new("Remove Lyrics",        k => Keybinds.RemoveLyricsKey = k, () => Keybinds.RemoveLyricsKey),
            new("RR Speed Change",        k => Keybinds.RRSpeedKey = k, () => Keybinds.RRSpeedKey),
            new("Change Tuning Offset",        k => Keybinds.TuningOffsetKey = k, () => Keybinds.TuningOffsetKey),
            new("Toggle Extended Range",        k => Keybinds.ToggleExtendedRangeKey = k, () => Keybinds.ToggleExtendedRangeKey),
            new("Start Loop",        k => Keybinds.LoopStartKey = k, () => Keybinds.LoopStartKey),
            new("End Loop",        k => Keybinds.LoopEndKey = k, () => Keybinds.LoopEndKey),
            new("Rewind Song",        k => Keybinds.RewindKey = k, () => Keybinds.RewindKey)
        ];

        public static readonly List<KeybindItem> AudioKeybinds =
        [
            new("Master Volume", k => AudioKeybindings.MasterVolumeKey = k, () => AudioKeybindings.MasterVolumeKey),
            new("Song Volume", k => AudioKeybindings.SongVolumeKey = k, () => AudioKeybindings.SongVolumeKey),
            new("Player 1 Volume", k => AudioKeybindings.Player1VolumeKey = k, () => AudioKeybindings.Player1VolumeKey),
            new("Player 2 Volume", k => AudioKeybindings.Player2VolumeKey = k, () => AudioKeybindings.Player2VolumeKey),
            new("Microphone Volume", k => AudioKeybindings.MicrophoneVolumeKey = k, () => AudioKeybindings.MicrophoneVolumeKey),
            new("Voice-Over Volume", k => AudioKeybindings.VoiceOverVolumeKey = k, () => AudioKeybindings.VoiceOverVolumeKey),
            new("SFX Volume", k => AudioKeybindings.SFXVolumeKey = k, () => AudioKeybindings.SFXVolumeKey),
            new("Display Mixer", k => AudioKeybindings.DisplayMixerKey = k, () => AudioKeybindings.DisplayMixerKey),
            new("Mute / Unmute Player 1", k => AudioKeybindings.MutePlayer1Key = k, () => AudioKeybindings.MutePlayer1Key),
            new("Mute / Unmute Player 2", k => AudioKeybindings.MutePlayer2Key = k, () => AudioKeybindings.MutePlayer2Key)
        ];

        public static readonly List<KeybindItem> GuitarSpeakKeybinds =
        [
            new("Delete", v => RsModsSettings.GuitarSpeak.GuitarSpeakDeleteWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakDeleteWhen),
            new("Space", v => RsModsSettings.GuitarSpeak.GuitarSpeakSpaceWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakSpaceWhen),
            new("Enter", v => RsModsSettings.GuitarSpeak.GuitarSpeakEnterWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakEnterWhen),
            new("Tab", v => RsModsSettings.GuitarSpeak.GuitarSpeakTabWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakTabWhen),
            new("Page Up", v => RsModsSettings.GuitarSpeak.GuitarSpeakPGUPWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakPGUPWhen),
            new("Page Down", v => RsModsSettings.GuitarSpeak.GuitarSpeakPGDNWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakPGDNWhen),
            new("Up Arrow", v => RsModsSettings.GuitarSpeak.GuitarSpeakUPWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakUPWhen),
            new("Down Arrow", v => RsModsSettings.GuitarSpeak.GuitarSpeakDNWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakDNWhen),
            new("Escape", v => RsModsSettings.GuitarSpeak.GuitarSpeakESCWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakESCWhen),
            new("Open Bracket", v => RsModsSettings.GuitarSpeak.GuitarSpeakOBracketWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakOBracketWhen),
            new("Close Bracket", v => RsModsSettings.GuitarSpeak.GuitarSpeakCBracketWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakCBracketWhen),
            new("Tilde / Tilda", v => RsModsSettings.GuitarSpeak.GuitarSpeakTildeaWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakTildeaWhen),
            new("Forward Slash", v => RsModsSettings.GuitarSpeak.GuitarSpeakForSlashWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakForSlashWhen),
            new("Alt", v => RsModsSettings.GuitarSpeak.GuitarSpeakAltWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakAltWhen),
            new("Close Guitar Speak", v => RsModsSettings.GuitarSpeak.GuitarSpeakCloseWhen = v, () => RsModsSettings.GuitarSpeak.GuitarSpeakCloseWhen)
        ];

        public static readonly Dictionary<string, string> ResultVoiceOverDictionary = new()
        {
            { "Bad Performance", "play_VO_RESULTSSCREEN2_001_15NARRATOR_DRY_26312" },
            { "Disappointing Performance", "play_VO_RESULTSSCREEN2_001_13NARRATOR_DRY_26310" },
            { "Subpar Performance", "play_VO_RESULTSSCREEN2_001_17NARRATOR_DRY_26314" },
            { "Could Be Better", "play_VO_RESULTSSCREEN2_001_22NARRATOR_DRY_26319" },
            { "Decent Performance", "play_VO_RESULTSSCREEN_001_31NARRATOR_DRY_26288" },
            { "Alright Performance", "play_VO_RESULTSSCREEN_001_04NARRATOR_DRY_26328" },
            { "Excellent Performance", "play_VO_RESULTSSCREEN_001_22NARRATOR_DRY_26344" },
            { "Top Notch Performance", "play_VO_RESULTSSCREEN2_001_24NARRATOR_DRY_26322" },
            { "Superb Performance", "play_VO_RESULTSSCREEN2_001_30NARRATOR_DRY_26352" },
            { "Dazzling Performance", "play_VO_RESULTSSCREEN_001_13NARRATOR_DRY_26337" },
            { "You\'re Gonna Be A Superstar", "play_VO_RESULTSSCREEN_001_39NARRATOR_DRY_26296" },
            { "Wonderful Performance", "play_VO_RESULTSSCREEN_001_19NARRATOR_DRY_26341" },
            { "Exceptional Performance", "play_VO_RESULTSSCREEN_001_23NARRATOR_DRY_26345" },
            { "Amazing Performance", "play_VO_RESULTSSCREEN2_001_29NARRATOR_DRY_26351" },
            { "Exemplary Performance", "play_VO_RESULTSSCREEN_001_26NARRATOR_DRY_26348" },
            { "(98%) Masterful Performance", "play_VO_RESULTSSCREEN2_001_27NARRATOR_DRY_26324" },
            { "(99%) Masterful Performance", "play_VO_RESULTSSCREEN_001_20NARRATOR_DRY_26342" },
            { "Flawless Performance", "play_VO_RESULTSSCREEN_001_12NARRATOR_DRY_26336" }
        };

        public static readonly List<ColorItem> NotewayColors =
        [
            new("Change Numbered Frets",   v => HighwayColors.CustomHighwayNumbered = v, () => HighwayColors.CustomHighwayNumbered),
            new("Change UnNumbered Frets", v => HighwayColors.CustomHighwayUnNumbered = v, () => HighwayColors.CustomHighwayUnNumbered),
            new("Change Noteway Sides",    v => HighwayColors.CustomHighwayGutter = v, () => HighwayColors.CustomHighwayGutter),
            new("Change Fret Number",      v => HighwayColors.CustomFretNubmers = v, () => HighwayColors.CustomFretNubmers)
        ];
    }
}
