using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using RocksmithToolkitLib.DLCPackage;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using RSMods.Data;
using RSMods.Util;
using SevenZip;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Management;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace RSMods
{
    public static class SetAndForgetMods
    {
        #region Generic
        // General PSARC operations 

        public static void RepackCachePsarc()
        {
            try
            {
                if (!Directory.Exists(Constants.CachePcPath))
                    UnpackCachePsarc();

                if (!File.Exists(Path.Combine(Constants.CachePcPath, "sltsv1_aggregategraph.nt")))
                    GenUtil.ExtractEmbeddedResource(Constants.CachePcPath, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "sltsv1_aggregategraph.nt" }); //NOTE: when adding resources, change Build Action to Embeded Resource  

                Packer.Pack(Constants.CachePcPath, Constants.CachePsarcPath);
                MessageBox.Show("cache.psarc repackaged successfully", "Success");
            }
            catch (IOException ex)
            {
                MessageBox.Show("Unable to repack cache.psarc" + Environment.NewLine + "Error: " + ex.Message.ToString(), "Repacking error", MessageBoxButtons.OK);
            }
        }

        public static void UnpackCachePsarc()
        {
            if (!Directory.Exists(Constants.WorkFolder))
                Directory.CreateDirectory(Constants.WorkFolder);

            if (!File.Exists(Constants.CacheBackupPath))
                File.Copy(Constants.CachePsarcPath, Constants.CacheBackupPath);

            Packer.Unpack(Constants.CachePsarcPath, Constants.WorkFolder);
        }

        public static bool RestoreDefaults()
        {
            if (MessageBox.Show(@"Do you wish to restore your cache.psarc to it's original state?", "Restore cache.psarc?", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.No)
                return false;

            try
            {
                if (File.Exists(Constants.CacheBackupPath))
                {
                    File.Copy(Constants.CacheBackupPath, Constants.CachePsarcPath, true);
                    MessageBox.Show("Cache backup was restored!", "Backup restored", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }

                else
                    MessageBox.Show("No cache backup found!", "Error");

                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "tuning.database.json" });
                //TODO: extract the rest

                return true;
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Problems restoring backup: " + ioex.Message, "Error");
                return false;
            }
        }

        public static void CleanUnpackedCache()
        {
            if (!Directory.Exists(Constants.CachePcPath))
                return;

            ZipUtilities.DeleteDirectory(Constants.CachePcPath, true);

            UnpackCachePsarc();
        }

        public static bool ImportExistingSettings()
        {
            if (!File.Exists(Constants.Cache4_7zPath) || !File.Exists(Constants.Cache7_7zPath))
                UnpackCachePsarc();

            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.TuningsJSON_InternalPath);
            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache4_7zPath, Constants.LocalizationCSV_InternalPath);

            return true;
        }

        public static void RemoveTempFolders()
        {
            ZipUtilities.DeleteDirectory(Constants.WorkFolder);
        }

        public static void LoadDefaultFiles()
        {
            if (!File.Exists(Path.Combine(Constants.TuningJSON_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "tuning.database.json" });

            if (!File.Exists(Path.Combine(Constants.IntroGFX_MidPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "introsequence_mid.gfx" });

            if (!File.Exists(Path.Combine(Constants.IntroGFX_MaxPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "introsequence_max.gfx" });

            if (!File.Exists(Path.Combine(Constants.LocalizationCSV_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "maingame.csv" });

            if (!File.Exists(Path.Combine(Constants.ExtendedMenuJson_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "ui_menu_pillar_mission.database.json" });

            if (!File.Exists(Path.Combine(Constants.MainMenuJson_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "ui_menu_pillar_main.database.json" });

            if (!File.Exists(Path.Combine(Constants.DirectConnectStartupJson_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "ui_menu_pillar_startup.database.json" });

            if (!File.Exists(Path.Combine(Constants.WwiseInitBnk_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "init.bnk" });
        }
        #endregion
        #region Custom Tunings
        // Custom Tunings Mod

        public static TuningDefinitionList TuningsCollection;

        public static TuningDefinitionList LoadTuningsCollection()
        {
            string tuningsFileContent = File.ReadAllText(Constants.TuningJSON_CustomPath);
            var tuningsJson = JObject.Parse(tuningsFileContent);
            var tuningsList = tuningsJson["Static"]["TuningDefinitions"];

            return JsonConvert.DeserializeObject<TuningDefinitionList>(tuningsList.ToString());
        }

        public static void AddCustomTunings()
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath)) // Don't replace existing unpacked cache, in case the user wants to add more mods together
                UnpackCachePsarc();

            AddLocalizationForTuningEntries();

            ZipUtilities.InjectFile(Constants.TuningJSON_CustomPath, Constants.Cache7_7zPath, Constants.TuningsJSON_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);
            ZipUtilities.InjectFile(Constants.LocalizationCSV_CustomPath, Constants.Cache4_7zPath, Constants.LocalizationCSV_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }

        public static Tuple<string, string> SplitTuningUIName(string uiName)
        {
            string index, name = uiName;

            Regex rxIndexExists = new Regex(@"\[.*?\]", RegexOptions.Compiled | RegexOptions.IgnoreCase); // If it already has an index enclosed by []
            Regex rxGetIndex = new Regex(@"\[(\d+)\]", RegexOptions.Compiled | RegexOptions.IgnoreCase); // Extract the digits that lay between []
            Regex rxGrabAfterBracket = new Regex(@"\](.*)", RegexOptions.Compiled | RegexOptions.IgnoreCase); // Extract everything post ]
            if (rxIndexExists.IsMatch(uiName))
            {
                index = rxGetIndex.Matches(uiName)[0].Groups[1].Value;
                name = rxGrabAfterBracket.Matches(uiName)[0].Groups[1].Value;
            }
            else
                index = "0";

            return new Tuple<string, string>(index, name);
        }

        public static void AddLocalizationForTuningEntries()
        {
            try
            {
                string currentUIName, csvContents = File.ReadAllText(Constants.LocalizationCSV_CustomPath);
                int newIndex = 37500;

                using (StreamWriter sw = new StreamWriter(Constants.LocalizationCSV_CustomPath, true))
                {
                    foreach (var tuningDefinition in TuningsCollection)
                    {
                        currentUIName = tuningDefinition.Value.UIName;
                        var tuning = SplitTuningUIName(currentUIName);
                        string index = tuning.Item1;
                        string onlyName = tuning.Item2;

                        if (index == "0") // I.e. if it does not contain an index, give it one
                        {
                            while (csvContents.Contains(newIndex.ToString())) // Efficient ? Nope, but does the job
                                newIndex++;

                            tuningDefinition.Value.UIName = String.Format("$[{0}]{1}", newIndex, onlyName); // Append its index in front
                            index = newIndex.ToString();
                        }

                        if (!csvContents.Contains(index)) // If the CSV already contains that index, don't add it to it
                        {
                            sw.Write(sw.NewLine);
                            sw.Write(index);
                            for (int i = 0; i < 7; i++)
                            {
                                sw.Write(',');
                                sw.Write(tuning.Item2);
                            }

                            csvContents += index;
                        }
                    }
                }
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Error: " + ioex.Message.ToString(), "Error");
            }

            SaveTuningsJSON();
        }

        public static void SaveTuningsJSON()
        {
            string tuningsFileContent = File.ReadAllText(Constants.TuningJSON_CustomPath);
            var tuningsJson = JObject.Parse(tuningsFileContent);
            tuningsJson["Static"]["TuningDefinitions"] = JObject.FromObject(SetAndForgetMods.TuningsCollection);

            try
            {
                File.WriteAllText(Constants.TuningJSON_CustomPath, tuningsJson.ToString());
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Error: " + ioex.ToString(), "Error");
            }
        }
        #endregion
        #region Custom Menu Options
        // Custom Menu Options Mod & Direct Mode Mod

        public static void AddExitGameMenuOption()
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath))
                UnpackCachePsarc();

            ZipUtilities.InjectFile(Constants.MainMenuJson_CustomPath, Constants.Cache7_7zPath, Constants.MainMenuJson_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }

        public static void AddDirectConnectModeOption()
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath))
                UnpackCachePsarc();

            ZipUtilities.InjectFile(Constants.ExtendedMenuJson_CustomPath, Constants.Cache7_7zPath, Constants.ExtendedMenuJson_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);
            ZipUtilities.InjectFile(Constants.DirectConnectStartupJson_CustomPath, Constants.Cache7_7zPath, Constants.DirectConnectStartupJson_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }
        #endregion
        #region Default Tones
        // Custom Default Tones Mod

        public static Dictionary<string, Tone2014> tonesFromAllProfiles = new Dictionary<string, Tone2014>();

        public static void SetDefaultTones(string selectedToneName, int selectedToneType, bool alreadyTried = false)
        {
            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath);

            if (!File.Exists(Constants.ToneManager_CustomPath))
            {
                MessageBox.Show("Could not extract tones from cache.psarc.", "Error");
                return;
            }

            string toneManagerFileContent = File.ReadAllText(Constants.ToneManager_CustomPath);
            var tonesJson = JObject.Parse(toneManagerFileContent);
            //var toneList = tonesJson["Static"]["ToneManager"]["Tones"];
            //var defaultTones = JsonConvert.DeserializeObject<List<Tone2014>>(toneList.ToString());

            var selectedTone = tonesFromAllProfiles[selectedToneName];

            tonesJson["Static"]["ToneManager"]["Tones"][selectedToneType]["GearList"] = JObject.FromObject(selectedTone.GearList);

            try
            {
                File.WriteAllText(Constants.ToneManager_CustomPath, tonesJson.ToString());
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Error:{ioex.Message}");
                return;
            }

            ZipUtilities.InjectFile(Constants.ToneManager_CustomPath, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();

            MessageBox.Show("Successfully changed default tones!", "Success");
        }

        public static void SetGuitarArcadeTone(string selectedToneName, int selectedToneType)
        {
            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath);

            if (!File.Exists(Constants.ToneManager_CustomPath))
                MessageBox.Show("Could not extract tones from cache.psarc. Please press Import Existing Settings button!", "Error");

            string toneManagerFileContent = File.ReadAllText(Constants.ToneManager_CustomPath);
            var tonesJson = JObject.Parse(toneManagerFileContent);

            var selectedTone = tonesFromAllProfiles[selectedToneName];

            selectedToneType += 8; // GuitarArcade tones start at the 8th element of Tone list

            var x = tonesJson["Static"]["ToneManager"]["Tones"][selectedToneType];

            tonesJson["Static"]["ToneManager"]["Tones"][selectedToneType]["GearList"] = JObject.FromObject(selectedTone.GearList);

            try
            {
                File.WriteAllText(Constants.ToneManager_CustomPath, tonesJson.ToString());
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Error:{ioex.Message}");
                return;
            }

            ZipUtilities.InjectFile(Constants.ToneManager_CustomPath, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();

            MessageBox.Show("Successfully changed GuitarArcade tones!", "Success");
        }

        public static List<string> GetSteamProfilesTones()
        {
            var profileTones = new List<string>();
            var userprofileFolder = GenUtil.GetSteamProfilesFolderManual();

            if (Directory.Exists(userprofileFolder))
            {
                var profiles = Directory.EnumerateFiles(userprofileFolder, "*_PRFLDB", SearchOption.AllDirectories).ToList();

                tonesFromAllProfiles.Clear();

                foreach (string profile in profiles)
                {
                    foreach (var tone in Tone2014.Import(profile))
                    {
                        tonesFromAllProfiles.Add(tone.Name, tone);
                        profileTones.Add(tone.Name);
                    }
                }
            }

            return profileTones;
        }
        #endregion
        #region Fast Load
        // Fast Load Mod

        private static Tuple<string, bool> GetDriveType(char driveLetter) // This may not work on Win7, MSDN says its for >= Win8
        {
            if ((Environment.OSVersion.Version.Major == 6 && Environment.OSVersion.Version.Minor >= 2) || Environment.OSVersion.Version.Major == 10) // OS Chart here: https://stackoverflow.com/a/2819962
            {
                try
                {
                    uint driveNumber = 0;

                    ManagementScope scope = new ManagementScope(@"\\.\root\microsoft\windows\storage");
                    using (ManagementObjectSearcher searcher = new ManagementObjectSearcher("SELECT * FROM MSFT_Partition")) // Grab drive ID for this partition
                    {
                        scope.Connect();
                        searcher.Scope = scope;

                        foreach (ManagementObject queryObj in searcher.Get())
                        {
                            char letter = (char)queryObj["DriveLetter"];

                            if (letter == driveLetter)
                            {
                                driveNumber = (uint)queryObj["DiskNumber"];
                                break;
                            }
                        }
                    }

                    using (ManagementObjectSearcher searcher = new ManagementObjectSearcher("SELECT * FROM MSFT_PhysicalDisk"))
                    {
                        string type = "";
                        bool isNVMe = false;
                        scope.Connect();
                        searcher.Scope = scope;

                        foreach (ManagementObject queryObj in searcher.Get())
                        {
                            string devID = queryObj["DeviceId"].ToString();

                            if (devID != driveNumber.ToString()) // For whatever reason, DeviceID seems to be equivalent to driveNumber, but unlike driveNumber, it's a string
                                continue;

                            switch (Convert.ToInt16(queryObj["MediaType"]))
                            {
                                case 1:
                                    type = "Unspecified";
                                    break;

                                case 3:
                                    type = "HDD";
                                    break;

                                case 4:
                                    type = "SSD";
                                    break;

                                case 5:
                                    type = "SCM";
                                    break;

                                default:
                                    type = "Unspecified";
                                    break;
                            }

                            if (Convert.ToInt16(queryObj["BusType"]) == 17)
                                isNVMe = true;

                            return new Tuple<string, bool>(type, isNVMe);
                        }
                    }
                }
                catch (ManagementException) //Not much we can do in this case and it's not really important that we inform the user
                { }
            }
            return new Tuple<string, bool>("Unspecified", false);
        }


        private static void AddFastLoadModFile(bool NVMe)
        {
            if (NVMe)
                File.Copy(Constants.IntroGFX_MaxPath, Constants.IntroGFX_CustomPath, true);
            else
                File.Copy(Constants.IntroGFX_MidPath, Constants.IntroGFX_CustomPath, true);
        }

        public static void AddFastLoadMod()
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath))
                UnpackCachePsarc();

            try
            {
                char driveLetter = Constants.RSFolder.ToUpper()[0];
                var driveType = GetDriveType(driveLetter);

                if (driveType.Item1 == "HDD")
                {
                    if (MessageBox.Show(@"It appears as though Rocksmith installed on a hard disk drive. HDDs are normally too slow to support fast load mod and will likely result in a crash. \n Do you wish to proceed?", "Drive too slow for fast load", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.No)
                        return;

                    AddFastLoadModFile(false);
                }
                else if (driveType.Item1 == "SSD")
                {
                    if (driveType.Item2) // If is NVMe
                    {
                        if (MessageBox.Show("Can you confirm Rocksmith is installed on a NVMe drive?\n If you are unsure, press \"No\", because Rocksmith is likely to crash if you pick the fastest option!", "Is RS on a NVMe drive?", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                            AddFastLoadModFile(true);
                        else
                            AddFastLoadModFile(false);
                    }
                    else
                        AddFastLoadModFile(false);
                }
                else
                {
                    if (MessageBox.Show(@"We were unable to detect the drive type on which Rocksmith is installed. \n Is it on a NVMe drive? (if it's not, fastest loading option is likely to crash your game!)", "Fast drive?", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                        AddFastLoadModFile(true);
                    else
                        AddFastLoadModFile(false);
                }
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Unable to copy required files. Error: {ioex.Message}");
            }

            ZipUtilities.InjectFile(Constants.IntroGFX_CustomPath, Constants.Cache4_7zPath, Constants.IntroGFX_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }
        #endregion
        #region Custom Wwise
        public static void AddIncreasedVolumeWwiseBank()
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath))
                UnpackCachePsarc();

            ZipUtilities.InjectFile(Constants.WwiseInitBnk_CustomPath, Constants.Cache3_7zPath, Constants.WwiseInitBnk_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }
        #endregion
    }
}
