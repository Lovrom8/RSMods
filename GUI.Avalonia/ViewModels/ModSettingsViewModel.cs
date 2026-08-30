using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Audio;
using RSMods.Core;
using RSMods.Services;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>How an input was captured, so the shared key policy can accept the right phase per key type.</summary>
internal enum KeyCapturePhase
{
    KeyDown,
    KeyUp,
    Mouse,
}

/// <summary>
/// First ported settings screen: a representative slice of the WinForms "Mod Settings" tab covering
/// toggle, enum, and numeric settings. Holds an editable snapshot loaded from <see cref="RsModsSettings"/>
/// and writes it back on save; ranges come from the shared <see cref="RsModsLimits"/> rather than XAML.
/// </summary>
internal sealed partial class ModSettingsViewModel(SettingsService settings, IDialogService dialogs, ProfileService profiles) : ObservableObject
{
    private bool _loading;

    // --- Toggles ---
    [ObservableProperty] private bool _toggleLoft;
    [ObservableProperty] private bool _showSongTimer;
    [ObservableProperty] private bool _rainbowStrings;
    [ObservableProperty] private bool _rainbowNotes;
    [ObservableProperty] private bool _fretless;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowSkylineWhen))]
    private bool _skyline;

    // --- Enums ---
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowReEnumerationInterval))]
    private EnumerationMode _forceReEnumeration;
    [ObservableProperty] private LoftMode _toggleLoftWhen;

    public static EnumerationMode[] EnumerationModes { get; } = (EnumerationMode[])Enum.GetValues(typeof(EnumerationMode));

    public static LoftMode[] LoftModes { get; } = (LoftMode[])Enum.GetValues(typeof(LoftMode));

    // --- Riff Repeater ---
    [ObservableProperty] private decimal _riffRepeaterSpeed;

    public static decimal RiffRepeaterSpeedMin => RsModsLimits.RiffRepeaterSpeedMin;
    public static decimal RiffRepeaterSpeedMax => RsModsLimits.RiffRepeaterSpeedMax;

    // --- On-screen text font ---
    // The font RSMods draws any in-game text with (e.g. the current-note display). The family list is
    // enumerated from Avalonia's system fonts; the size reuses the shared FontSize range.
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(OnScreenFontPreview))]
    private string? _onScreenFont;

    [ObservableProperty] private decimal _onScreenFontSize;

    public ObservableCollection<string> AvailableFonts { get; } = [];

    /// <summary>The font family used to render the live preview; falls back to Arial for a blank value.</summary>
    public FontFamily OnScreenFontPreview =>
        new(string.IsNullOrEmpty(OnScreenFont) ? "Arial" : OnScreenFont);

    public static decimal FontSizeMin => RsModsLimits.FontSizeMin;
    public static decimal FontSizeMax => RsModsLimits.FontSizeMax;

    // --- Playback & timing ---
    // These settings are stored in RSMods.ini as milliseconds but presented as seconds
    // (value / 1000 on load, value * 1000 on save). Each is gated by a toggle.
    [ObservableProperty] private decimal _reEnumerationIntervalSeconds;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowRewindSettings))]
    private bool _allowRewind;

    [ObservableProperty] private decimal _rewindBySeconds;
    [ObservableProperty] private decimal _rewindLeadupSeconds;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowLoopingLeadUp))]
    private bool _allowLooping;

    [ObservableProperty] private decimal _loopingLeadUpSeconds;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowNspTimer))]
    private bool _useCustomNspTimer;

    [ObservableProperty] private decimal _nspTimeLimitSeconds;

    // Gating: the numeric group only applies while its parent toggle is on.
    public bool ShowReEnumerationInterval => ForceReEnumeration != EnumerationMode.Off;
    public bool ShowRewindSettings => AllowRewind;
    public bool ShowLoopingLeadUp => AllowLooping;
    public bool ShowNspTimer => UseCustomNspTimer;

    public static decimal ReEnumerationIntervalMin => RsModsLimits.EnumerateIntervalSecondsMin;
    public static decimal ReEnumerationIntervalMax => RsModsLimits.EnumerateIntervalSecondsMax;
    public static decimal RewindByMin => RsModsLimits.RewindBySecondsMin;
    public static decimal RewindByMax => RsModsLimits.RewindBySecondsMax;
    public static decimal RewindLeadupMin => RsModsLimits.RewindLeadupSecondsMin;
    public static decimal RewindLeadupMax => RsModsLimits.RewindLeadupSecondsMax;
    public static decimal LoopingLeadUpMin => RsModsLimits.LoopingLeadUpSecondsMin;
    public static decimal LoopingLeadUpMax => RsModsLimits.LoopingLeadUpSecondsMax;
    public static decimal NspTimerMin => RsModsLimits.NspTimerSecondsMin;
    public static decimal NspTimerMax => RsModsLimits.NspTimerSecondsMax;

    // --- Visual toggles ---
    // Plain in-game show/hide toggles, plus the "remove X" toggles that reveal a "when" enum.
    [ObservableProperty] private bool _greenScreenWall;
    [ObservableProperty] private bool _removeInlays;
    [ObservableProperty] private bool _removeLaneMarkers;
    [ObservableProperty] private bool _screenShotScores;
    [ObservableProperty] private bool _showCurrentNoteOnScreen;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowHeadstockWhen))]
    private bool _removeHeadstock;

    [ObservableProperty] private HeadstockMode _removeHeadstockWhen;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowLyricsWhen))]
    private bool _removeLyrics;

    [ObservableProperty] private LyricsMode _removeLyricsWhen;

    [ObservableProperty] private SkylineMode _toggleSkylineWhen;

    // The "when" enum only applies while its parent removal toggle is on (mirrors WinForms group-box visibility).
    public bool ShowHeadstockWhen => RemoveHeadstock;
    public bool ShowLyricsWhen => RemoveLyrics;
    public bool ShowSkylineWhen => Skyline;

    public static HeadstockMode[] HeadstockModes { get; } = (HeadstockMode[])Enum.GetValues(typeof(HeadstockMode));

    public static LyricsMode[] LyricsModes { get; } = (LyricsMode[])Enum.GetValues(typeof(LyricsMode));

    public static SkylineMode[] SkylineModes { get; } = (SkylineMode[])Enum.GetValues(typeof(SkylineMode));

    // --- Audio ---
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowVolumeInterval))]
    private bool _controlGameVolume;

    [ObservableProperty] private decimal _volumeControlInterval;
    [ObservableProperty] private bool _allowAudioInBackground;
    [ObservableProperty] private bool _linearRiffRepeater;

    public bool ShowVolumeInterval => ControlGameVolume;
    public static decimal VolumeIntervalMin => RsModsLimits.VolumeIntervalMin;
    public static decimal VolumeIntervalMax => RsModsLimits.VolumeIntervalMax;

    // --- Fixes & misc ---
    [ObservableProperty] private bool _removeSongPreviews;
    [ObservableProperty] private bool _fixBassTuning;
    [ObservableProperty] private bool _fixOculusCrash;
    [ObservableProperty] private bool _preventBuggyTones;
    [ObservableProperty] private bool _showCurrentAccuracy;
    [ObservableProperty] private bool _preventPauseMidSong;
    [ObservableProperty] private bool _removeFingerprints;
    [ObservableProperty] private bool _bypassTwoRtcMessageBox;

    // --- Extended range ---
    // The extended-range tuning is stored as a negative semitone offset (index 0 = "D" = -2, down to
    // "Octave Down" = -12): value = -index - 2.
    public static string[] ExtendedRangeTunings { get; } =
        ["D", "C#", "C", "B", "A#", "A", "G#", "G", "F#", "F", "Octave Down"];

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowExtendedRangeDetails))]
    private bool _extendedRange;

    [ObservableProperty] private string _extendedRangeTuning = "B";
    [ObservableProperty] private bool _extendedRangeDropTuning;

    public bool ShowExtendedRangeDetails => ExtendedRange;

    // --- Secondary monitor ---
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowSecondaryMonitorPosition))]
    private bool _secondaryMonitor;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(SecondaryMonitorPositionText))]
    private int _secondaryMonitorX;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(SecondaryMonitorPositionText))]
    private int _secondaryMonitorY;

    public bool ShowSecondaryMonitorPosition => SecondaryMonitor;
    public string SecondaryMonitorPositionText => $"Start position: {SecondaryMonitorX}, {SecondaryMonitorY}";

    // --- Alternative output sample rate ---
    public static int[] OutputSampleRates { get; } =
        [44100, 48000, 88200, 96000, 176400, 192000];

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowSampleRate))]
    private bool _useAlternativeOutputSampleRate;

    [ObservableProperty] private int _alternativeOutputSampleRate;

    public bool ShowSampleRate => UseAlternativeOutputSampleRate;

    // --- Override input volume ---
    // Force a specific capture device's input volume. The device list is enumerated from the shared
    // core-audio source; the device and level only apply while the toggle is on.
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowOverrideInputVolume))]
    private bool _overrideInputVolumeEnabled;

    [ObservableProperty] private decimal _overrideInputVolume;
    [ObservableProperty] private string? _selectedInputDevice;

    public ObservableCollection<string> AvailableInputDevices { get; } = [];

    public bool ShowOverrideInputVolume => OverrideInputVolumeEnabled;

    public static decimal OverrideInputVolumeMin => RsModsLimits.OverrideInputVolumeMin;
    public static decimal OverrideInputVolumeMax => RsModsLimits.OverrideInputVolumeMax;

    // --- Profile auto-load ---
    // Optionally load one of the user's Rocksmith profiles on game boot. The list comes from the shared
    // profile service (the profiles found in the resolved save folder); an empty selection means "none".
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowProfileAutoLoad))]
    private bool _autoLoadProfile;

    [ObservableProperty] private string? _selectedProfile;

    public ObservableCollection<string> AvailableProfiles { get; } = [];

    public bool ShowProfileAutoLoad => AutoLoadProfile;

    // --- MIDI auto-tune & tuning pedal ---
    // If the player has a MIDI-controllable drop-tuning pedal, RSMods can drive it to keep them in the
    // song's tuning. The master toggle gates a MIDI-out device (the pedal), a target tuning offset, the
    // pedal model, an optional MIDI-in device, and when the auto-tune fires. Device lists come from the
    // shared winmm enumeration; a stored device/pedal that is no longer present is kept so a saved value
    // is never silently dropped.
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowAutoTuneDetails))]
    private bool _autoTuneForSong;

    [ObservableProperty] private string? _selectedMidiOutDevice;
    [ObservableProperty] private string? _selectedMidiInDevice;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowChordsMode))]
    private TuningPedalDevice _tuningPedal;

    [ObservableProperty] private bool _chordsMode;
    [ObservableProperty] private string _autoTuneOffset = AutoTuneOffsets[MidiTuningOffsetDefaultIndex];
    [ObservableProperty] private AutoTuneWhen _autoTuneForSongWhen;

    public ObservableCollection<string> AvailableMidiOutDevices { get; } = [];
    public ObservableCollection<string> AvailableMidiInDevices { get; } = [];

    public static TuningPedalDevice[] TuningPedalDevices { get; } = (TuningPedalDevice[])Enum.GetValues(typeof(TuningPedalDevice));

    public static AutoTuneWhen[] AutoTuneWhenModes { get; } = (AutoTuneWhen[])Enum.GetValues(typeof(AutoTuneWhen));

    /// <summary>
    /// The tuning each auto-tune offset targets, ordered from highest to lowest (offset = index - 3, so
    /// index 3 = offset 0 = the default "E Standard / Drop D"). Mirrors the WinForms offset list.
    /// </summary>
    public static string[] AutoTuneOffsets { get; } =
    [
        "G Standard / Drop F",
        "F# Standard / Drop E",
        "F Standard / Drop Eb",
        "E Standard / Drop D",
        "Eb Standard / Drop C#",
        "D Standard / Drop C",
        "C# Standard / Drop B",
        "C Standard / Drop A#",
        "B Standard / Drop A",
        "A# Standard / Drop G#",
        "A Standard / Drop G",
        "G# Standard / Drop F#",
        "G Standard / Drop F (Octave)",
        "F# Standard / Drop E (Octave)",
        "F Standard / Drop Eb (Octave)",
        "E Standard / Drop D (Octave)",
    ];

    private static int MidiTuningOffsetDefaultIndex => RsModsLimits.MidiOffsetToListIndex(RsModsLimits.MidiTuningOffsetDefault);

    public bool ShowAutoTuneDetails => AutoTuneForSong;
    public bool ShowChordsMode => TuningPedal is TuningPedalDevice.WhammyBass or TuningPedalDevice.WhammyFive;

    // --- Guitar Speak ---
    // Maps guitar notes to key presses so menus can be driven from the guitar. The master toggle gates a
    // "while tuning" toggle and the mapping editor: pick a note + octave, then assign it to the selected
    // key-press row. The 15 key-press rows are the shared Guitar Speak keybinds; each stores a MIDI
    // note number (note index + 36 + octave*12), matching the WinForms math.
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowGuitarSpeak))]
    private bool _guitarSpeakEnabled;

    [ObservableProperty] private bool _guitarSpeakWhileTuning;

    public bool ShowGuitarSpeak => GuitarSpeakEnabled;

    public ObservableCollection<GuitarSpeakRowViewModel> GuitarSpeakMappings { get; } = [];

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AssignGuitarSpeakCommand))]
    [NotifyCanExecuteChangedFor(nameof(ClearGuitarSpeakCommand))]
    private GuitarSpeakRowViewModel? _selectedGuitarSpeakMapping;

    // The note/octave pickers match the WinForms list order (note index 0..11, octave -1..6).
    public static string[] GuitarSpeakNotes { get; } =
        ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"];

    public static string[] GuitarSpeakOctaves { get; } =
        ["-1", "0", "1", "2", "3", "4", "5", "6"];

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AssignGuitarSpeakCommand))]
    private string? _selectedGuitarSpeakNote;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AssignGuitarSpeakCommand))]
    private string? _selectedGuitarSpeakOctave;

    // --- Keybindings ---
    // The shared mod and audio binds. Each row holds an in-memory virtual-key snapshot; the view's
    // capture box feeds framework key names into CaptureModKeybindAsync/CaptureAudioKeybindAsync, which
    // apply the shared Rocksmith key policy before assigning to the selected row.
    public ObservableCollection<KeybindRowViewModel> ModKeybinds { get; } = [];
    public ObservableCollection<KeybindRowViewModel> AudioKeybinds { get; } = [];

    [ObservableProperty] private KeybindRowViewModel? _selectedModKeybind;
    [ObservableProperty] private KeybindRowViewModel? _selectedAudioKeybind;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(SaveCommand))]
    [NotifyCanExecuteChangedFor(nameof(RevertCommand))]
    private bool _isDirty;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    /// <summary>Loads the editable snapshot from the settings store. Call after settings are loaded.</summary>
    public void Load()
    {
        _loading = true;
        try
        {
            ToggleLoft = RsModsSettings.Toggles.ToggleLoft;
            ShowSongTimer = RsModsSettings.Toggles.ShowSongTimer;
            RainbowStrings = RsModsSettings.Toggles.RainbowStrings;
            RainbowNotes = RsModsSettings.Toggles.RainbowNotes;
            Fretless = RsModsSettings.Toggles.Fretless;
            Skyline = RsModsSettings.Toggles.Skyline;

            ForceReEnumeration = RsModsSettings.Toggles.ForceReEnumeration;
            ToggleLoftWhen = RsModsSettings.Toggles.ToggleLoftWhen;

            RiffRepeaterSpeed = RsModsSettings.ModSettings.RRSpeedInterval;
            OnScreenFontSize = RsModsSettings.ModSettings.OnScreenFontSize;
            LoadFonts();
            OnScreenFont = RsModsSettings.Toggles.OnScreenFont;

            // Milliseconds in the store, seconds in the UI.
            ReEnumerationIntervalSeconds = RsModsSettings.ModSettings.CheckForNewSongsInterval / 1000m;
            AllowRewind = RsModsSettings.Toggles.AllowRewind;
            RewindBySeconds = RsModsSettings.ModSettings.RewindBy / 1000m;
            RewindLeadupSeconds = RsModsSettings.ModSettings.RewindLeadup / 1000m;
            AllowLooping = RsModsSettings.Toggles.AllowLooping;
            LoopingLeadUpSeconds = RsModsSettings.ModSettings.LoopingLeadUp / 1000m;
            UseCustomNspTimer = RsModsSettings.Toggles.UseCustomNSPTimer;
            NspTimeLimitSeconds = RsModsSettings.ModSettings.CustomNSPTimeLimit / 1000m;

            GreenScreenWall = RsModsSettings.Toggles.GreenScreenWall;
            RemoveInlays = RsModsSettings.Toggles.Inlays;
            RemoveLaneMarkers = RsModsSettings.Toggles.LaneMarkers;
            ScreenShotScores = RsModsSettings.Toggles.ScreenShotScores;
            ShowCurrentNoteOnScreen = RsModsSettings.Toggles.ShowCurrentNoteOnScreen;
            RemoveHeadstock = RsModsSettings.Toggles.Headstock;
            RemoveHeadstockWhen = RsModsSettings.Toggles.RemoveHeadstockWhen;
            RemoveLyrics = RsModsSettings.Toggles.Lyrics;
            RemoveLyricsWhen = RsModsSettings.Toggles.RemoveLyricsWhen;
            ToggleSkylineWhen = RsModsSettings.Toggles.ToggleSkylineWhen;

            ControlGameVolume = RsModsSettings.Toggles.VolumeControl;
            VolumeControlInterval = RsModsSettings.ModSettings.VolumeControlInterval;
            AllowAudioInBackground = RsModsSettings.Toggles.AllowAudioInBackground;
            LinearRiffRepeater = RsModsSettings.Toggles.LinearRiffRepeater;

            RemoveSongPreviews = RsModsSettings.Toggles.SongPreviews;
            FixBassTuning = RsModsSettings.Toggles.ExtendedRangeFixBassTuning;
            FixOculusCrash = RsModsSettings.Toggles.FixOculusCrash;
            PreventBuggyTones = RsModsSettings.Toggles.FixBrokenTones;
            ShowCurrentAccuracy = RsModsSettings.Toggles.DisplayCurrentAccuracy;
            PreventPauseMidSong = RsModsSettings.Toggles.PreventMidSongPause;
            RemoveFingerprints = RsModsSettings.Toggles.RemoveFingerprints;
            BypassTwoRtcMessageBox = RsModsSettings.Toggles.BypassTwoRTCMessageBox;

            ExtendedRange = RsModsSettings.Toggles.ExtendedRange;
            ExtendedRangeTuning = TuningFromOffset(RsModsSettings.ModSettings.ExtendedRangeModeAt);
            ExtendedRangeDropTuning = RsModsSettings.Toggles.ExtendedRangeDropTuning;

            SecondaryMonitor = RsModsSettings.Toggles.SecondaryMonitor == OnOffMode.On;
            SecondaryMonitorX = RsModsSettings.ModSettings.SecondaryMonitorXPosition;
            SecondaryMonitorY = RsModsSettings.ModSettings.SecondaryMonitorYPosition;

            UseAlternativeOutputSampleRate = RsModsSettings.Toggles.AltOutputSampleRate;
            AlternativeOutputSampleRate = RsModsSettings.ModSettings.AlternativeOutputSampleRate;

            OverrideInputVolumeEnabled = RsModsSettings.Toggles.OverrideInputVolumeEnabled;
            OverrideInputVolume = RsModsSettings.ModSettings.OverrideInputVolume;
            LoadInputDevices();
            string inputDevice = RsModsSettings.Toggles.OverrideInputVolumeDevice;
            SelectedInputDevice = string.IsNullOrEmpty(inputDevice) ? null : inputDevice;

            AutoLoadProfile = RsModsSettings.Toggles.ForceProfileLoad;
            LoadAvailableProfiles();
            string profileToLoad = RsModsSettings.Toggles.ProfileToLoad;
            SelectedProfile = string.IsNullOrEmpty(profileToLoad) ? null : profileToLoad;

            AutoTuneForSong = RsModsSettings.Toggles.AutoTuneForSong;
            LoadMidiDevices();
            string midiOut = RsModsSettings.Toggles.AutoTuneForSongDevice;
            SelectedMidiOutDevice = string.IsNullOrEmpty(midiOut) ? null : midiOut;
            string midiIn = RsModsSettings.Toggles.MidiInDevice;
            SelectedMidiInDevice = string.IsNullOrEmpty(midiIn) ? null : midiIn;
            TuningPedal = RsModsSettings.ModSettings.TuningPedal;
            ChordsMode = RsModsSettings.Toggles.ChordsMode;
            AutoTuneOffset = OffsetLabelFromValue(RsModsSettings.ModSettings.TuningOffset);
            AutoTuneForSongWhen = RsModsSettings.Toggles.AutoTuneForSongWhen;

            GuitarSpeakEnabled = RsModsSettings.Toggles.GuitarSpeak;
            GuitarSpeakWhileTuning = RsModsSettings.GuitarSpeak.GuitarSpeakWhileTuning;
            SelectedGuitarSpeakNote = null;
            SelectedGuitarSpeakOctave = null;
            SelectedGuitarSpeakMapping = null;
            LoadGuitarSpeakRows();

            SelectedModKeybind = null;
            SelectedAudioKeybind = null;
            LoadKeybindRows(ModKeybinds, Dictionaries.ModKeybinds);
            LoadKeybindRows(AudioKeybinds, Dictionaries.AudioKeybinds);
        }
        finally
        {
            _loading = false;
            IsDirty = false;
            StatusMessage = string.Empty;
        }
    }

    private bool CanSaveOrRevert => IsDirty;

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private async Task SaveAsync()
    {
        RsModsSettings.Toggles.ToggleLoft = ToggleLoft;
        RsModsSettings.Toggles.ShowSongTimer = ShowSongTimer;
        RsModsSettings.Toggles.RainbowStrings = RainbowStrings;
        RsModsSettings.Toggles.RainbowNotes = RainbowNotes;
        RsModsSettings.Toggles.Fretless = Fretless;
        RsModsSettings.Toggles.Skyline = Skyline;

        RsModsSettings.Toggles.ForceReEnumeration = ForceReEnumeration;
        RsModsSettings.Toggles.ToggleLoftWhen = ToggleLoftWhen;

        RsModsSettings.ModSettings.RRSpeedInterval = RiffRepeaterSpeed;
        RsModsSettings.ModSettings.OnScreenFontSize = (int)OnScreenFontSize;
        RsModsSettings.Toggles.OnScreenFont = string.IsNullOrEmpty(OnScreenFont) ? "Arial" : OnScreenFont;

        // Seconds back to the milliseconds the store expects.
        RsModsSettings.ModSettings.CheckForNewSongsInterval = (int)(ReEnumerationIntervalSeconds * 1000);
        RsModsSettings.Toggles.AllowRewind = AllowRewind;
        RsModsSettings.ModSettings.RewindBy = (int)(RewindBySeconds * 1000);
        RsModsSettings.ModSettings.RewindLeadup = (int)(RewindLeadupSeconds * 1000);
        RsModsSettings.Toggles.AllowLooping = AllowLooping;
        RsModsSettings.ModSettings.LoopingLeadUp = (int)(LoopingLeadUpSeconds * 1000);
        RsModsSettings.Toggles.UseCustomNSPTimer = UseCustomNspTimer;
        RsModsSettings.ModSettings.CustomNSPTimeLimit = (int)(NspTimeLimitSeconds * 1000);

        RsModsSettings.Toggles.GreenScreenWall = GreenScreenWall;
        RsModsSettings.Toggles.Inlays = RemoveInlays;
        RsModsSettings.Toggles.LaneMarkers = RemoveLaneMarkers;
        RsModsSettings.Toggles.ScreenShotScores = ScreenShotScores;
        RsModsSettings.Toggles.ShowCurrentNoteOnScreen = ShowCurrentNoteOnScreen;
        RsModsSettings.Toggles.Headstock = RemoveHeadstock;
        RsModsSettings.Toggles.RemoveHeadstockWhen = RemoveHeadstockWhen;
        RsModsSettings.Toggles.Lyrics = RemoveLyrics;
        RsModsSettings.Toggles.RemoveLyricsWhen = RemoveLyricsWhen;
        RsModsSettings.Toggles.ToggleSkylineWhen = ToggleSkylineWhen;

        RsModsSettings.Toggles.VolumeControl = ControlGameVolume;
        RsModsSettings.ModSettings.VolumeControlInterval = (int)VolumeControlInterval;
        RsModsSettings.Toggles.AllowAudioInBackground = AllowAudioInBackground;
        RsModsSettings.Toggles.LinearRiffRepeater = LinearRiffRepeater;

        RsModsSettings.Toggles.SongPreviews = RemoveSongPreviews;
        RsModsSettings.Toggles.ExtendedRangeFixBassTuning = FixBassTuning;
        RsModsSettings.Toggles.FixOculusCrash = FixOculusCrash;
        RsModsSettings.Toggles.FixBrokenTones = PreventBuggyTones;
        RsModsSettings.Toggles.DisplayCurrentAccuracy = ShowCurrentAccuracy;
        RsModsSettings.Toggles.PreventMidSongPause = PreventPauseMidSong;
        RsModsSettings.Toggles.RemoveFingerprints = RemoveFingerprints;
        RsModsSettings.Toggles.BypassTwoRTCMessageBox = BypassTwoRtcMessageBox;

        RsModsSettings.Toggles.ExtendedRange = ExtendedRange;
        RsModsSettings.ModSettings.ExtendedRangeModeAt = OffsetFromTuning(ExtendedRangeTuning);
        RsModsSettings.Toggles.ExtendedRangeDropTuning = ExtendedRangeDropTuning;

        RsModsSettings.Toggles.SecondaryMonitor = SecondaryMonitor ? OnOffMode.On : OnOffMode.Off;
        RsModsSettings.ModSettings.SecondaryMonitorXPosition = SecondaryMonitorX;
        RsModsSettings.ModSettings.SecondaryMonitorYPosition = SecondaryMonitorY;

        RsModsSettings.Toggles.AltOutputSampleRate = UseAlternativeOutputSampleRate;
        RsModsSettings.ModSettings.AlternativeOutputSampleRate = AlternativeOutputSampleRate;

        RsModsSettings.Toggles.OverrideInputVolumeEnabled = OverrideInputVolumeEnabled;
        RsModsSettings.ModSettings.OverrideInputVolume = (int)OverrideInputVolume;
        RsModsSettings.Toggles.OverrideInputVolumeDevice = SelectedInputDevice ?? string.Empty;

        RsModsSettings.Toggles.ForceProfileLoad = AutoLoadProfile;
        RsModsSettings.Toggles.ProfileToLoad = SelectedProfile ?? string.Empty;

        RsModsSettings.Toggles.AutoTuneForSong = AutoTuneForSong;
        RsModsSettings.Toggles.AutoTuneForSongDevice = SelectedMidiOutDevice ?? string.Empty;
        RsModsSettings.Toggles.MidiInDevice = SelectedMidiInDevice ?? string.Empty;
        RsModsSettings.ModSettings.TuningPedal = TuningPedal;
        RsModsSettings.Toggles.ChordsMode = ChordsMode;
        RsModsSettings.ModSettings.TuningOffset = OffsetValueFromLabel(AutoTuneOffset);
        RsModsSettings.Toggles.AutoTuneForSongWhen = AutoTuneForSongWhen;

        RsModsSettings.Toggles.GuitarSpeak = GuitarSpeakEnabled;
        RsModsSettings.GuitarSpeak.GuitarSpeakWhileTuning = GuitarSpeakWhileTuning;
        foreach (GuitarSpeakRowViewModel row in GuitarSpeakMappings)
            row.WriteBack();

        foreach (KeybindRowViewModel row in ModKeybinds)
            row.WriteBack();
        foreach (KeybindRowViewModel row in AudioKeybinds)
            row.WriteBack();

        await settings.SaveAsync();

        IsDirty = false;
        StatusMessage = "Settings saved.";
    }

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private void Revert()
    {
        Load();
        StatusMessage = "Reverted to the last saved values.";
    }

    /// <summary>
    /// Captures the configurator window's current on-screen position as the secondary-monitor start
    /// position, mirroring the WinForms "set start position" button: drag the window to the target
    /// monitor, then click. The window is passed in from the view so the view-model stays window-agnostic.
    /// </summary>
    [RelayCommand]
    private void CaptureSecondaryMonitorPosition(Window? window)
    {
        if (window is null)
            return;

        PixelPoint position = window.Position;
        SecondaryMonitorX = position.X;
        SecondaryMonitorY = position.Y;
    }

    /// <summary>Clears the auto-load profile selection (equivalent to "no profile"), mirroring the WinForms clear button.</summary>
    [RelayCommand]
    private void ClearProfileSelection() => SelectedProfile = null;

    /// <summary>
    /// Rebuilds the on-screen font list from Avalonia's system fonts. Keeps a stored font that is not
    /// currently installed so a saved font name is never silently dropped.
    /// </summary>
    private void LoadFonts()
    {
        AvailableFonts.Clear();

        foreach (string fontName in FontManager.Current.SystemFonts.Select(family => family.Name).Distinct().OrderBy(name => name))
            AvailableFonts.Add(fontName);

        string stored = RsModsSettings.Toggles.OnScreenFont;
        if (!string.IsNullOrEmpty(stored) && !AvailableFonts.Contains(stored))
            AvailableFonts.Add(stored);
    }

    /// <summary>
    /// Rebuilds the capture-device list from the shared core-audio enumeration (best-effort; an empty
    /// list on failure). Keeps a stored device that is not currently present so a saved device name is
    /// never silently dropped.
    /// </summary>
    private void LoadInputDevices()
    {
        AvailableInputDevices.Clear();

        foreach (string deviceName in InputDevices.FriendlyNames())
            AvailableInputDevices.Add(deviceName);

        string stored = RsModsSettings.Toggles.OverrideInputVolumeDevice;
        if (!string.IsNullOrEmpty(stored) && !AvailableInputDevices.Contains(stored))
            AvailableInputDevices.Add(stored);
    }

    /// <summary>
    /// Rebuilds the available-profile list from the shared profile service (best-effort; an empty or
    /// unreadable save folder yields an empty list). Keeps a stored selection that is not currently
    /// present so a saved profile name is never silently dropped.
    /// </summary>
    private void LoadAvailableProfiles()
    {
        AvailableProfiles.Clear();

        try
        {
            foreach (string profileName in profiles.GetAvailableProfiles().Keys)
                AvailableProfiles.Add(profileName);
        }
        catch
        {
            // The profiles could not be read; leave the list empty rather than failing the whole screen.
        }

        string stored = RsModsSettings.Toggles.ProfileToLoad;
        if (!string.IsNullOrEmpty(stored) && !AvailableProfiles.Contains(stored))
            AvailableProfiles.Add(stored);
    }

    /// <summary>
    /// Rebuilds the MIDI out/in device lists from the shared winmm enumeration (best-effort; an empty
    /// list on failure). Keeps a stored device that is not currently present so a saved device name is
    /// never silently dropped.
    /// </summary>
    private void LoadMidiDevices()
    {
        AvailableMidiOutDevices.Clear();
        foreach (string deviceName in MidiDevices.OutputDeviceNames())
            AvailableMidiOutDevices.Add(deviceName);

        string storedOut = RsModsSettings.Toggles.AutoTuneForSongDevice;
        if (!string.IsNullOrEmpty(storedOut) && !AvailableMidiOutDevices.Contains(storedOut))
            AvailableMidiOutDevices.Add(storedOut);

        AvailableMidiInDevices.Clear();
        foreach (string deviceName in MidiDevices.InputDeviceNames())
            AvailableMidiInDevices.Add(deviceName);

        string storedIn = RsModsSettings.Toggles.MidiInDevice;
        if (!string.IsNullOrEmpty(storedIn) && !AvailableMidiInDevices.Contains(storedIn))
            AvailableMidiInDevices.Add(storedIn);
    }

    // --- Guitar Speak commands ---

    private bool CanAssignGuitarSpeak =>
        SelectedGuitarSpeakMapping is not null &&
        SelectedGuitarSpeakNote is not null &&
        SelectedGuitarSpeakOctave is not null;

    /// <summary>
    /// Assigns the picked note + octave to the selected key-press row, storing the MIDI note number
    /// (note index + 36, offset by (octave index - 3) * 12) exactly as the WinForms screen did.
    /// </summary>
    [RelayCommand(CanExecute = nameof(CanAssignGuitarSpeak))]
    private void AssignGuitarSpeak()
    {
        if (SelectedGuitarSpeakMapping is null || SelectedGuitarSpeakNote is null || SelectedGuitarSpeakOctave is null)
            return;

        int noteIndex = Array.IndexOf(GuitarSpeakNotes, SelectedGuitarSpeakNote);
        int octaveIndex = Array.IndexOf(GuitarSpeakOctaves, SelectedGuitarSpeakOctave);
        if (noteIndex < 0 || octaveIndex < 0)
            return;

        int inputNote = noteIndex + 36;
        int inputOctave = octaveIndex - 3;
        int outputNoteOctave = inputNote + (inputOctave * 12);

        SelectedGuitarSpeakMapping.Value = outputNoteOctave.ToString();
    }

    private bool CanClearGuitarSpeak => SelectedGuitarSpeakMapping is not null;

    /// <summary>Clears the mapping on the selected key-press row (equivalent to the WinForms clear button).</summary>
    [RelayCommand(CanExecute = nameof(CanClearGuitarSpeak))]
    private void ClearGuitarSpeak()
    {
        if (SelectedGuitarSpeakMapping is not null)
            SelectedGuitarSpeakMapping.Value = string.Empty;
    }

    // --- Keybinding capture ---

    /// <summary>Applies a captured input to the selected mod bind (called by the view's capture box).</summary>
    public Task CaptureModKeybindAsync(string frameworkKeyName, KeyCapturePhase phase) =>
        CaptureAsync(SelectedModKeybind, frameworkKeyName, phase);

    /// <summary>Applies a captured input to the selected audio bind (called by the view's capture box).</summary>
    public Task CaptureAudioKeybindAsync(string frameworkKeyName, KeyCapturePhase phase) =>
        CaptureAsync(SelectedAudioKeybind, frameworkKeyName, phase);

    /// <summary>
    /// Classifies a framework input name through the shared Rocksmith key policy and, if it is accepted
    /// for the given capture phase, assigns it to <paramref name="row"/>. Reserved keys (letters/digits
    /// Rocksmith itself uses) prompt for confirmation first, mirroring the WinForms warning.
    /// </summary>
    private async Task CaptureAsync(KeybindRowViewModel? row, string frameworkKeyName, KeyCapturePhase phase)
    {
        if (row is null)
            return;

        RocksmithInputClassification classification = RocksmithKeys.Classify(frameworkKeyName);

        bool accepted = phase switch
        {
            KeyCapturePhase.KeyDown => classification is RocksmithInputClassification.KeyDown or RocksmithInputClassification.Reserved,
            KeyCapturePhase.KeyUp => classification is RocksmithInputClassification.KeyUp,
            KeyCapturePhase.Mouse => classification is RocksmithInputClassification.MouseButton,
            _ => false,
        };

        if (!accepted)
            return;

        if (classification == RocksmithInputClassification.Reserved)
        {
            bool useAnyway = await dialogs.ShowConfirmAsync(
                "That key is normally used by Rocksmith and may interfere with playing the game. Use it as a keybind anyway?",
                "Keybinding warning");
            if (!useAnyway)
                return;
        }

        row.Vkey = KeyConversion.VirtualKey(frameworkKeyName);
    }

    /// <summary>Rebuilds a keybind collection from the shared dictionary, wiring per-row dirty tracking.</summary>
    private void LoadKeybindRows(ObservableCollection<KeybindRowViewModel> target, System.Collections.Generic.IReadOnlyList<KeybindItem> source)
    {
        foreach (KeybindRowViewModel existing in target)
            existing.PropertyChanged -= OnChildRowChanged;
        target.Clear();

        foreach (KeybindItem item in source)
        {
            var row = new KeybindRowViewModel(item);
            row.PropertyChanged += OnChildRowChanged;
            target.Add(row);
        }
    }

    /// <summary>Rebuilds the Guitar Speak mapping rows from the shared dictionary, wiring dirty tracking.</summary>
    private void LoadGuitarSpeakRows()
    {
        foreach (GuitarSpeakRowViewModel existing in GuitarSpeakMappings)
            existing.PropertyChanged -= OnChildRowChanged;
        GuitarSpeakMappings.Clear();

        foreach (KeybindItem item in Dictionaries.GuitarSpeakKeybinds)
        {
            var row = new GuitarSpeakRowViewModel(item);
            row.PropertyChanged += OnChildRowChanged;
            GuitarSpeakMappings.Add(row);
        }
    }

    /// <summary>A child row's edited value marks the whole snapshot dirty, like any other setting change.</summary>
    private void OnChildRowChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!_loading)
            IsDirty = true;
    }

    protected override void OnPropertyChanged(PropertyChangedEventArgs e)
    {
        base.OnPropertyChanged(e);

        if (_loading)
            return;

        // Ignore bookkeeping and transient selection state; only real setting edits mark the snapshot dirty.
        if (e.PropertyName is nameof(IsDirty) or nameof(StatusMessage)
            or nameof(SelectedModKeybind) or nameof(SelectedAudioKeybind)
            or nameof(SelectedGuitarSpeakMapping) or nameof(SelectedGuitarSpeakNote) or nameof(SelectedGuitarSpeakOctave))
            return;

        IsDirty = true;
    }

    /// <summary>Maps a stored semitone offset (-2..-12) to its tuning name; clamps out-of-range values.</summary>
    private static string TuningFromOffset(int offset)
    {
        int index = Math.Clamp(-offset - 2, 0, ExtendedRangeTunings.Length - 1);
        return ExtendedRangeTunings[index];
    }

    /// <summary>Maps a tuning name back to its stored semitone offset, falling back to the default.</summary>
    private static int OffsetFromTuning(string tuning)
    {
        int index = Array.IndexOf(ExtendedRangeTunings, tuning);
        return index < 0 ? RsModsLimits.ExtendedRangeTuningDefault : -index - 2;
    }

    /// <summary>Maps a stored auto-tune offset (-3..12) to its label; clamps out-of-range values.</summary>
    private static string OffsetLabelFromValue(int offset)
    {
        int index = Math.Clamp(RsModsLimits.MidiOffsetToListIndex(offset), 0, AutoTuneOffsets.Length - 1);
        return AutoTuneOffsets[index];
    }

    /// <summary>Maps an auto-tune offset label back to its stored value (offset = index - 3), defaulting when unknown.</summary>
    private static int OffsetValueFromLabel(string label)
    {
        int index = Array.IndexOf(AutoTuneOffsets, label);
        return index < 0 ? RsModsLimits.MidiTuningOffsetDefault : index - 3;
    }
}
