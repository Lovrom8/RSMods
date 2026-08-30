using Newtonsoft.Json.Linq;
using RocksmithToolkitLib.DLCPackage;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using RSMods.Data;
using RSMods.Util;
using SevenZip;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Management;
using System.Reflection;
using System.Threading.Tasks;
using RSMods.Core;
using RSMods.SetAndForget;
using RSMods.SetAndForget.Models;
using ArrangementTuning = Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning;

namespace RSMods
{
    public static class SetAndForgetMods
    {
        #region Generic
        // General PSARC operations 

        public static void RepackCachePsarc()
        {
            if (!Directory.Exists(Constants.CachePcPath))
                UnpackCachePsarc();

            if (!File.Exists(Path.Combine(Constants.CachePcPath, "sltsv1_aggregategraph.nt")))
                GenUtil.ExtractEmbeddedResource(Constants.CachePcPath, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["sltsv1_aggregategraph.nt"]); //NOTE: when adding resources, change Build Action to Embeded Resource

            Packer.Pack(Constants.CachePcPath, Constants.CachePsarcPath);
        }

        public static void UnpackCachePsarc()
        {
            if (!Directory.Exists(Constants.WorkFolder))
                Directory.CreateDirectory(Constants.WorkFolder);

            if (!File.Exists(Constants.CacheBackupPath))
                File.Copy(Constants.CachePsarcPath, Constants.CacheBackupPath);

            Packer.Unpack(Constants.CachePsarcPath, Constants.WorkFolder);
        }

        public static async Task<bool> RestoreDefaults(IDialogService dialogs)
        {
            if (!await dialogs.ShowConfirmAsync("Do you wish to restore your cache.psarc to its original state?", "Restore cache.psarc?"))
                return false;

            try
            {
                if (File.Exists(Constants.CacheBackupPath))
                {
                    File.Copy(Constants.CacheBackupPath, Constants.CachePsarcPath, true);
                    await dialogs.ShowInfoAsync("Cache backup was restored!", "Backup restored");
                }
                else
                {
                    await dialogs.ShowErrorAsync("No cache backup found!");
                }

                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["tuning.database.json"]);
                //TODO: extract the rest

                return true;
            }
            catch (IOException ioex)
            {
                await dialogs.ShowErrorAsync("Problems restoring backup: " + ioex.Message);
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
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["tuning.database.json"]);

            if (!File.Exists(Path.Combine(Constants.IntroGFX_MidPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["introsequence_mid.gfx"]);

            if (!File.Exists(Path.Combine(Constants.IntroGFX_MaxPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["introsequence_max.gfx"]);

            if (!File.Exists(Path.Combine(Constants.LocalizationCSV_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["maingame.csv"]);

            if (!File.Exists(Path.Combine(Constants.ExtendedMenuJson_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["ui_menu_pillar_mission.database.json"]);

            if (!File.Exists(Path.Combine(Constants.MainMenuJson_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["ui_menu_pillar_main.database.json"]);

            if (!File.Exists(Path.Combine(Constants.DirectConnectStartupJson_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["ui_menu_pillar_startup.database.json"]);

            if (!File.Exists(Path.Combine(Constants.WwiseInitBnk_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["init.bnk"]);
        }
        #endregion
        #region Custom Tunings
        // Custom Tunings Mod

        private static readonly TuningService Tuning = new TuningService();

        public static void LoadTuningsCollection()
        {
            Tuning.Load(Constants.TuningJSON_CustomPath);
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

        public static (string Index, string Name) SplitTuningUIName(string uiName)
        {
            return TuningService.SplitUiName(uiName);
        }

        public static void AddLocalizationForTuningEntries()
        {
            Tuning.AddLocalizationEntries(
                Constants.LocalizationCSV_CustomPath,
                Constants.TuningJSON_CustomPath);
        }

        public static void SaveTuningsJSON()
        {
            Tuning.Save(Constants.TuningJSON_CustomPath);
        }
        #endregion
        #region Tuning Queries

        public static ArrangementTuning ToArrangementTuning(TuningDefinitionInfo tuning)
        {
            return TuningService.ToArrangementTuning(tuning);
        }

        public static bool IsTuningStandard(ArrangementTuning t, bool forceBass = false) =>
            TuningService.IsStandard(t, forceBass);

        public static bool IsTuningDrop(ArrangementTuning t, bool forceBass = false) =>
            TuningService.IsDrop(t, forceBass);

        public static IEnumerable<ArrangementTuning> GetDefinedTunings() =>
            Tuning.GetDefinedTunings();

        public static SortedDictionary<string, ArrangementTuning> GetUnknownTunings(IEnumerable<SongData> songs)
        {
            return Tuning.GetUnknownTunings(songs);
        }

        public static List<string> GetSongsWithTuning(IEnumerable<SongData> songs, ArrangementTuning tuning)
        {
            return Tuning.GetSongsWithTuning(songs, tuning);
        }

        public static List<string> GetSongsWithBadBassTuning(IEnumerable<SongData> songs)
        {
            return Tuning.GetSongsWithBadBassTuning(songs);
        }

        #endregion
        #region Custom Menu Options
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
        private static readonly Dictionary<string, Tone2014> tonesFromAllProfiles = [];

        public static TuningDefinitionList TuningsCollection => Tuning.Tunings;

        private static (bool IsSuccess, string ErrorMessage) UpdateToneManagerInCache(string selectedToneName, int targetToneIndex)
        {
            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath);

            if (!File.Exists(Constants.ToneManager_CustomPath))
            {
                return (false, "Could not extract tones from cache.psarc. Please check your existing settings.");
            }

            if (!tonesFromAllProfiles.TryGetValue(selectedToneName, out var selectedTone))
            {
                return (false, $"The tone '{selectedToneName}' could not be found in the loaded profiles.");
            }

            string toneManagerFileContent = File.ReadAllText(Constants.ToneManager_CustomPath);
            var tonesJson = JObject.Parse(toneManagerFileContent);

            tonesJson["Static"]["ToneManager"]["Tones"][targetToneIndex]["GearList"] = JObject.FromObject(selectedTone.GearList);

            try
            {
                File.WriteAllText(Constants.ToneManager_CustomPath, tonesJson.ToString());
            }
            catch (IOException ioex)
            {
                return (false, $"Error saving tone data: {ioex.Message}");
            }

            ZipUtilities.InjectFile(Constants.ToneManager_CustomPath, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            try
            {
                RepackCachePsarc();
            }
            catch (IOException ioex)
            {
                return (false, $"Unable to repack cache.psarc: {ioex.Message}");
            }

            return (true, string.Empty);
        }

        public static (bool IsSuccess, string Message) SetGuitarArcadeTone(string selectedToneName, int selectedToneType)
        {
            const int GuitarArcadeOffset = 8;
            int targetIndex = selectedToneType + GuitarArcadeOffset;

            var (IsSuccess, ErrorMessage) = UpdateToneManagerInCache(selectedToneName, targetIndex);

            if (!IsSuccess)
            {
                return (false, ErrorMessage);
            }

            return (true, "Successfully changed Guitarcade tones!");
        }

        public static (bool IsSuccess, string Message) SetDefaultTones(string selectedToneName, int selectedToneType)
        {
            var (IsSuccess, ErrorMessage) = UpdateToneManagerInCache(selectedToneName, selectedToneType);

            if (!IsSuccess)
            {
                return (false, ErrorMessage);
            }

            return (true, "Successfully changed default tones!");
        }

        public static List<string> GetSteamProfilesTones()
        {
            var profileTones = new List<string>();
            var userProfileFolder = GenUtil.GetSteamProfilesFolderManual();

            if (Directory.Exists(userProfileFolder))
            {
                var profiles = Directory.EnumerateFiles(userProfileFolder, "*_PRFLDB", SearchOption.AllDirectories).ToList();

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

        private static bool DoesOSSupportReadingDriveTypes()
        {
            return Environment.OSVersion.Version >= new Version(6, 2); // OS Chart here: https://stackoverflow.com/a/2819962
        }

        private static uint? GetDiskNumber(ManagementScope scope, char driveLetter)
        {
            string query = $"SELECT DiskNumber FROM MSFT_Partition WHERE DriveLetter = '{driveLetter}'";

            using var searcher = new ManagementObjectSearcher(scope, new ObjectQuery(query));
            using var results = searcher.Get();

            var partition = results.Cast<ManagementObject>().FirstOrDefault();

            if (partition == null)
                return null;

            return Convert.ToUInt32(partition["DiskNumber"]);
        }

        // Physical drive media type, mapped from the WMI MSFT_PhysicalDisk MediaType code.
        public enum DriveMediaType { Unspecified, Hdd, Ssd, Scm }

        private static (DriveMediaType Type, bool IsNVMe) GetDriveSpecifications(ManagementScope scope, uint diskNumber)
        {
            string query = $"SELECT MediaType, BusType FROM MSFT_PhysicalDisk WHERE DeviceId = '{diskNumber}'";

            using var searcher = new ManagementObjectSearcher(scope, new ObjectQuery(query));
            using var results = searcher.Get();

            var disk = results.Cast<ManagementObject>().FirstOrDefault();

            if (disk == null)
                return (DriveMediaType.Unspecified, false);

            DriveMediaType type = Convert.ToInt16(disk["MediaType"]) switch
            {
                3 => DriveMediaType.Hdd,
                4 => DriveMediaType.Ssd,
                5 => DriveMediaType.Scm,
                _ => DriveMediaType.Unspecified,
            };

            bool isNVMe = Convert.ToInt16(disk["BusType"]) == 17;

            return (type, isNVMe);
        }

        public static (DriveMediaType Type, bool IsNVMe) GetDriveType(char driveLetter)
        {
            if (!DoesOSSupportReadingDriveTypes()) // This may not work on Win7, MSDN says its for >= Win8
                return (DriveMediaType.Unspecified, false);

            try
            {
                ManagementScope scope = new(@"\\.\root\microsoft\windows\storage");
                scope.Connect();

                uint? diskNumber = GetDiskNumber(scope, driveLetter);

                if (diskNumber == null)
                    return (DriveMediaType.Unspecified, false);

                return GetDriveSpecifications(scope, diskNumber.Value);
            }
            catch (ManagementException)
            {
                // Best effort - not much we can do in this case and it's not really important that we inform the user
                return (DriveMediaType.Unspecified, false);
            }
        }

        private static void AddFastLoadModFile(bool NVMe)
        {
            if (NVMe)
                File.Copy(Constants.IntroGFX_MaxPath, Constants.IntroGFX_CustomPath, true);
            else
                File.Copy(Constants.IntroGFX_MidPath, Constants.IntroGFX_CustomPath, true);
        }

        // Which confirmation (if any) the UI should show before applying the fast-load mod,
        // based on the detected drive type.
        public enum FastLoadDrivePrompt
        {
            None,            // Fast enough (SSD, non-NVMe) - no question needed.
            ConfirmHddRisk,  // HDD - warn it may crash; proceed only if confirmed.
            ConfirmNvme,     // NVMe detected - confirm before using the fastest option.
            ConfirmUnknown   // Drive type unknown - ask whether it's NVMe.
        }

        public static FastLoadDrivePrompt GetFastLoadDrivePrompt()
        {
            char driveLetter = Constants.RSFolder.ToUpper()[0];
            var (driveType, isNVMe) = GetDriveType(driveLetter);

            if (driveType == DriveMediaType.Hdd) return FastLoadDrivePrompt.ConfirmHddRisk;
            if (driveType == DriveMediaType.Ssd && isNVMe) return FastLoadDrivePrompt.ConfirmNvme;
            if (driveType == DriveMediaType.Unspecified) return FastLoadDrivePrompt.ConfirmUnknown;
            return FastLoadDrivePrompt.None;
        }

        public static void ApplyFastLoadMod(bool useNvmeFastLoad)
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath))
                UnpackCachePsarc();

            AddFastLoadModFile(useNvmeFastLoad);

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
