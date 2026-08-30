using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using RSMods.Core;

namespace RSMods.ViewModels;

/// <summary>
/// One RS_ASIO input channel (Input.0, Input.1, or Input.Mic). All three share the same shape, so the
/// parent maps each one to/from its store section. "Enabled" plus a driver selection stands in for the
/// store's disable convention (blank or commented driver), which the parent applies on save.
/// </summary>
internal sealed partial class AsioInputViewModel(string title, ObservableCollection<string> availableDrivers) : ObservableObject
{
    public string Title { get; } = title;
    public ObservableCollection<string> AvailableDrivers { get; } = availableDrivers;

    [ObservableProperty] private bool _enabled;
    [ObservableProperty] private string? _driver;
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
