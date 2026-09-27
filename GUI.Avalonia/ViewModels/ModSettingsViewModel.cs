#nullable enable
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Core.Settings;
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
/// Settings screen driven by the declarative settings schema manifest (<see cref="SettingsCoordinator"/>).
/// Holds editable field view models loaded from <see cref="RsModsSettings"/> and saves each change a moment
/// after it's made, as the WinForms configurator did, preserving round-trip comments and unknown entries.
/// Bespoke table editors for Guitar Speak and Mod/Audio keybindings are retained alongside the schema-driven fields.
/// </summary>
internal sealed partial class ModSettingsViewModel : ObservableObject
{
    private readonly SettingsService _settingsService;
    private readonly IManifestService _manifest;
    private readonly IDialogService _dialogs;
    private readonly INavigationService _navigation;
    private readonly DebouncedSaver _saver;
    private bool _loading;
    private bool _saving;

    public SettingsCoordinator Coordinator { get; }

    public IReadOnlyList<SettingGroupViewModel> SettingGroups => Coordinator.Groups;

    // --- On-screen text font preview ---
    // Shown whenever the font setting is.
    public bool ShowOnScreenFontPreview =>
        Coordinator.Find("OnScreenFont")?.IsVisible ?? false;

    public FontFamily OnScreenFontPreview
    {
        get
        {
            var fontName = Coordinator.Find<EnumSettingFieldViewModel>("OnScreenFont")?.SelectedValue;
            return string.IsNullOrWhiteSpace(fontName) ? FontFamily.Default : new FontFamily(fontName);
        }
    }

    // --- Secondary monitor helper ---
    public bool ShowSecondaryMonitor =>
        Coordinator.Find<BoolSettingFieldViewModel>("SecondaryMonitor")?.Value ?? false;

    public string SecondaryMonitorPositionText
    {
        get
        {
            var x = Coordinator.Find<NumericSettingFieldViewModel>("SecondaryMonitorXPosition")?.Value ?? 0;
            var y = Coordinator.Find<NumericSettingFieldViewModel>("SecondaryMonitorYPosition")?.Value ?? 0;
            return $"Start position: {x}, {y}";
        }
    }

    // --- Guitar Speak ---
    public bool ShowGuitarSpeak =>
        Coordinator.Find<BoolSettingFieldViewModel>("GuitarSpeak")?.Value ?? false;

    public ObservableCollection<GuitarSpeakRowViewModel> GuitarSpeakMappings { get; } = [];

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AssignGuitarSpeakCommand))]
    [NotifyCanExecuteChangedFor(nameof(ClearGuitarSpeakCommand))]
    private GuitarSpeakRowViewModel? _selectedGuitarSpeakMapping;

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
    public ObservableCollection<KeybindRowViewModel> ModKeybinds { get; } = [];
    public ObservableCollection<KeybindRowViewModel> AudioKeybinds { get; } = [];

    [ObservableProperty] private KeybindRowViewModel? _selectedModKeybind;
    [ObservableProperty] private KeybindRowViewModel? _selectedAudioKeybind;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    public ModSettingsViewModel(
        SettingsCoordinator coordinator,
        IManifestService manifest,
        SettingsService settings,
        IDialogService dialogs,
        INavigationService navigation,
        AutoSaveService autoSave)
    {
        Coordinator = coordinator;
        _manifest = manifest;
        _settingsService = settings;
        _dialogs = dialogs;
        _navigation = navigation;
        _saver = autoSave.Create(SaveAsync, ex => StatusMessage = $"Couldn't save: {ex.Message}");

        WireCustomEditors();

        Coordinator.StateChanged += (_, _) =>
        {
            if (_loading)
                return;

            RefreshAuxiliaryProperties();

            // Saving clears each field's dirty flag, which raises this too; only a real edit should save.
            if (!_saving && Coordinator.IsDirty)
                _saver.Request();
        };
    }

    private void WireCustomEditors()
    {
        foreach (var field in Coordinator.AllFields)
        {
            if (field is CustomEditorFieldViewModel customEditor)
            {
                customEditor.EditorRequested += OnCustomEditorRequested;
            }
        }
    }

    private async void OnCustomEditorRequested(string editorType)
    {
        switch (editorType)
        {
            case "HighwayColors":
            case "StringColors":
                await _navigation.NavigateToAsync(NavigationTarget.Colors, editorType);
                break;

            case "Twitch":
                await _navigation.NavigateToAsync(NavigationTarget.Twitch);
                break;

            case "GuitarSpeak":
                if (Coordinator.Find<BoolSettingFieldViewModel>("GuitarSpeak") is { } gsToggle && !gsToggle.Value)
                {
                    gsToggle.Value = true;
                }
                RefreshAuxiliaryProperties();
                StatusMessage = "Guitar Speak note mappings opened below.";
                break;

            case "Midi":
                await _dialogs.ShowInfoAsync(
                    "Configure MIDI input and auto-tune devices in the Mod Settings toggles and pickers above.",
                    "MIDI Setup");
                break;
        }
    }

    private void RefreshAuxiliaryProperties()
    {
        OnPropertyChanged(nameof(OnScreenFontPreview));
        OnPropertyChanged(nameof(ShowOnScreenFontPreview));
        OnPropertyChanged(nameof(ShowSecondaryMonitor));
        OnPropertyChanged(nameof(SecondaryMonitorPositionText));
        OnPropertyChanged(nameof(ShowGuitarSpeak));
    }

    /// <summary>Loads the editable snapshot from the settings store. Call after settings are loaded.</summary>
    public void Load()
    {
        _loading = true;
        try
        {
            if (RsModsSettings.Ini is { } ini)
                Coordinator.Load(ini);

            SelectedGuitarSpeakNote = null;
            SelectedGuitarSpeakOctave = null;
            SelectedGuitarSpeakMapping = null;
            LoadGuitarSpeakRows();

            SelectedModKeybind = null;
            SelectedAudioKeybind = null;
            if (RsModsSettings.Ini is { } keyIni)
            {
                LoadKeybindRows(ModKeybinds, ManifestKeybinds.Mod(_manifest, keyIni));
                LoadKeybindRows(AudioKeybinds, ManifestKeybinds.Audio(_manifest, keyIni));
            }

            RefreshAuxiliaryProperties();
        }
        finally
        {
            _loading = false;
            StatusMessage = string.Empty;
        }
    }

    // Runs on the UI thread via the debounced saver; the file write and the game ping happen off it.
    private async Task SaveAsync()
    {
        _saving = true;
        try
        {
            if (RsModsSettings.Ini is { } ini)
                Coordinator.Save(ini);

            // Rows write only when they changed, so they can't overwrite an edit made to the same key elsewhere.
            foreach (GuitarSpeakRowViewModel row in GuitarSpeakMappings)
                row.WriteBack();
            foreach (KeybindRowViewModel row in ModKeybinds)
                row.WriteBack();
            foreach (KeybindRowViewModel row in AudioKeybinds)
                row.WriteBack();
        }
        finally
        {
            _saving = false;
        }

        await _settingsService.SaveAsync();
        StatusMessage = $"Saved at {DateTime.Now:HH:mm:ss}";
    }

    [RelayCommand]
    private void CaptureSecondaryMonitorPosition(Window? window)
    {
        if (window is null)
            return;

        PixelPoint position = window.Position;
        if (Coordinator.Find<NumericSettingFieldViewModel>("SecondaryMonitorXPosition") is { } xField)
            xField.Value = position.X;
        if (Coordinator.Find<NumericSettingFieldViewModel>("SecondaryMonitorYPosition") is { } yField)
            yField.Value = position.Y;
    }

    [RelayCommand]
    private void ClearProfileSelection()
    {
        if (Coordinator.Find<EnumSettingFieldViewModel>("ProfileToLoad") is { } profileField)
            profileField.SelectedValue = "";
    }

    // --- Guitar Speak commands ---

    private bool CanAssignGuitarSpeak =>
        SelectedGuitarSpeakMapping is not null &&
        SelectedGuitarSpeakNote is not null &&
        SelectedGuitarSpeakOctave is not null;

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

    [RelayCommand(CanExecute = nameof(CanClearGuitarSpeak))]
    private void ClearGuitarSpeak()
    {
        if (SelectedGuitarSpeakMapping is not null)
            SelectedGuitarSpeakMapping.Value = string.Empty;
    }

    // --- Keybinding capture ---

    public Task CaptureModKeybindAsync(string frameworkKeyName, KeyCapturePhase phase) =>
        CaptureAsync(SelectedModKeybind, frameworkKeyName, phase);

    public Task CaptureAudioKeybindAsync(string frameworkKeyName, KeyCapturePhase phase) =>
        CaptureAsync(SelectedAudioKeybind, frameworkKeyName, phase);

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
            bool useAnyway = await _dialogs.ShowConfirmAsync(
                "That key is normally used by Rocksmith and may interfere with playing the game. Use it as a keybind anyway?",
                "Keybinding warning");
            if (!useAnyway)
                return;
        }

        row.Vkey = KeyConversion.VirtualKey(frameworkKeyName);
    }

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

    private void OnChildRowChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!_loading)
            _saver.Request();
    }
}
