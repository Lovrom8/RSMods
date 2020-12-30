using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using Microsoft.Win32;
using System.Windows.Forms;
using RSMods.Data;

namespace RSMods
{
    class BackupProfile
    {

        private static string GetSaveDirectory()
        {
            RegistryKey availableUser = Registry.CurrentUser.OpenSubKey("SOFTWARE").OpenSubKey("Valve").OpenSubKey("Steam").OpenSubKey("Users");
            string steamFolder = Registry.CurrentUser.OpenSubKey("SOFTWARE").OpenSubKey("Valve").OpenSubKey("Steam").GetValue("SteamPath").ToString(), profileSubFolders = "/221680/remote", userDataFolder = "/userdata/", fullProfileFolder = "";

            foreach(string user in availableUser.GetSubKeyNames())
            {
                if (Directory.Exists(steamFolder + userDataFolder + user + profileSubFolders))
                {
                    fullProfileFolder = steamFolder + userDataFolder + user + profileSubFolders;
                    break;
                }
                    
                else
                    continue;
            }

            return fullProfileFolder;
        }

        public static void SaveProfile()
        {
            string profileFolder = GetSaveDirectory();

            if (profileFolder == "")
            {
                MessageBox.Show("We can't find your profiles to backup");
                return;
            }

            string profileBackupsFolder = Path.Combine(RSMods.Data.Constants.RSFolder, "Profile_Backups");
            DateTime now = DateTime.Now;
            string timedBackupFolder = Path.Combine(profileBackupsFolder, now.ToString("MM-dd-yyyy_HH-mm-ss"));
            string howToRestoreBackupTxt = Path.Combine(profileBackupsFolder, "howto.txt");

            Directory.CreateDirectory(profileBackupsFolder);
            Directory.CreateDirectory(timedBackupFolder);

            using (StreamWriter sw = File.CreateText(howToRestoreBackupTxt))
            {
                sw.WriteLine("If your save gets corrupted, take all the files in one of these folders and put them in this folder: " + profileFolder);
            }

            foreach(var file in Directory.GetFiles(profileFolder))
            {
                File.Copy(file, Path.Combine(timedBackupFolder, Path.GetFileName(file)));
            }
        }
    }
}
