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
/// Settings screen driven by the declarative settings schema manifest (<see cref="SettingsCoordinator"/>): a list
/// of mods, and the selected mod's settings, key binds and helpers beside it. Holds editable field view models
/// loaded from <see cref="RsModsSettings"/> and saves each change a moment after it's made, as the WinForms
/// configurator did, preserving round-trip comments and unknown entries.
/// </summary>
internal sealed partial class ModSettingsViewModel : ObservableObject
{
    private readonly SettingsService _settingsService;
    private readonly IDialogService _dialogs;
    private readonly INavigationService _navigation;
    private readonly DebouncedSaver _saver;
    private bool _loading;
    private bool _saving;

    public SettingsCoordinator Coordinator { get; }

    // --- Mod list ---
    private readonly List<ModItemViewModel> _allMods = [];

    /// <summary>The mods matching <see cref="Filter"/>, sorted by name.</summary>
    public ObservableCollection<ModItemViewModel> Mods { get; } = [];

    [ObservableProperty]
    private ModItemViewModel? _selectedMod;

    [ObservableProperty]
    private string _filter = string.Empty;

    // --- On-screen text font preview ---
    public FontFamily OnScreenFontPreview
    {
        get
        {
            var fontName = Coordinator.Find<EnumSettingFieldViewModel>("OnScreenFont")?.SelectedValue;
            return string.IsNullOrWhiteSpace(fontName) ? FontFamily.Default : new FontFamily(fontName);
        }
    }

    public double OnScreenFontSizePreview =>
        Math.Clamp((double)(Coordinator.Find<NumericSettingFieldViewModel>("OnScreenFontSize")?.Value ?? 16), 8, 80);

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
    private string _statusMessage = string.Empty;

    public ModSettingsViewModel(
        SettingsCoordinator coordinator,
        SettingsService settings,
        IDialogService dialogs,
        INavigationService navigation,
        AutoSaveService autoSave)
    {
        Coordinator = coordinator;
        _settingsService = settings;
        _dialogs = dialogs;
        _navigation = navigation;
        _saver = autoSave.Create(SaveAsync, ex => StatusMessage = $"Couldn't save: {ex.Message}");

        WireCustomEditors();

        foreach (ModEntryViewModel entry in Coordinator.Mods)
            _allMods.Add(new ModItemViewModel(entry, []));
        ApplyFilter();

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
        }
    }

    partial void OnFilterChanged(string value) => ApplyFilter();

    private void ApplyFilter()
    {
        ModItemViewModel? selected = SelectedMod;

        Mods.Clear();
        foreach (ModItemViewModel mod in _allMods.Where(m => m.Matches(Filter)))
            Mods.Add(mod);

        // Clearing the list drops the selection; keep it when the mod still shows, else show the first match.
        SelectedMod = selected is not null && Mods.Contains(selected) ? selected : Mods.FirstOrDefault();
    }

    private void RefreshAuxiliaryProperties()
    {
        OnPropertyChanged(nameof(OnScreenFontPreview));
        OnPropertyChanged(nameof(OnScreenFontSizePreview));
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

            LoadGuitarSpeakRows();

            if (RsModsSettings.Ini is { } keyIni)
            {
                foreach (ModItemViewModel mod in _allMods)
                    LoadKeybindRows(mod.Keybinds, mod.Entry.Keybinds.Select(d => ManifestKeybinds.Create(d, keyIni)));
            }

            RefreshAuxiliaryProperties();
        }
        finally
        {
            _loading = false;
            StatusMessage = string.Empty;
        }

        // A field that loaded a non-canonical value (e.g. a toggle stored as 1) is dirty; write the fix straight away.
        if (Coordinator.IsDirty)
            _saver.Request();
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
            foreach (KeybindRowViewModel row in _allMods.SelectMany(m => m.Keybinds))
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

    // --- Keybinding capture ---

    /// <summary>
    /// Offers a captured key to a row waiting for one. Returns false when the key isn't accepted in this phase,
    /// so the row keeps waiting.
    /// </summary>
    public async Task<bool> CaptureKeybindAsync(KeybindRowViewModel row, string frameworkKeyName, KeyCapturePhase phase)
    {
        RocksmithInputClassification classification = RocksmithKeys.Classify(frameworkKeyName);

        bool accepted = phase switch
        {
            KeyCapturePhase.KeyDown => classification is RocksmithInputClassification.KeyDown or RocksmithInputClassification.Reserved,
            KeyCapturePhase.KeyUp => classification is RocksmithInputClassification.KeyUp,
            KeyCapturePhase.Mouse => classification is RocksmithInputClassification.MouseButton,
            _ => false,
        };

        if (!accepted)
            return false;

        // Done waiting before the prompt, so the key that answers it isn't captured too.
        row.IsCapturing = false;

        if (classification == RocksmithInputClassification.Reserved)
        {
            bool useAnyway = await _dialogs.ShowConfirmAsync(
                "That key is normally used by Rocksmith and may interfere with playing the game. Use it as a keybind anyway?",
                "Keybinding warning");
            if (!useAnyway)
                return true;
        }

        row.Vkey = KeyConversion.VirtualKey(frameworkKeyName);
        return true;
    }

    private void LoadKeybindRows(ObservableCollection<KeybindRowViewModel> target, IEnumerable<KeybindItem> source)
    {
        foreach (KeybindRowViewModel existing in target)
            existing.PropertyChanged -= OnKeybindRowChanged;
        target.Clear();

        foreach (KeybindItem item in source)
        {
            var row = new KeybindRowViewModel(item);
            row.PropertyChanged += OnKeybindRowChanged;
            target.Add(row);
        }
    }

    private void OnKeybindRowChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!_loading && e.PropertyName == nameof(KeybindRowViewModel.Vkey))
            _saver.Request();
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
        if (!_loading && e.PropertyName == nameof(GuitarSpeakRowViewModel.Value))
            _saver.Request();
    }
}
