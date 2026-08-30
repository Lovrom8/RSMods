#if NET8_0_OR_GREATER
using System.Runtime.Versioning;

// Declaring the assembly Windows-supported resolves the CA1416 platform-compatibility 
// warnings on the registry calls without pinning the TFM to net8.0-windows. Guarded by
// NET8_0_OR_GREATER because SupportedOSPlatformAttribute does not exist on net48.
[assembly: SupportedOSPlatform("windows")]
#endif
