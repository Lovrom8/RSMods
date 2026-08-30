using System.Windows.Forms;

namespace RSMods.ASIO
{
    public static class RsAsioLimits
    {
        public const int CustomBufferSizeMin = 1;
        public const int CustomBufferSizeMax = 65536;
        public const int CustomBufferSizeDefault = 48;

        public const int ChannelMin = 0;
        public const int ChannelMax = 255;
        public const int ChannelDefault = 0;
        public const int OutputBaseChannelDefault = 0;
        public const int OutputAltBaseChannelDefault = 0;
        public const int Input0ChannelDefault = 0;
        public const int Input1ChannelDefault = 1;
        public const int InputMicChannelDefault = 1;

        public const int VolumePercentMin = 0;
        public const int VolumePercentMax = 1000;
        public const int VolumePercentDefault = 100;

        public const string BufferModeDriver = "driver";
        public const string BufferModeHost = "host";
        public const string BufferModeCustom = "custom";

        public const bool EnableWasapiOutputsDefault = false;
        public const bool EnableWasapiInputsDefault = false;
        public const bool EnableAsioDefault = true;
        public const bool EnableSoftwareVolumeControlDefault = true;

        public static bool IsValidBufferMode(string mode)
        {
            if (string.IsNullOrWhiteSpace(mode))
                return false;
            string m = mode.Trim().ToLowerInvariant();
            return m == BufferModeDriver || m == BufferModeHost || m == BufferModeCustom;
        }

        public static bool IsWasapiOutputsPromptMode(string rawValue)
        {
            if (string.IsNullOrWhiteSpace(rawValue))
                return false;
            return int.TryParse(rawValue.Trim(), out int v) && v < 0;
        }

        public static void ApplyToUiControls(
            NumericUpDown customBufferSize,
            NumericUpDown outputBaseChannel,
            NumericUpDown outputAltBaseChannel,
            NumericUpDown outputMaxVolume,
            NumericUpDown input0Channel,
            NumericUpDown input0MaxVolume,
            NumericUpDown input1Channel,
            NumericUpDown input1MaxVolume,
            NumericUpDown inputMicChannel,
            NumericUpDown inputMicMaxVolume,
            CheckBox wasapiOutputsTriState)
        {
            ConfigureNumeric(customBufferSize, CustomBufferSizeMin, CustomBufferSizeMax, CustomBufferSizeDefault);
            ConfigureNumeric(outputBaseChannel, ChannelMin, ChannelMax, OutputBaseChannelDefault);
            ConfigureNumeric(outputAltBaseChannel, ChannelMin, ChannelMax, OutputAltBaseChannelDefault);
            ConfigureNumeric(outputMaxVolume, VolumePercentMin, VolumePercentMax, VolumePercentDefault);
            ConfigureNumeric(input0Channel, ChannelMin, ChannelMax, Input0ChannelDefault);
            ConfigureNumeric(input0MaxVolume, VolumePercentMin, VolumePercentMax, VolumePercentDefault);
            ConfigureNumeric(input1Channel, ChannelMin, ChannelMax, Input1ChannelDefault);
            ConfigureNumeric(input1MaxVolume, VolumePercentMin, VolumePercentMax, VolumePercentDefault);
            ConfigureNumeric(inputMicChannel, ChannelMin, ChannelMax, InputMicChannelDefault);
            ConfigureNumeric(inputMicMaxVolume, VolumePercentMin, VolumePercentMax, VolumePercentDefault);

            if (wasapiOutputsTriState != null)
            {
                wasapiOutputsTriState.ThreeState = true;
                wasapiOutputsTriState.CheckState = CheckState.Unchecked;
            }
        }

        private static void ConfigureNumeric(NumericUpDown control, int min, int max, int defaultValue)
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
