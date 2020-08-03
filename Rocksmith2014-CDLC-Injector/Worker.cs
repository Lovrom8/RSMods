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
        public static void InjectDLL(string rocksmithLocation)
        {
            File.WriteAllBytes(Path.Combine(@rocksmithLocation, "D3DX9_42.dll"), Properties.Resources.D3DX9_42);
            MessageBox.Show("CDLC should now work in your game. Make sure to purchase Cherub Rock on Steam if they don't show up", "Congrats!", MessageBoxButtons.OK, MessageBoxIcon.Information);
            System.Diagnostics.Process.Start("https://store.steampowered.com/app/248750/");
            Environment.Exit(1);
        }

        public static void InjectGUI(string rocksmithLocation)
        {
            string rootModFolder = Path.Combine(rocksmithLocation, "RSMods");
            string customModsFolder = Path.Combine(rootModFolder, "CustomMods");
            string ddcFolder = Path.Combine(rootModFolder, "ddc");
            string resourcesFolder = Path.Combine(rootModFolder, "Resources");
            string toolsFolder = Path.Combine(rootModFolder, "tools");

            Directory.CreateDirectory(rootModFolder);
            Directory.CreateDirectory(customModsFolder);
            Directory.CreateDirectory(ddcFolder);
            Directory.CreateDirectory(resourcesFolder);
            Directory.CreateDirectory(toolsFolder);

            // Root Folder
                File.WriteAllBytes(Path.Combine(rootModFolder, "7z.dll"), Properties.Resources._7z);
                File.WriteAllBytes(Path.Combine(rootModFolder, "ICSharpCode.SharpZipLib.dll"), Properties.Resources.ICSharpCode_SharpZipLib);
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
                File.WriteAllBytes(Path.Combine(rootModFolder, "X360.dll"), Properties.Resources.X360);
                File.WriteAllBytes(Path.Combine(rootModFolder, "zlib.net.dll"), Properties.Resources.zlib_net);
                File.WriteAllText(Path.Combine(rootModFolder, "version.txt"), Properties.Resources.version);
            // ddc Folder
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc.exe"), Properties.Resources.ddc);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_chords_protector.xml"), Properties.Resources.ddc);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_chords_remover.xml"), Properties.Resources.ddc);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_dd_remover.xml"), Properties.Resources.ddc);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_default.cfg"), Properties.Resources.ddc);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_default.xml"), Properties.Resources.ddc);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_keep_all_levels.cfg"), Properties.Resources.ddc);
                File.WriteAllBytes(Path.Combine(ddcFolder, "ddc_merge_all_levels.cfg"), Properties.Resources.ddc);
                File.WriteAllText(Path.Combine(ddcFolder, "license.txt"), Properties.Resources.license);
                File.WriteAllText(Path.Combine(ddcFolder, "readme.txt"), Properties.Resources.readme1);
            // Resources Folder
                File.WriteAllBytes(Path.Combine(resourcesFolder, "introsequence.gfx"), Properties.Resources.introsequence);
                File.WriteAllBytes(Path.Combine(resourcesFolder, "introsequence_original.gfx"), Properties.Resources.introsequence_original);
                File.WriteAllText(Path.Combine(resourcesFolder, "maingame.csv"), Properties.Resources.maingame);
                File.WriteAllText(Path.Combine(resourcesFolder, "maingame_original.csv"), Properties.Resources.maingame_original);
                File.WriteAllBytes(Path.Combine(resourcesFolder, "sltsv1_aggregategraph.nt"), Properties.Resources.sltsv1_aggregategraph);
                File.WriteAllBytes(Path.Combine(resourcesFolder, "tuning.database.json"), Properties.Resources.tuning_database);
                File.WriteAllBytes(Path.Combine(resourcesFolder, "ui_menu_pillar_main.database.json"), Properties.Resources.ui_menu_pillar_main_database);
                File.WriteAllBytes(Path.Combine(resourcesFolder, "ui_menu_pillar_mission.database.json"), Properties.Resources.ui_menu_pillar_mission_database);
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
    }
} 
