using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RSMods.Util
{
    public static class GenUtil
    {
        public static void ExtractEmbeddedResource(string outputDir, Assembly resourceAssembly, string resourceLocation, string[] files)
        {
            string[] names = resourceAssembly.GetManifestResourceNames();

            string resourcePath = String.Empty;
            foreach (string file in files)
            {
                resourcePath = Path.Combine(outputDir, file);

                Stream stream = resourceAssembly.GetManifestResourceStream(String.Format("{0}.{1}", resourceLocation, file));
                using (FileStream fileStream = new FileStream(resourcePath, FileMode.Create))
                    stream.CopyTo(fileStream);
            }
        }

        public static bool IsDirectoryEmpty(string path)
        {
            return !Directory.EnumerateFileSystemEntries(path).Any();
        }

        private static bool IsRSFolder(this string folderPath)
        {
            if (!Directory.Exists(folderPath))
                return false;

            string dlcFolderPath = Path.Combine(folderPath, "dlc");
            string cachePsarcPath = Path.Combine(folderPath, "cache.psarc");

            if (IsDirectoryEmpty(dlcFolderPath))
                return false;

            if (!File.Exists(cachePsarcPath))
                return false;

            return true;
        }

        private static string GetStringValueFromRegistry(string keyName, string valueName)
        {
            try
            {
                var retValue = (string)Registry.GetValue(keyName, valueName, "");
                return retValue == null ? String.Empty : retValue;
            }
            catch (Exception)
            {
                return String.Empty;
            }
        }
        private static List<string> GetCustomSteamappsFolders(string mainSteamPath)
        {
            string libRegex = "(^\\t\"[1-9]\").*(\".*\")";
            var libDirs = new List<string>();

            string steamappsFolder = Path.Combine(mainSteamPath, "steamapps");
            string libVdf = Path.Combine(steamappsFolder, "libraryfolders.vdf");

            if (!File.Exists(libVdf))
                return new List<string>();

            var content = File.ReadAllLines(libVdf);
            foreach (string l in content)
            {
                var reg = Regex.Match(l, libRegex);
                string dir = reg.Groups[2].Value;

                if (dir != string.Empty)
                {
                    string ndir = dir.Trim('\"');
                    libDirs.Add(ndir);
                }
            }

            if (libDirs.Count == 0)
                return new List<string>();

            return libDirs;
        }


        private static string GetCustomRSFolder(string mainSteamPath)
        {
            var customSteamppsFolders = GetCustomSteamappsFolders(mainSteamPath);
            string finalPath = string.Empty;
            string rsFolderPath = string.Empty;
            bool found = false;

            customSteamppsFolders.ForEach(dir =>
            {
                string dirPath = Path.Combine(dir, "steamapps", "appmanifest_221680.acf");

                if (File.Exists(dirPath))
                    finalPath = Path.GetDirectoryName(dirPath);

                rsFolderPath = Path.Combine(finalPath, "common", "Rocksmith2014");

                if (rsFolderPath.IsRSFolder())
                {
                    found = true;
                    finalPath = rsFolderPath;
                }
            });

            if (found)
            {
                //  MessageBox.Show("Found Custom RS2014 Installation Directory ...");
                return rsFolderPath;
            }

            // MessageBox.Show("<WARNING> Custom RS2014 Installation Directory not found ...");
            return String.Empty;
        }

        public static string GetSteamDirectory()
        {
            const string steamRegPath = @"HKEY_CURRENT_USER\SOFTWARE\Valve\Steam"; //IIRC it isn't the same on X86 machines, but do we really need to support those?

            return GetStringValueFromRegistry(steamRegPath, "SteamPath").Replace('/', '\\');
        }

        public static string GetRSDirectory()
        {
            try
            {
                const string installValueName = "InstallLocation";

                const string rsX64Path = @"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Ubisoft\Rocksmith2014";
                const string rsX64Steam = @"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680";

                const string rsX86Path = @"HKEY_LOCAL_MACHINE\SOFTWARE\Ubisoft\Rocksmith2014";
                const string rsX86Steam = @"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680";

                var rs2RootDir = String.Empty;
                var steamRootPath = GetSteamDirectory();

                if (!String.IsNullOrEmpty(steamRootPath))
                {
                    rs2RootDir = Path.Combine(steamRootPath, "SteamApps\\common\\Rocksmith2014");

                    if (!Directory.Exists(rs2RootDir) || !rs2RootDir.IsRSFolder())
                        rs2RootDir = GetCustomRSFolder(steamRootPath);
                    else if (!String.IsNullOrEmpty(rs2RootDir))
                    {
                        if (!String.IsNullOrEmpty(GetStringValueFromRegistry(rsX64Path, "installdir")))
                            rs2RootDir = GetStringValueFromRegistry(rsX64Path, "installdir");
                        else if (!String.IsNullOrEmpty(GetStringValueFromRegistry(rsX64Steam, installValueName)))
                            rs2RootDir = GetStringValueFromRegistry(rsX64Steam, installValueName);
                        else if (!String.IsNullOrEmpty(GetStringValueFromRegistry(rsX86Path, installValueName)))
                            rs2RootDir = GetStringValueFromRegistry(rsX86Path, installValueName);
                        else if (!String.IsNullOrEmpty(GetStringValueFromRegistry(rsX86Steam, installValueName)))
                            rs2RootDir = GetStringValueFromRegistry(rsX86Steam, installValueName);

                        //if (!String.IsNullOrEmpty(rs2RootDir))
                        //    MessageBox.Show("Found Steam RS2014 Installation Directory in Registry ...");
                    }
                    // else
                    //      MessageBox.Show("<WARNING> Steam RS2014 Installation Directory not found in Registry ...");
                }
                //else
                //    MessageBox.Show("<WARNING> Steam root path not found in Registry ... ");

                return rs2RootDir;
            }
            catch (Exception ex)
            {
                MessageBox.Show("<Warning> GetStreamDirectory, " + ex.Message);
            }

            return String.Empty;
        }
    }
}
