using System;
using System.IO;
using System.Windows.Forms;

namespace Rocksmith2014_CDLC_Injector
{
    class Worker
    {
        public static string WhereIsRocksmith()
        {
            return RSMods.Util.GenUtil.GetRSDirectory();
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
