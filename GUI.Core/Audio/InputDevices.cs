using System.Collections.Generic;
using NAudio.CoreAudioApi;

namespace RSMods.Audio
{
    /// <summary>
    /// Enumeration of active audio capture (input) devices via the Windows core-audio (WASAPI) API.
    /// Detection is best-effort and returns an empty list on failure. Mirrors the shared ASIO device enumeration.
    /// </summary>
    public static class InputDevices
    {
        /// <summary>The friendly names of the active capture endpoints, in enumeration order.</summary>
        public static List<string> FriendlyNames()
        {
            var names = new List<string>();

            try
            {
                using var enumerator = new MMDeviceEnumerator();
                foreach (MMDevice device in enumerator.EnumerateAudioEndPoints(DataFlow.Capture, DeviceState.Active))
                {
                    using (device)
                        names.Add(device.FriendlyName);
                }
            }
            catch
            {
                // Best-effort detection; leave the list empty on any core-audio failure.
            }

            return names;
        }
    }
}
