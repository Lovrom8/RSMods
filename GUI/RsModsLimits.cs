using System.Windows.Forms;
using CoreLimits = RSMods.Core.RsModsLimits;

namespace RSMods
{
    /// <summary>
    /// WinForms-facing view over the shared <see cref="CoreLimits"/> ranges. The numeric ranges and
    /// defaults now live in GUI.Core so the Avalonia frontend clamps identically; this class
    /// re-exports them so existing WinForms callers stay unchanged and adds the WinForms-only control
    /// wiring in <see cref="ApplyToUiControls"/>.
    /// </summary>
    public static class RsModsLimits
    {
        public const int VolumeIntervalMin = CoreLimits.VolumeIntervalMin;
        public const int VolumeIntervalMax = CoreLimits.VolumeIntervalMax;
        public const int VolumeIntervalDefault = CoreLimits.VolumeIntervalDefault;

        public const int RiffRepeaterSpeedMin = CoreLimits.RiffRepeaterSpeedMin;
        public const int RiffRepeaterSpeedMax = CoreLimits.RiffRepeaterSpeedMax;
        public const int RiffRepeaterSpeedDefault = CoreLimits.RiffRepeaterSpeedDefault;

        public const int NumberOfBackupsMin = CoreLimits.NumberOfBackupsMin;
        public const int NumberOfBackupsMax = CoreLimits.NumberOfBackupsMax;
        public const int NumberOfBackupsDefault = CoreLimits.NumberOfBackupsDefault;

        public const int OverrideInputVolumeMin = CoreLimits.OverrideInputVolumeMin;
        public const int OverrideInputVolumeMax = CoreLimits.OverrideInputVolumeMax;
        public const int OverrideInputVolumeDefault = CoreLimits.OverrideInputVolumeDefault;

        public const int FontSizeMin = CoreLimits.FontSizeMin;
        public const int FontSizeMax = CoreLimits.FontSizeMax;
        public const int FontSizeDefault = CoreLimits.FontSizeDefault;

        public const int ExtendedRangeTuningMin = CoreLimits.ExtendedRangeTuningMin;
        public const int ExtendedRangeTuningMax = CoreLimits.ExtendedRangeTuningMax;
        public const int ExtendedRangeTuningDefault = CoreLimits.ExtendedRangeTuningDefault;

        public const int MidiTuningOffsetMin = CoreLimits.MidiTuningOffsetMin;
        public const int MidiTuningOffsetMax = CoreLimits.MidiTuningOffsetMax;
        public const int MidiTuningOffsetDefault = CoreLimits.MidiTuningOffsetDefault;

        public const int EnumerateIntervalMsDefault = CoreLimits.EnumerateIntervalMsDefault;
        public const decimal EnumerateIntervalSecondsMin = CoreLimits.EnumerateIntervalSecondsMin;
        public const decimal EnumerateIntervalSecondsMax = CoreLimits.EnumerateIntervalSecondsMax;
        public const decimal EnumerateIntervalSecondsDefault = CoreLimits.EnumerateIntervalSecondsDefault;

        public const int LoopingLeadUpMsDefault = CoreLimits.LoopingLeadUpMsDefault;
        public const decimal LoopingLeadUpSecondsMin = CoreLimits.LoopingLeadUpSecondsMin;
        public const decimal LoopingLeadUpSecondsMax = CoreLimits.LoopingLeadUpSecondsMax;
        public const decimal LoopingLeadUpSecondsDefault = CoreLimits.LoopingLeadUpSecondsDefault;

        public const int RewindByMsDefault = CoreLimits.RewindByMsDefault;
        public const decimal RewindBySecondsMin = CoreLimits.RewindBySecondsMin;
        public const decimal RewindBySecondsMax = CoreLimits.RewindBySecondsMax;
        public const decimal RewindBySecondsDefault = CoreLimits.RewindBySecondsDefault;

        public const int RewindLeadupMsDefault = CoreLimits.RewindLeadupMsDefault;
        public const decimal RewindLeadupSecondsMin = CoreLimits.RewindLeadupSecondsMin;
        public const decimal RewindLeadupSecondsMax = CoreLimits.RewindLeadupSecondsMax;
        public const decimal RewindLeadupSecondsDefault = CoreLimits.RewindLeadupSecondsDefault;

        public const int NspTimerMsDefault = CoreLimits.NspTimerMsDefault;
        public const decimal NspTimerSecondsMin = CoreLimits.NspTimerSecondsMin;
        public const decimal NspTimerSecondsMax = CoreLimits.NspTimerSecondsMax;
        public const decimal NspTimerSecondsDefault = CoreLimits.NspTimerSecondsDefault;

        public const int TuningPedalMin = CoreLimits.TuningPedalMin;
        public const int TuningPedalMax = CoreLimits.TuningPedalMax;

        public static void ApplyToUiControls(
            NumericUpDown volumeInterval,
            NumericUpDown riffRepeaterSpeed,
            NumericUpDown numberOfBackups,
            NumericUpDown overrideInputVolume,
            NumericUpDown forceEnumerationSeconds,
            NumericUpDown loopingLeadUpSeconds,
            NumericUpDown rewindBySeconds,
            NumericUpDown rewindLeadupSeconds,
            NumericUpDown nspTimerSeconds,
            TrackBar fontSize,
            ListBox extendedRangeTunings,
            ListBox midiTuningOffset)
        {
            ConfigureNumeric(volumeInterval, VolumeIntervalMin, VolumeIntervalMax, VolumeIntervalDefault);
            ConfigureNumeric(riffRepeaterSpeed, RiffRepeaterSpeedMin, RiffRepeaterSpeedMax, RiffRepeaterSpeedDefault);
            ConfigureNumeric(numberOfBackups, NumberOfBackupsMin, NumberOfBackupsMax, NumberOfBackupsDefault);
            ConfigureNumeric(overrideInputVolume, OverrideInputVolumeMin, OverrideInputVolumeMax, OverrideInputVolumeDefault);
            ConfigureNumeric(forceEnumerationSeconds, EnumerateIntervalSecondsMin, EnumerateIntervalSecondsMax, EnumerateIntervalSecondsDefault);
            ConfigureNumeric(loopingLeadUpSeconds, LoopingLeadUpSecondsMin, LoopingLeadUpSecondsMax, LoopingLeadUpSecondsDefault);
            ConfigureNumeric(rewindBySeconds, RewindBySecondsMin, RewindBySecondsMax, RewindBySecondsDefault);
            ConfigureNumeric(rewindLeadupSeconds, RewindLeadupSecondsMin, RewindLeadupSecondsMax, RewindLeadupSecondsDefault);
            ConfigureNumeric(nspTimerSeconds, NspTimerSecondsMin, NspTimerSecondsMax, NspTimerSecondsDefault);

            if (fontSize != null)
            {
                if (fontSize.Maximum < FontSizeMax) fontSize.Maximum = FontSizeMax;
                if (fontSize.Minimum > FontSizeMin) fontSize.Minimum = FontSizeMin;
                fontSize.Minimum = FontSizeMin;
                fontSize.Maximum = FontSizeMax;
                fontSize.Value = FontSizeDefault;
            }

            if (extendedRangeTunings != null && extendedRangeTunings.Items.Count > 0)
            {
                int erIndex = (ExtendedRangeTuningDefault * -1) - 2;
                if (erIndex >= 0 && erIndex < extendedRangeTunings.Items.Count)
                    extendedRangeTunings.SelectedIndex = erIndex;
            }

            if (midiTuningOffset != null && midiTuningOffset.Items.Count > 0)
            {
                int midiIndex = MidiTuningOffsetDefault + 3;
                if (midiIndex >= 0 && midiIndex < midiTuningOffset.Items.Count)
                    midiTuningOffset.SelectedIndex = midiIndex;
            }
        }

        public static int ExtendedRangeTuningToListIndex(int tuning) => CoreLimits.ExtendedRangeTuningToListIndex(tuning);

        public static int ListIndexToExtendedRangeTuning(int index) => CoreLimits.ListIndexToExtendedRangeTuning(index);

        public static int MidiOffsetToListIndex(int offset) => CoreLimits.MidiOffsetToListIndex(offset);

        private static void ConfigureNumeric(NumericUpDown control, decimal min, decimal max, decimal defaultValue)
        {
            if (control == null)
                return;

            if (control.Maximum < max)
                control.Maximum = max;
            if (control.Minimum > min)
                control.Minimum = min;

            decimal clampedDefault = defaultValue;
            if (clampedDefault < min) clampedDefault = min;
            if (clampedDefault > max) clampedDefault = max;

            if (control.Value < min || control.Value > max)
                control.Value = clampedDefault;

            control.Minimum = min;
            control.Maximum = max;
            control.Value = clampedDefault;
        }
    }
}
