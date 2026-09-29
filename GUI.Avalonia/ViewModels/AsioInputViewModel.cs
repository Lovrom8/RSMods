using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using RSMods.Core;

namespace RSMods.ViewModels;

/// <summary>
/// One RS_ASIO input channel (Input.0, Input.1, or Input.Mic). All three share the same shape, so the
/// parent maps each one to/from its store section. "Enabled" plus a driver selection stands in for the
/// store's disable convention (blank or commented driver), which the parent applies on save.
/// </summary>
internal sealed partial class AsioInputViewModel(string title, string hint, ObservableCollection<string> availableDrivers,
    Func<string?, bool> isInstalled) : ObservableObject
{
    public string Title { get; } = title;
    public string Hint { get; } = hint;
    public ObservableCollection<string> AvailableDrivers { get; } = availableDrivers;

    [ObservableProperty] private bool _enabled;
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(DriverNotInstalled))]
    private string? _driver;

    /// <summary>The saved driver isn't installed, so it's only listed to keep the saved value visible.</summary>
    public bool DriverNotInstalled => !string.IsNullOrEmpty(Driver) && !isInstalled(Driver);

    /// <summary>Re-evaluates <see cref="DriverNotInstalled"/> after the installed drivers were re-read.</summary>
    public void RefreshInstalled() => OnPropertyChanged(nameof(DriverNotInstalled));
    [ObservableProperty] private decimal _channel;
    [ObservableProperty] private bool _enableEndpointVolume;
    [ObservableProperty] private bool _enableMasterVolume;
    [ObservableProperty] private decimal _masterVolumePercent;
    [ObservableProperty] private bool _refCountHack;

    public static decimal ChannelMin => RsAsioLimits.ChannelMin;
    public static decimal ChannelMax => RsAsioLimits.ChannelMax;
    public static decimal VolumePercentMin => RsAsioLimits.VolumePercentMin;
    public static decimal VolumePercentMax => RsAsioLimits.VolumePercentMax;
}
