using System.Windows.Forms;
using CoreLimits = RSMods.Core.RsAsioLimits;

// Namespace RSMods (not RSMods.ASIO) so unqualified `RsAsioLimits` in the WinForms UI resolves to this
// re-export via the enclosing namespace, winning over the using-imported RSMods.Core.RsAsioLimits.
namespace RSMods
{
    /// <summary>
    /// WinForms-facing view over the shared <see cref="CoreLimits"/> RS_ASIO ranges. The ranges,
    /// defaults, buffer-mode constants, and validation helpers now live in GUI.Core so the Avalonia
    /// frontend behaves identically; this class re-exports them (existing WinForms callers unchanged)
    /// and keeps the WinForms-only control wiring in <see cref="ApplyToUiControls"/>.
    /// </summary>
    public static class RsAsioLimits
    {
        public const int CustomBufferSizeMin = CoreLimits.CustomBufferSizeMin;
        public const int CustomBufferSizeMax = CoreLimits.CustomBufferSizeMax;
        public const int CustomBufferSizeDefault = CoreLimits.CustomBufferSizeDefault;

        public const int ChannelMin = CoreLimits.ChannelMin;
        public const int ChannelMax = CoreLimits.ChannelMax;
        public const int ChannelDefault = CoreLimits.ChannelDefault;
        public const int OutputBaseChannelDefault = CoreLimits.OutputBaseChannelDefault;
        public const int OutputAltBaseChannelDefault = CoreLimits.OutputAltBaseChannelDefault;
        public const int Input0ChannelDefault = CoreLimits.Input0ChannelDefault;
        public const int Input1ChannelDefault = CoreLimits.Input1ChannelDefault;
        public const int InputMicChannelDefault = CoreLimits.InputMicChannelDefault;

        public const int VolumePercentMin = CoreLimits.VolumePercentMin;
        public const int VolumePercentMax = CoreLimits.VolumePercentMax;
        public const int VolumePercentDefault = CoreLimits.VolumePercentDefault;

        public const string BufferModeDriver = CoreLimits.BufferModeDriver;
        public const string BufferModeHost = CoreLimits.BufferModeHost;
        public const string BufferModeCustom = CoreLimits.BufferModeCustom;

        public const bool EnableWasapiOutputsDefault = CoreLimits.EnableWasapiOutputsDefault;
        public const bool EnableWasapiInputsDefault = CoreLimits.EnableWasapiInputsDefault;
        public const bool EnableAsioDefault = CoreLimits.EnableAsioDefault;
        public const bool EnableSoftwareVolumeControlDefault = CoreLimits.EnableSoftwareVolumeControlDefault;

        public static bool IsValidBufferMode(string mode) => CoreLimits.IsValidBufferMode(mode);

        public static bool IsWasapiOutputsPromptMode(string rawValue) => CoreLimits.IsWasapiOutputsPromptMode(rawValue);

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
