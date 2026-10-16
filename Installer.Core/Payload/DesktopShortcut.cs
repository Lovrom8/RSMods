using System.IO;
using System.Text;

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
            // The shell reads .url files as INI: UTF-16 with a BOM keeps non-ASCII paths intact, where UTF-8 without one
            // is read in the ANSI code page.
            using var writer = new StreamWriter(shortcutPath, append: false, Encoding.Unicode);

            writer.WriteLine("[InternetShortcut]");
            writer.WriteLine("URL=" + FileUrl(targetExePath));
            writer.WriteLine("IconIndex=0");
            writer.WriteLine("IconFile=" + targetExePath.Replace('\\', '/'));
        }

        // A literal '#' would start a fragment and '%' an escape, so both are escaped; the rest stays as the path.
        public static string FileUrl(string path) => "file:///" + path.Replace("%", "%25").Replace("#", "%23");
    }
}
