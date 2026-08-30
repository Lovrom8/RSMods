namespace RSMods.Core
{
    /// <summary>
    /// Frontend-neutral numeric ranges, defaults, and buffer-mode constants for RS_ASIO settings.
    /// </summary>
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
    }
}
