using System;
using System.IO;
using System.Windows.Forms;

namespace Rocksmith2014_CDLC_Injector
{
    class Worker
    {
        public static string WhereIsRocksmith()
        {
            if (File.Exists(@"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rocksmith2014\\Rocksmith2014.exe"))
            {
                return @"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rocksmith2014\\";
            }
            else
            {
                FolderBrowserDialog AskRocksmithLocation = new FolderBrowserDialog();
                AskRocksmithLocation.RootFolder = Environment.SpecialFolder.MyComputer;
                AskRocksmithLocation.Description = "Where is your Rocksmith Installed at?";
                if (AskRocksmithLocation.ShowDialog() == DialogResult.OK)
                {
                    if (File.Exists(@AskRocksmithLocation.SelectedPath + "\\Rocksmith2014.exe"))
                    {
                        return @AskRocksmithLocation.SelectedPath;
                    }
                    else
                    {
                        MessageBox.Show("The folder you selected ''" + AskRocksmithLocation.SelectedPath + "'' does not contain Rocksmith 2014.", "Rocksmith Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        WhereIsRocksmith();
                    }
                }
                else
                {
                    Environment.Exit(1);
                    return "exit";
                }
            }
            return WhereIsRocksmith();
        }
    }
    class DLLStuff
    {
        public static  void InjectDLL(string rocksmithLocation)
        {
            File.WriteAllBytes(Path.Combine(@rocksmithLocation, "D3DX9_42.dll"), Properties.Resources.D3DX9_42);
            MessageBox.Show("CDLC should now work in your game. Make sure to purchase Cherub Rock on Steam if they don't show up", "Congrats!", MessageBoxButtons.OK, MessageBoxIcon.Information);
            System.Diagnostics.Process.Start("https://store.steampowered.com/app/248750/");
            Environment.Exit(1);
        }
    }
} 
