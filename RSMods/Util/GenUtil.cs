using Microsoft.Win32;
using RSMods.Data;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Microsoft.WindowsAPICodePack.Dialogs;

namespace RSMods.Util
{
    public static class GenUtil
    {
        public static bool IsDirectoryEmpty(string path)
        {
            return !Directory.EnumerateFileSystemEntries(path).Any();
        }

        public static void ExtractEmbeddedResource(string outputDir, Assembly resourceAssembly, string resourceLocation, string[] files)
        {
            if (!Directory.Exists(outputDir))
                Directory.CreateDirectory(outputDir);

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

        private static bool IsRSFolder(this string folderPath)
        {
            if (!Directory.Exists(folderPath))
                return false;

            string dlcFolderPath = Path.Combine(folderPath, "dlc");
            string cachePsarcPath = Path.Combine(folderPath, "cache.psarc");

            //if (IsDirectoryEmpty(dlcFolderPath))
            //    return false;

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

            foreach (var customSteamappsFolder in customSteamppsFolders)
            {
                string dirPath = Path.Combine(customSteamappsFolder, "steamapps", "appmanifest_221680.acf");

                if (File.Exists(dirPath))
                {
                    finalPath = Path.GetDirectoryName(dirPath);

                    rsFolderPath = Path.Combine(finalPath, "common", "Rocksmith2014");

                    if (rsFolderPath.IsRSFolder())
                    {
                        found = true;
                        break;
                    }
                }
            }

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

        public static List<Tuple<string, string>> InstallRegKeys = new List<Tuple<string, string>>()
        {
            new Tuple<string, string>(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Ubisoft\Rocksmith2014", "installdir"),
            new Tuple<string, string>(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680", "InstallLocation"),
            new Tuple<string, string>(@"HKEY_LOCAL_MACHINE\SOFTWARE\Ubisoft\Rocksmith2014", "InstallLocation"),
            new Tuple<string, string>(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680", "InstallLocation")
        };

        public static List<string> GetSettingsLines()
        {
            return File.ReadAllLines(Constants.SettingsPath).ToList();
        }

        private static Dictionary<string, string> settingsDict = null;
        public static Dictionary<string, string> GetSettingsPairs(List<string> settingsLines)
        {
            var dictRet = new Dictionary<string, string>();

            settingsLines.ForEach(line => {
                string entry = line.Split('=')[0].Trim();
                string value = line.Split('=')[1].Trim();

                dictRet.Add(entry, value); 
              }
            );

            settingsDict = dictRet;

            return dictRet;
        }

        public static string GetSettingsEntry(string entryName, Dictionary<string, string> settingsPairs = default) // If you want to reread the settings, don't include the second arg
        {
            if (settingsPairs == null)
                settingsDict = GetSettingsPairs(GetSettingsLines());

            //  return settingsLines.FirstOrDefault(line => line.Contains(entryName)).Split('=')[1].Trim();

            if (settingsDict.ContainsKey(entryName))
                return settingsDict[entryName];
            else
                return String.Empty;
        }

        public static string GetRSDirectory()
        {
            if (!IsRSFolder(Constants.RSFolder))
            {
                if (File.Exists(Constants.SettingsPath))
                {
                    Constants.RSFolder = GetSettingsEntry("RSPath");
                    return Constants.RSFolder;
                }
            }
            else
                return Constants.RSFolder;

            try
            {
                var rs2RootDir = String.Empty;
                var steamRootPath = GetSteamDirectory();

                if (!String.IsNullOrEmpty(steamRootPath))
                {
                    rs2RootDir = Path.Combine(steamRootPath, "SteamApps\\common\\Rocksmith2014");

                    if (!Directory.Exists(rs2RootDir)) // RS-Folder doesn't exist
                    {
                        // Go through each possible registry location
                        foreach (var installRegKey in InstallRegKeys)
                        {
                            var path = GetStringValueFromRegistry(installRegKey.Item1, installRegKey.Item2);

                            if (!string.IsNullOrEmpty(path))
                            {
                                rs2RootDir = path;
                                break;
                            }
                        }



                        rs2RootDir = GetCustomRSFolder(steamRootPath); // Grab custom Steam library paths from .vdf file

                        if (String.IsNullOrEmpty(rs2RootDir) || !rs2RootDir.IsRSFolder()) // If neither that's OK, ask the user to point the GUI to the correct location
                        {
                            MessageBox.Show("We were unable to detect your Rocksmith 2014 folder, please select it manually!", "Your help is required!");
                            return AskUserForRSFolder();
                        }
                    }
                    else // RS-Folder does exist
                    {
                        if (!File.Exists(Path.Combine(rs2RootDir, "cache.psarc")))
                        { // If cache.psarc doesn't exist (old install / steam left-overs)
                            if (AskUserForRSFolder() == String.Empty)
                            {
                                MessageBox.Show("We were unable to detect your Rocksmith 2014 folder, and you didn't give us a valid RS Folder.", "Closing Application");
                                Application.Exit();
                            }

                        }
                    }
                }

                return rs2RootDir;
            }
            catch (Exception ex)
            {
                MessageBox.Show("<Warning> GetStreamDirectory, " + ex.Message);
            }

            return String.Empty;
        }

        public static string AskUserForRSFolder()
        {
            using (CommonOpenFileDialog dialog = new CommonOpenFileDialog()) // FolderBrowserDialog lacks usability, while using OpenFileDialog can be a bit wonky so this is likely the best solution
            {
                dialog.IsFolderPicker = true;
                if (dialog.ShowDialog() == CommonFileDialogResult.Ok)
                {
                    string rsFolder = dialog.FileName;

                    if (rsFolder.IsRSFolder())
                    {
                        Constants.RSFolder = rsFolder;
                        return rsFolder;
                    }
                }
            }
            return String.Empty;
        }
    }
}