#nullable enable
using System;
using System.IO;
using RSMods.Core.Settings;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class SettingsCoordinatorTests
{
    [Fact]
    public void CoordinatorInstantiatesAllFieldsAndGroups()
    {
        var manifest = new ManifestService();
        var coordinator = new SettingsCoordinator(manifest);

        Assert.Equal(manifest.AllSettings.Count, coordinator.AllFields.Count);
        Assert.NotEmpty(coordinator.Groups);

        var volumeControl = coordinator.Find<BoolSettingFieldViewModel>("VolumeControlEnabled");
        Assert.NotNull(volumeControl);

        var volumeInterval = coordinator.Find<NumericSettingFieldViewModel>("VolumeControlInterval");
        Assert.NotNull(volumeInterval);
    }

    [Fact]
    public void ReactiveVisibilityGatingWorks()
    {
        var manifest = new ManifestService();
        var coordinator = new SettingsCoordinator(manifest);

        var volumeControl = coordinator.Find<BoolSettingFieldViewModel>("VolumeControlEnabled");
        var volumeInterval = coordinator.Find<NumericSettingFieldViewModel>("VolumeControlInterval");

        Assert.NotNull(volumeControl);
        Assert.NotNull(volumeInterval);

        // Initially volumeControl is false (default is "off"), so volumeInterval should be invisible
        volumeControl.Value = false;
        Assert.False(volumeInterval.IsVisible);

        // Turn on volume control -> interval should reactively become visible
        volumeControl.Value = true;
        Assert.True(volumeInterval.IsVisible);

        // Turn off again -> interval should become invisible
        volumeControl.Value = false;
        Assert.False(volumeInterval.IsVisible);
    }

    [Fact]
    public void ChoicesProviderPopulatesDynamicChoices()
    {
        var manifest = new ManifestService();
        var choicesProvider = new DefaultChoicesProvider();
        choicesProvider.Register("microphones", () => ["Mic 1", "Mic 2"]);

        var coordinator = new SettingsCoordinator(manifest, choicesProvider);
        var micField = coordinator.Find<EnumSettingFieldViewModel>("OverrideInputVolumeDevice");

        Assert.NotNull(micField);
        Assert.Contains("Mic 1", micField.Choices);
        Assert.Contains("Mic 2", micField.Choices);
    }

    [Fact]
    public void LoadAndSaveRoundtripViaIniManager()
    {
        string tempFile = Path.Combine(Path.GetTempPath(), $"rsmods_test_{Guid.NewGuid():N}.ini");
        try
        {
            File.WriteAllText(tempFile, """
                [Toggle Switches]
                VolumeControl=off
                SecondaryMonitor=on

                [Mod Settings]
                VolumeControlInterval=12
                SecondaryMonitorXPosition=1920
                """);

            var ini = new IniManager(tempFile);
            ini.Load();

            var manifest = new ManifestService();
            var coordinator = new SettingsCoordinator(manifest);
            coordinator.Load(ini);

            var volumeControl = coordinator.Find<BoolSettingFieldViewModel>("VolumeControlEnabled");
            var volumeInterval = coordinator.Find<NumericSettingFieldViewModel>("VolumeControlInterval");
            var secondaryMonitor = coordinator.Find<BoolSettingFieldViewModel>("SecondaryMonitor");
            var secondaryX = coordinator.Find<NumericSettingFieldViewModel>("SecondaryMonitorXPosition");

            Assert.NotNull(volumeControl);
            Assert.NotNull(volumeInterval);
            Assert.NotNull(secondaryMonitor);
            Assert.NotNull(secondaryX);

            Assert.False(volumeControl.Value);
            Assert.Equal(12m, volumeInterval.Value);
            Assert.True(secondaryMonitor.Value);
            Assert.Equal(1920m, secondaryX.Value);

            // Modify values
            volumeControl.Value = true;
            volumeInterval.Value = 15m;
            secondaryX.Value = 2560m;

            Assert.True(coordinator.IsDirty);

            // Save
            coordinator.Save(ini);
            ini.Save();

            // Re-read from disk with a fresh IniManager
            var reloadIni = new IniManager(tempFile);
            reloadIni.Load();

            Assert.True(reloadIni.GetBool("[Toggle Switches]", "VolumeControl"));
            Assert.Equal(15, reloadIni.GetInt("[Mod Settings]", "VolumeControlInterval"));
            Assert.Equal(2560, reloadIni.GetInt("[Mod Settings]", "SecondaryMonitorXPosition"));
        }
        finally
        {
            if (File.Exists(tempFile))
                File.Delete(tempFile);
        }
    }

    [Fact]
    public void ScaledAndRangedNumericSettingsBehaveCorrectly()
    {
        string tempFile = Path.Combine(Path.GetTempPath(), $"rsmods_test_scaled_{Guid.NewGuid():N}.ini");
        try
        {
            File.WriteAllText(tempFile, """
                [Mod Settings]
                CustomNSPTimeLimit=10000
                RewindBy=5000
                RRSpeedInterval=2
                """);

            var ini = new IniManager(tempFile);
            ini.Load();

            var manifest = new ManifestService();
            var coordinator = new SettingsCoordinator(manifest);
            coordinator.Load(ini);

            var nsp = coordinator.Find<NumericSettingFieldViewModel>("CustomNSPTimeLimit");
            Assert.NotNull(nsp);
            Assert.True(nsp.HasScale);
            Assert.Equal(2m, nsp.Minimum);
            Assert.Equal(60m, nsp.Maximum);
            Assert.Equal(0.25m, nsp.Increment);
            Assert.Equal("0.###", nsp.FormatString);
            Assert.Equal(10m, nsp.Value);

            // Change NSP to 15.5 seconds and save
            nsp.Value = 15.5m;
            coordinator.Save(ini);
            ini.Save();

            var reloadIni = new IniManager(tempFile);
            reloadIni.Load();
            Assert.Equal(15500, reloadIni.GetInt("[Mod Settings]", "CustomNSPTimeLimit"));

            // Check RR Speed Interval (unscaled with negative min)
            var rrSpeed = coordinator.Find<NumericSettingFieldViewModel>("RRSpeedInterval");
            Assert.NotNull(rrSpeed);
            Assert.False(rrSpeed.HasScale);
            Assert.Equal(-50m, rrSpeed.Minimum);
            Assert.Equal(50m, rrSpeed.Maximum);
            Assert.Equal(1m, rrSpeed.Increment);
            Assert.Equal("0", rrSpeed.FormatString);
            Assert.Equal(2m, rrSpeed.Value);
        }
        finally
        {
            if (File.Exists(tempFile))
                File.Delete(tempFile);
        }
    }

    [Fact]
    public void BoolSettingFieldViewModelDeterminesFormatFromSectionAndDisk()
    {
        string tempFile = Path.Combine(Path.GetTempPath(), $"rsmods_test_bool_{Guid.NewGuid():N}.ini");
        try
        {
            File.WriteAllText(tempFile, """
                [Toggle Switches]
                ToggleOne=on

                [Audio]
                ExclusiveMode=1

                [Custom Section]
                CustomNumeric=0
                CustomText=off
                """);

            var ini = new IniManager(tempFile);
            ini.Load();

            // 1. Toggle Switches: even if default were "1", [Toggle Switches] forces "on"/"off"
            var toggleDesc = new SettingDescriptor(
                "ToggleOne", new IniLocation("Toggle Switches", "ToggleOne"),
                SettingType.Bool, "1", "Toggle One", null, "Toggle Switches",
                null, null, null, null, null, null, null);
            var toggleVm = new BoolSettingFieldViewModel(toggleDesc);
            toggleVm.Load(ini);
            Assert.True(toggleVm.Value);
            toggleVm.Value = false;
            toggleVm.Save(ini);
            Assert.Equal("off", ini.GetString("[Toggle Switches]", "ToggleOne"));

            // 2. Audio section: native Rocksmith sections force numeric "1"/"0"
            var audioDesc = new SettingDescriptor(
                "ExclusiveMode", new IniLocation("Audio", "ExclusiveMode"),
                SettingType.Bool, "on", "Exclusive Mode", null, "Audio",
                null, null, null, null, null, null, null);
            var audioVm = new BoolSettingFieldViewModel(audioDesc);
            audioVm.Load(ini);
            Assert.True(audioVm.Value);
            audioVm.Value = false;
            audioVm.Save(ini);
            Assert.Equal("0", ini.GetString("[Audio]", "ExclusiveMode"));

            // 3. Disk format preservation: Custom Section preserving 0/1 vs on/off
            var customNumDesc = new SettingDescriptor(
                "CustomNumeric", new IniLocation("Custom Section", "CustomNumeric"),
                SettingType.Bool, "off", "Custom Numeric", null, "Custom Section",
                null, null, null, null, null, null, null);
            var customNumVm = new BoolSettingFieldViewModel(customNumDesc);
            customNumVm.Load(ini);
            customNumVm.Value = true;
            customNumVm.Save(ini);
            Assert.Equal("1", ini.GetString("[Custom Section]", "CustomNumeric"));

            var customTextDesc = new SettingDescriptor(
                "CustomText", new IniLocation("Custom Section", "CustomText"),
                SettingType.Bool, "1", "Custom Text", null, "Custom Section",
                null, null, null, null, null, null, null);
            var customTextVm = new BoolSettingFieldViewModel(customTextDesc);
            customTextVm.Load(ini);
            customTextVm.Value = true;
            customTextVm.Save(ini);
            Assert.Equal("on", ini.GetString("[Custom Section]", "CustomText"));
        }
        finally
        {
            if (File.Exists(tempFile))
                File.Delete(tempFile);
        }
    }

    [Fact]
    public void CustomEditorGenericDirtyTrackingAndLifecycle()
    {
        string tempFile = Path.Combine(Path.GetTempPath(), $"rsmods_test_custom_{Guid.NewGuid():N}.ini");
        try
        {
            var desc = new SettingDescriptor(
                "GuitarSpeak", new IniLocation("Guitar Speak", "GuitarSpeak"),
                SettingType.String, "", "Guitar Speak", null, "Guitar Speak",
                null, null, null, null, null, null, "GuitarSpeak");

            var manifest = new ManifestService();
            var coordinator = new SettingsCoordinator(manifest);

            var customEditor = coordinator.Find<CustomEditorFieldViewModel>("GuitarSpeakCustomEditor");
            Assert.NotNull(customEditor);
            Assert.False(customEditor.IsDirty);
            Assert.False(coordinator.IsDirty);

            // Marking dirty propagates to Coordinator.IsDirty
            customEditor.SetDirty(true);
            Assert.True(customEditor.IsDirty);
            Assert.True(coordinator.IsDirty);

            // Hook up a custom save handler
            bool saveHandlerCalled = false;
            customEditor.SaveHandler = ini =>
            {
                saveHandlerCalled = true;
                ini.SetString("[Guitar Speak]", "TestKey", "SavedValue");
            };

            var ini = new IniManager(tempFile);
            coordinator.Save(ini);

            Assert.True(saveHandlerCalled);
            Assert.False(customEditor.IsDirty);
            Assert.False(coordinator.IsDirty);
            Assert.Equal("SavedValue", ini.GetString("[Guitar Speak]", "TestKey"));
        }
        finally
        {
            if (File.Exists(tempFile))
                File.Delete(tempFile);
        }
    }
}

