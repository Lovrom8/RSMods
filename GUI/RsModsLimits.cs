using System.Windows.Forms;

namespace RSMods
{
    public static class RsModsLimits
    {
        public const int VolumeIntervalMin = 1;
        public const int VolumeIntervalMax = 100;
        public const int VolumeIntervalDefault = 5;

        public const int RiffRepeaterSpeedMin = -50;
        public const int RiffRepeaterSpeedMax = 50;
        public const int RiffRepeaterSpeedDefault = 2;

        public const int NumberOfBackupsMin = 0;
        public const int NumberOfBackupsMax = 100000;
        public const int NumberOfBackupsDefault = 50;

        public const int OverrideInputVolumeMin = 0;
        public const int OverrideInputVolumeMax = 100;
        public const int OverrideInputVolumeDefault = 17;

        public const int FontSizeMin = 8;
        public const int FontSizeMax = 80;
        public const int FontSizeDefault = 24;

        public const int ExtendedRangeTuningMin = -12;
        public const int ExtendedRangeTuningMax = -2;
        public const int ExtendedRangeTuningDefault = -5;

        public const int MidiTuningOffsetMin = -3;
        public const int MidiTuningOffsetMax = 12;
        public const int MidiTuningOffsetDefault = 0;

        public const int EnumerateIntervalMsDefault = 5000;
        public const decimal EnumerateIntervalSecondsMin = 0.1m;
        public const decimal EnumerateIntervalSecondsMax = 100000m;
        public const decimal EnumerateIntervalSecondsDefault = 5m;

        public const int LoopingLeadUpMsDefault = 0;
        public const decimal LoopingLeadUpSecondsMin = 0m;
        public const decimal LoopingLeadUpSecondsMax = 5m;
        public const decimal LoopingLeadUpSecondsDefault = 0m;

        public const int RewindByMsDefault = 5000;
        public const decimal RewindBySecondsMin = 0m;
        public const decimal RewindBySecondsMax = 90m;
        public const decimal RewindBySecondsDefault = 5m;

        public const int RewindLeadupMsDefault = 2000;
        public const decimal RewindLeadupSecondsMin = 0m;
        public const decimal RewindLeadupSecondsMax = 90m;
        public const decimal RewindLeadupSecondsDefault = 2m;

        public const int NspTimerMsDefault = 10000;
        public const decimal NspTimerSecondsMin = 2m;
        public const decimal NspTimerSecondsMax = 60m;
        public const decimal NspTimerSecondsDefault = 10m;

        public const int TuningPedalMin = 1;
        public const int TuningPedalMax = 4;

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

        public static int ExtendedRangeTuningToListIndex(int tuning) => (tuning * -1) - 2;

        public static int ListIndexToExtendedRangeTuning(int index) => (index * -1) - 2;

        public static int MidiOffsetToListIndex(int offset) => offset + 3;

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
