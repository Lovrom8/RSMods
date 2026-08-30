using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Services;

namespace RSMods.ViewModels;

/// <summary>
/// The Custom Colors screen: the string, note, and highway colours that live in the shared
/// <see cref="RsModsSettings"/> store. Each palette keeps both a normal and a colour-blind set (the game
/// picks between them), so both are held in the snapshot and only the cells the user changes are written back.
/// </summary>
internal sealed partial class ColorsViewModel : ObservableObject
{
    private readonly SettingsService _settings;
    private bool _loading;
    private bool _initialized;

    // Low E through high e; the store keys colours by string index 0..5 in this order.
    private static readonly string[] StringLabels = ["E (low)", "A", "D", "G", "B", "e (high)"];

    public ColorSwatchViewModel[] StringColorsNormal { get; }
    public ColorSwatchViewModel[] StringColorsColorblind { get; }
    public ColorSwatchViewModel[] NoteColorsNormal { get; }
    public ColorSwatchViewModel[] NoteColorsColorblind { get; }

    public ColorSwatchViewModel HighwayNumbered { get; } = new("Numbered frets");
    public ColorSwatchViewModel HighwayUnNumbered { get; } = new("Un-numbered frets");
    public ColorSwatchViewModel HighwayGutter { get; } = new("Noteway sides");
    public ColorSwatchViewModel HighwayFretNumbers { get; } = new("Fret numbers");
    public ColorSwatchViewModel[] HighwaySwatches { get; }

    // --- String colours ---
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowStringColors))]
    private bool _useCustomStringColors;

    [ObservableProperty] private bool _stringColorblindPalette;

    // --- Note colours ---
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowNoteColors), nameof(ShowNoteSwatches))]
    private bool _useSeparateNoteColors;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowNoteSwatches))]
    private bool _useRocksmithNoteColors;

    [ObservableProperty] private bool _noteColorblindPalette;

    // --- Highway colours ---
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowHighwayColors))]
    private bool _useCustomHighwayColors;

    public bool ShowStringColors => UseCustomStringColors;
    public bool ShowNoteColors => UseSeparateNoteColors;
    // Rocksmith's own note colours mean the custom swatches don't apply, so hide them.
    public bool ShowNoteSwatches => UseSeparateNoteColors && !UseRocksmithNoteColors;
    public bool ShowHighwayColors => UseCustomHighwayColors;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(SaveCommand), nameof(RevertCommand))]
    private bool _isDirty;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    public ColorsViewModel(SettingsService settings)
    {
        _settings = settings;

        StringColorsNormal = BuildRow();
        StringColorsColorblind = BuildRow();
        NoteColorsNormal = BuildRow();
        NoteColorsColorblind = BuildRow();
        HighwaySwatches = [HighwayNumbered, HighwayUnNumbered, HighwayGutter, HighwayFretNumbers];

        foreach (ColorSwatchViewModel swatch in AllSwatches())
            swatch.PropertyChanged += OnSwatchChanged;
    }

    private static ColorSwatchViewModel[] BuildRow() => StringLabels.Select(label => new ColorSwatchViewModel(label)).ToArray();

    private IEnumerable<ColorSwatchViewModel> AllSwatches() =>
        StringColorsNormal
            .Concat(StringColorsColorblind)
            .Concat(NoteColorsNormal)
            .Concat(NoteColorsColorblind)
            .Concat(HighwaySwatches);

    /// <summary>
    /// Loads the snapshot on first navigation. The store is already populated by startup, so this only
    /// reads it; it is idempotent, mirroring the other lazily-loaded settings screens.
    /// </summary>
    public Task InitializeAsync()
    {
        if (_initialized)
            return Task.CompletedTask;
        _initialized = true;

        Load();
        return Task.CompletedTask;
    }

    private void Load()
    {
        _loading = true;
        try
        {
            UseCustomStringColors = RsModsSettings.Toggles.CustomStringColors != CustomStringColorMode.Off;
            UseSeparateNoteColors = RsModsSettings.Toggles.SeparateNoteColors == OnOffMode.On;
            UseRocksmithNoteColors = RsModsSettings.ModSettings.SeparateNoteColorsMode == NoteColorMode.RocksmithColors;
            UseCustomHighwayColors = RsModsSettings.HighwayColors.CustomHighwayColors;

            // Palette pickers are view-only state; always start on the normal palette.
            StringColorblindPalette = false;
            NoteColorblindPalette = false;

            for (int i = 0; i < StringLabels.Length; i++)
            {
                StringColorsNormal[i].Set(RsModsSettings.StringColors.GetStringColor(i, normal: true));
                StringColorsColorblind[i].Set(RsModsSettings.StringColors.GetStringColor(i, normal: false));
                NoteColorsNormal[i].Set(RsModsSettings.StringColors.GetNoteColor(i, normal: true));
                NoteColorsColorblind[i].Set(RsModsSettings.StringColors.GetNoteColor(i, normal: false));
            }

            HighwayNumbered.Set(RsModsSettings.HighwayColors.CustomHighwayNumbered);
            HighwayUnNumbered.Set(RsModsSettings.HighwayColors.CustomHighwayUnNumbered);
            HighwayGutter.Set(RsModsSettings.HighwayColors.CustomHighwayGutter);
            HighwayFretNumbers.Set(RsModsSettings.HighwayColors.CustomFretNubmers);
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
        RsModsSettings.Toggles.CustomStringColors =
            UseCustomStringColors ? CustomStringColorMode.Custom : CustomStringColorMode.Off;
        RsModsSettings.Toggles.SeparateNoteColors =
            UseSeparateNoteColors ? OnOffMode.On : OnOffMode.Off;
        RsModsSettings.ModSettings.SeparateNoteColorsMode = !UseSeparateNoteColors
            ? NoteColorMode.Off
            : UseRocksmithNoteColors ? NoteColorMode.RocksmithColors : NoteColorMode.Custom;
        RsModsSettings.HighwayColors.CustomHighwayColors = UseCustomHighwayColors;

        // Write only the swatches the user changed so untouched defaults aren't baked into the INI.
        for (int i = 0; i < StringLabels.Length; i++)
        {
            if (StringColorsNormal[i].Changed)
                RsModsSettings.StringColors.SetStringColor(i, normal: true, Normalize(StringColorsNormal[i].Hex));
            if (StringColorsColorblind[i].Changed)
                RsModsSettings.StringColors.SetStringColor(i, normal: false, Normalize(StringColorsColorblind[i].Hex));
            if (NoteColorsNormal[i].Changed)
                RsModsSettings.StringColors.SetNoteColor(i, normal: true, Normalize(NoteColorsNormal[i].Hex));
            if (NoteColorsColorblind[i].Changed)
                RsModsSettings.StringColors.SetNoteColor(i, normal: false, Normalize(NoteColorsColorblind[i].Hex));
        }

        if (HighwayNumbered.Changed)
            RsModsSettings.HighwayColors.CustomHighwayNumbered = Normalize(HighwayNumbered.Hex);
        if (HighwayUnNumbered.Changed)
            RsModsSettings.HighwayColors.CustomHighwayUnNumbered = Normalize(HighwayUnNumbered.Hex);
        if (HighwayGutter.Changed)
            RsModsSettings.HighwayColors.CustomHighwayGutter = Normalize(HighwayGutter.Hex);
        if (HighwayFretNumbers.Changed)
            RsModsSettings.HighwayColors.CustomFretNubmers = Normalize(HighwayFretNumbers.Hex);

        await _settings.SaveAsync();

        foreach (ColorSwatchViewModel swatch in AllSwatches())
            swatch.Commit();

        IsDirty = false;
        StatusMessage = "Settings saved.";
    }

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private void Revert()
    {
        Load();
        StatusMessage = "Reverted to the last saved values.";
    }

    private void OnSwatchChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_loading)
            return;

        if (e.PropertyName == nameof(ColorSwatchViewModel.Hex))
            IsDirty = true;
    }

    protected override void OnPropertyChanged(PropertyChangedEventArgs e)
    {
        base.OnPropertyChanged(e);

        if (_loading)
            return;

        // Only the four data toggles mark the page dirty. The palette pickers and computed visibility
        // flags are presentation state, and the bookkeeping properties are not persisted.
        switch (e.PropertyName)
        {
            case nameof(IsDirty):
            case nameof(StatusMessage):
            case nameof(StringColorblindPalette):
            case nameof(NoteColorblindPalette):
            case nameof(ShowStringColors):
            case nameof(ShowNoteColors):
            case nameof(ShowNoteSwatches):
            case nameof(ShowHighwayColors):
                return;
        }

        IsDirty = true;
    }

    /// <summary>Stores colours as the store expects: 6 upper-case hex digits with no leading #.</summary>
    private static string Normalize(string? hex)
    {
        string trimmed = (hex ?? string.Empty).Trim();
        if (trimmed.StartsWith('#'))
            trimmed = trimmed[1..];
        return trimmed.ToUpperInvariant();
    }
}
