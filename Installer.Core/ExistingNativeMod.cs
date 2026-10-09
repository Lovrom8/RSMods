using System.Diagnostics;
using System.IO;

namespace RS2014_Mod_Installer.Core
{
    public enum ExistingNativeModKind
    {
        None,         // No xinput1_3.dll yet.
        RsMods,       // An official RSMods build, or one from before builds carried version info.
        CustomRsMods, // An RSMods build with mods from DLL/ExternalMods compiled in.
        OtherProgram  // Another program's xinput1_3.dll, e.g. a controller wrapper.
    }

    public sealed record ExistingNativeMod(ExistingNativeModKind Kind, string Description)
    {
        // Replacing either loses something the player chose to install.
        public bool NeedsConfirmation => Kind is ExistingNativeModKind.CustomRsMods or ExistingNativeModKind.OtherProgram;
    }

    // Tells what the installer is about to replace in the Rocksmith folder, from the DLL's version info.
    public static class ExistingNativeModInspector
    {
        public const string NativeModFileName = "xinput1_3.dll";
        private const string ProductName = "RSMods";
        private const string ExternalModsPrefix = "External mods: "; // DLL/Version.rc, SpecialBuild
        private const string LooseManifestName = "mods.manifest.json";

        public static ExistingNativeMod Inspect(string rocksmithLocation)
        {
            string path = Path.Combine(rocksmithLocation, NativeModFileName);
            if (!File.Exists(path))
                return new ExistingNativeMod(ExistingNativeModKind.None, string.Empty);

            FileVersionInfo info = FileVersionInfo.GetVersionInfo(path);
            return Classify(info.ProductName, info.ProductVersion, info.SpecialBuild);
        }

        public static ExistingNativeMod Classify(string productName, string productVersion, string specialBuild)
        {
            // RSMods builds before 1.2.8.4 had no version info, so a DLL without a product name counts as ours.
            if (string.IsNullOrWhiteSpace(productName))
                return new ExistingNativeMod(ExistingNativeModKind.RsMods, string.Empty);

            if (productName != ProductName)
            {
                string version = string.IsNullOrWhiteSpace(productVersion) ? string.Empty : " " + productVersion.Trim();
                return new ExistingNativeMod(ExistingNativeModKind.OtherProgram, productName.Trim() + version);
            }

            if (specialBuild != null && specialBuild.StartsWith(ExternalModsPrefix))
                return new ExistingNativeMod(ExistingNativeModKind.CustomRsMods, specialBuild.Substring(ExternalModsPrefix.Length).Trim());

            return new ExistingNativeMod(ExistingNativeModKind.RsMods, string.Empty);
        }

        // A custom build ships its own manifest next to the configurator, which would keep listing settings for mods
        // the official DLL doesn't have. Kept as .bak rather than deleted: reinstalling the custom build restores it.
        public static void SetAsideCustomManifest(string rsModsFolder)
        {
            string manifest = Path.Combine(rsModsFolder, LooseManifestName);
            if (File.Exists(manifest))
                File.Move(manifest, manifest + ".bak", overwrite: true);
        }
    }
}
