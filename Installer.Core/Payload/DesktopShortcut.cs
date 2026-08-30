using System.IO;

namespace RS2014_Mod_Installer.Payload
{
    // The .url shortcut writer, split out from the Form so the launch target can be asserted in tests.
    // The UI layer owns the shortcut choice and desktop-directory lookup.
    public static class DesktopShortcut
    {
        // Writes an [InternetShortcut] .url at <paramref name="shortcutPath"/> that launches
        // <paramref name="targetExePath"/> (expected to be the installed RSMods.exe).
        public static void WriteUrlShortcut(string shortcutPath, string targetExePath)
        {
            using var writer = new StreamWriter(shortcutPath, append: false);

            writer.WriteLine("[InternetShortcut]");
            writer.WriteLine("URL=file:///" + targetExePath);
            writer.WriteLine("IconIndex=0");
            writer.WriteLine("IconFile=" + targetExePath.Replace('\\', '/'));
        }
    }
}
