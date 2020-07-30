using System;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;

namespace Rocksmith2014_CDLC_Injector
{
    public partial class GUI : Form
    {
        public GUI()
        {
            InitializeComponent();
        }

        private void UseCDLCButton_Click(object sender, EventArgs e)
        {
            IsVoid(Worker.WhereIsRocksmith());
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
