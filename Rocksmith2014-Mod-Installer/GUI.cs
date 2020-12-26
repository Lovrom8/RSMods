using System;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;

namespace RS2014_Mod_Installer
{
    public partial class GUI : Form
    {
        public GUI()
        {
            InitializeComponent();
        }
        // 
        private void UseCDLCButton_Click(object sender, EventArgs e)
        {
            if (Worker.WhereIsRocksmith() == String.Empty)
            {
                MessageBox.Show("We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.", "Error: RSLocation Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Environment.Exit(1);
                return;
            }

            IsVoid(Worker.WhereIsRocksmith());
            if (DLLStuff.InjectGUI(Worker.WhereIsRocksmith()))
            {
                MessageBox.Show("This version of the installer allows you to take advantage of the new mod settings available by opening: " + Path.Combine(Worker.WhereIsRocksmith(), "RSMods") + "\\RSMods.exe", "New Mod Settings Available!", MessageBoxButtons.OK, MessageBoxIcon.Information);
                Process.Start(Path.Combine(Worker.WhereIsRocksmith(), "RSMods") + "\\RSMods.exe");
            }

            DLLStuff.InjectDLL(Worker.WhereIsRocksmith());
        }

        private static void IsVoid(string installLocation) // Anti-Piracy Check (False = Real, True = Pirated) || Modified from Beat Saber Mod Assistant
        {
            if (File.Exists(Path.Combine(installLocation, "IGG-GAMES.COM.url")) || File.Exists(Path.Combine(installLocation, "SmartSteamEmu.ini")) || File.Exists(Path.Combine(installLocation, "GAMESTORRENT.CO.url")) || File.Exists(Path.Combine(installLocation, "Codex.ini")) || File.Exists(Path.Combine(installLocation, "Skidrow.ini")))
            {
                MessageBox.Show("You're pretty stupid. The DLL that works with this GUI, doesn't support pirated copies.", "ARGGGG", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Process.Start("chrome.exe", "https://store.steampowered.com/app/221680/");
                Environment.Exit(1);
                return;
            }
        }
    }
}
