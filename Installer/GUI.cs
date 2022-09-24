using System;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Linq;

namespace RS2014_Mod_Installer
{
    public partial class GUI : Form
    {
        public GUI()
        {
            InitializeComponent();
        }

        private void UseModsButton_Click(object sender, EventArgs e)
        {
            string originalButtonText = UseModsButton.Text;
            UseModsButton.Text += "\n(Please wait as we get the mods setup).";
            // Can we find Rocksmith's install?
            if (Worker.WhereIsRocksmith() == string.Empty)
            {
                MessageBox.Show("It looks like your current Rocksmith2014 install folder cannot be found. Please tell us where it is located!", "Error: RSLocation Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                string newRSFolder = RSMods.Util.GenUtil.AskUserForRSFolder();
                if (newRSFolder == string.Empty)
                {
                    MessageBox.Show("We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.", "Error: RSLocation Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Environment.Exit(1);
                    return;
                }

                Worker.RSLocation = newRSFolder;
            }

            IsVoid(Worker.WhereIsRocksmith());

            // Get DLL from Installer
            if (DLLStuff.InjectDLL(Worker.WhereIsRocksmith()))
            {
                // Get GUI from Installer
                if (DLLStuff.InjectGUI(Worker.WhereIsRocksmith()))
                {
                    string rsModsPath = Path.Combine(Worker.WhereIsRocksmith(), "RSMods") + "\\RSMods.exe";
                    MessageBox.Show("This version of the installer allows you to take advantage of the new mod settings available by opening: " + rsModsPath, "New Mod Settings Available!", MessageBoxButtons.OK, MessageBoxIcon.Information);

                    Process.Start(rsModsPath);
                    CreateDesktopShortcut(rsModsPath);

                    this.Close();
                }
            }

            UseModsButton.Text = originalButtonText;
        }

        private void CreateDesktopShortcut(string rsModsPath)
        {
            DialogResult dialogResult = MessageBox.Show("Would you like to create RSMods shortcut on your desktop?", "Create shortcut", MessageBoxButtons.YesNo);
            if (dialogResult != DialogResult.Yes)
            {
                return;
            }

            try
            {
                string deskDir = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);

                using (StreamWriter writer = new StreamWriter(deskDir + @"\\RSMods.url", true))
                {
                    writer.WriteLine("[InternetShortcut]");
                    writer.WriteLine("URL=file:///" + rsModsPath);
                    writer.WriteLine("IconIndex=0");
                    string icon = rsModsPath.Replace('\\', '/');
                    writer.WriteLine("IconFile=" + icon);
                }
            }
            catch (IOException ex)
            {
                MessageBox.Show(ex.Message, "Error creating the shortcut");
            }
        }

        /// <summary>
        /// Hash for Rocksmith2014.exe for the Remastered Update | SHA256
        /// </summary>
        readonly static byte[] HASH_EXE = { 0xA7, 0x25, 0x84, 0x61, 0x10, 0x1D, 0xA0, 0x20, 0x17, 0x07, 0xF5, 0xC2, 0x72, 0xBA, 0xAA, 0x62, 0xA3, 0xD3, 0xD1, 0x0B, 0x3D, 0x22, 0x13, 0xC0, 0xD0, 0xF2, 0x1C, 0xC8, 0x3B, 0x45, 0x88, 0xDA };
        readonly static byte[] HASH_EXE_NEW = { 0x0d, 0x42, 0xe2, 0xff, 0x3c, 0x7a, 0xf6, 0x84, 0x3e, 0xcb, 0x81, 0x25, 0x9c, 0xc6, 0x4f, 0x1d, 0xde, 0xfa, 0x13, 0x97, 0xb7, 0xce, 0x53, 0xfd, 0xcf, 0x0a, 0x05, 0xd0, 0xb6, 0x1a, 0x0d, 0xc3 };

        public static void IsVoid(string installLocation) // Anti-Piracy Check (False = Real, True = Pirated) || Modified from Beat Saber Mod Assistant
        {
            bool fakeSteamApi = true;
            try
            {
                X509Certificate2 cert = new X509Certificate2(X509Certificate.CreateFromSignedFile(Path.Combine(installLocation, "steam_api.dll")));

                if (cert.GetNameInfo(X509NameType.SimpleName, false) == "Valve" || cert.Verify())
                {
                    fakeSteamApi = false;
                }
            }
            catch { } // Fall-through = bad cert.

            if (File.Exists(Path.Combine(installLocation, "IGG-GAMES.COM.url")) || File.Exists(Path.Combine(installLocation, "SmartSteamEmu.ini")) || File.Exists(Path.Combine(installLocation, "GAMESTORRENT.CO.url")) || File.Exists(Path.Combine(installLocation, "Codex.ini")) || File.Exists(Path.Combine(installLocation, "Skidrow.ini")) || File.Exists(Path.Combine(installLocation, "steamclient.dll")) || fakeSteamApi || !CheckExecutable(installLocation))
            {
                MessageBox.Show("RSMods doesn't support pirated / stolen copies of Rocksmith 2014!", "ARGGGG", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Process.Start("https://store.steampowered.com/app/221680/");
                Environment.Exit(1);
                return;
            }
        }

        private static bool CheckExecutable(string installLocation)
        {
            try
            {
                using (SHA256 sha256 = SHA256.Create())
                {
                    FileStream exeStream = File.Open(Path.Combine(installLocation, "Rocksmith2014.exe"), FileMode.Open);
                    exeStream.Position = 0;

                    byte[] hash = sha256.ComputeHash(exeStream);

                    return hash.SequenceEqual(HASH_EXE) || hash.SequenceEqual(HASH_EXE_NEW); // True - User is using Remastered game, False - User is using a NON-Remastered game (VOID).
                }
            }
            catch // Game was open when performing the check
            {
                MessageBox.Show("Please close Rocksmith2014, then re-open this tool!");
                Environment.Exit(1);
                return true;
            }
        }
    }
}
