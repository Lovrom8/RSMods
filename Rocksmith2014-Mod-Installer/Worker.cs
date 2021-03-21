using System.IO;
using System.Windows.Forms;

namespace RS2014_Mod_Installer
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
        public static bool InjectDLL(string rocksmithLocation)
        {
            try
            {
                File.WriteAllBytes(Path.Combine(@rocksmithLocation, "D3DX9_42.dll"), Properties.Resources.D3DX9_42);
                return true;
            }
            catch (IOException)
            {
                MessageBox.Show("Please close Rocksmith, then press this button again.\nWe cannot create the necessary files while the game is open.", "Error: Rocksmith is open", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }
        }

        public static bool InjectGUI(string rocksmithLocation)
        {
            string rootModFolder = Path.Combine(rocksmithLocation, "RSMods");
            string customModsFolder = Path.Combine(rootModFolder, "CustomMods");
            string ddcFolder = Path.Combine(rootModFolder, "ddc");
            string toolsFolder = Path.Combine(rootModFolder, "tools");

            Directory.CreateDirectory(rootModFolder);
            Directory.CreateDirectory(customModsFolder);
            Directory.CreateDirectory(ddcFolder);
            Directory.CreateDirectory(toolsFolder);
            try
            {
                // Root Folder
                File.WriteAllBytes(Path.Combine(rootModFolder, "7z.dll"), Properties.Resources._7z);
                File.WriteAllBytes(Path.Combine(rootModFolder, "ICSharpCode.SharpZipLib.dll"), Properties.Resources.ICSharpCode_SharpZipLib);
                File.WriteAllBytes(Path.Combine(rootModFolder, "Microsoft.WindowsAPICodePack.dll"), Properties.Resources.Microsoft_WindowsAPICodePack);
                File.WriteAllBytes(Path.Combine(rootModFolder, "Microsoft.WindowsAPICodePack.Shell.dll"), Properties.Resources.Microsoft_WindowsAPICodePack_Shell);
                File.WriteAllBytes(Path.Combine(rootModFolder, "Microsoft.Extensions.Logging.Abstractions.dll"), Properties.Resources.Microsoft_Extensions_Logging_Abstractions);
                File.WriteAllBytes(Path.Combine(rootModFolder, "MiscUtil.dll"), Properties.Resources.MiscUtil);
                File.WriteAllBytes(Path.Combine(rootModFolder, "NDesk.Options.dll"), Properties.Resources.NDesk_Options);
                File.WriteAllBytes(Path.Combine(rootModFolder, "Newtonsoft.Json.dll"), Properties.Resources.Newtonsoft_Json);
                File.WriteAllBytes(Path.Combine(rootModFolder, "NLog.dll"), Properties.Resources.NLog);
                File.WriteAllBytes(Path.Combine(rootModFolder, "Ookii.Dialogs.dll"), Properties.Resources.Ookii_Dialogs);
                File.WriteAllBytes(Path.Combine(rootModFolder, "PSTaskDialog.dll"), Properties.Resources.PSTaskDialog);
                File.WriteAllBytes(Path.Combine(rootModFolder, "RocksmithToolkitLib.dll"), Properties.Resources.RocksmithToolkitLib);
                File.WriteAllBytes(Path.Combine(rootModFolder, "RocksmithToTabLib.dll"), Properties.Resources.RocksmithToTabLib);
                File.WriteAllBytes(Path.Combine(rootModFolder, "RSMods.exe"), Properties.Resources.RSMods);
                File.WriteAllText(Path.Combine(rootModFolder, "RSMods.exe.config"), Properties.Resources.RSMods_exe);
                File.WriteAllBytes(Path.Combine(rootModFolder, "RSMods.pdb"), Properties.Resources.RSMods1);
                File.WriteAllBytes(Path.Combine(rootModFolder, "SevenZipSharp.dll"), Properties.Resources.SevenZipSharp);
                File.WriteAllBytes(Path.Combine(rootModFolder, "SharpConfig.dll"), Properties.Resources.SharpConfig);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.Core.dll"), Properties.Resources.TwitchLib_Api_Core);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.Core.Enums.dll"), Properties.Resources.TwitchLib_Api_Core_Enums);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.Core.Interfaces.dll"), Properties.Resources.TwitchLib_Api_Core_Interfaces);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.Core.Models.dll"), Properties.Resources.TwitchLib_Api_Core_Models);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.dll"), Properties.Resources.TwitchLib_Api);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.Helix.dll"), Properties.Resources.TwitchLib_Api_Helix);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.Helix.Models.dll"), Properties.Resources.TwitchLib_Api_Helix_Models);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.V5.dll"), Properties.Resources.TwitchLib_Api_V5);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Api.V5.Models.dll"), Properties.Resources.TwitchLib_Api_V5_Models);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Client.dll"), Properties.Resources.TwitchLib_Client);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Client.Enums.dll"), Properties.Resources.TwitchLib_Client_Enums);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Client.Models.dll"), Properties.Resources.TwitchLib_Client_Models);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.Communication.dll"), Properties.Resources.TwitchLib_Communication);
                File.WriteAllBytes(Path.Combine(rootModFolder, "TwitchLib.PubSub.dll"), Properties.Resources.TwitchLib_PubSub);
                File.WriteAllBytes(Path.Combine(rootModFolder, "X360.dll"), Properties.Resources.X360);
                File.WriteAllBytes(Path.Combine(rootModFolder, "zlib.net.dll"), Properties.Resources.zlib_net);
                // ddc Folder
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc.exe"), Properties.Resources.ddc);
                File.WriteAllText(Path.Combine(ddcFolder, "ddc_chords_protector.xml"), Properties.Resources.ddc_chords_protector);
                File.WriteAllText(Path.Combine(ddcFolder, "ddc_chords_remover.xml"), Properties.Resources.ddc_chords_remover);
                File.WriteAllText(Path.Combine(ddcFolder, "ddc_dd_remover.xml"), Properties.Resources.ddc_dd_remover);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_default.cfg"), Properties.Resources.ddc_default);
                File.WriteAllText(Path.Combine(ddcFolder, "ddc_default.xml"), Properties.Resources.ddc_default1);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_keep_all_levels.cfg"), Properties.Resources.ddc_keep_all_levels);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_merge_all_levels.cfg"), Properties.Resources.ddc_merge_all_levels);
                File.WriteAllText(Path.Combine(ddcFolder, "license.txt"), Properties.Resources.license);
                File.WriteAllText(Path.Combine(ddcFolder, "readme.txt"), Properties.Resources.readme1);
                // tools Folder
                File.WriteAllBytes(Path.Combine(toolsFolder, "7za.exe"), Properties.Resources._7za);
                File.WriteAllBytes(Path.Combine(toolsFolder, "core.jar"), Properties.Resources.core);
                File.WriteAllBytes(Path.Combine(toolsFolder, "CreateToolkitShortcut.exe"), Properties.Resources.CreateToolkitShortcut);
                File.WriteAllBytes(Path.Combine(toolsFolder, "nvdxt.exe"), Properties.Resources.nvdxt);
                File.WriteAllBytes(Path.Combine(toolsFolder, "oggCut.exe"), Properties.Resources.oggCut);
                File.WriteAllBytes(Path.Combine(toolsFolder, "oggdec.exe"), Properties.Resources.oggdec);
                File.WriteAllBytes(Path.Combine(toolsFolder, "oggenc.exe"), Properties.Resources.oggenc);
                File.WriteAllText(Path.Combine(toolsFolder, "OpenCmd.bat"), Properties.Resources.OpenCmd);
                File.WriteAllBytes(Path.Combine(toolsFolder, "packed_codebooks.bin"), Properties.Resources.packed_codebooks);
                File.WriteAllBytes(Path.Combine(toolsFolder, "packed_codebooks_aoTuV_603.bin"), Properties.Resources.packed_codebooks_aoTuV_603);
                File.WriteAllText(Path.Combine(toolsFolder, "readme.txt"), Properties.Resources.readme);
                File.WriteAllBytes(Path.Combine(toolsFolder, "revorb.exe"), Properties.Resources.revorb);
                File.WriteAllBytes(Path.Combine(toolsFolder, "topng.exe"), Properties.Resources.topng);
                File.WriteAllBytes(Path.Combine(toolsFolder, "ww2ogg.exe"), Properties.Resources.ww2ogg);
            }
            catch (IOException ex)
            {
                if (ex.Message.Contains("cannot access"))
                    MessageBox.Show("Please make sure you don't have an instance of RSMods already running!", "Error");
                else
                    MessageBox.Show($"Error: {ex.Message}");

                return false;
            }

            return true;
        }
    }
} 
