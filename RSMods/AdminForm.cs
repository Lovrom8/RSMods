using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;
using RSMods.Util;
using RSMods.Data;
namespace RSMods
{
    public partial class AdminForm : Form
    {
        public AdminForm()
        {
            InitializeComponent();
            checkBox_SpeedUpEnumeration.Checked = ReadSettings.ProcessSettings(ReadSettings.SpeedUpLoadupIdentifier) == "on";
        }

        private void Save_SpeedUpEnumeration(object sender, EventArgs e)
        {
            string rocksmithFolder = Constants.RSFolder;
            string rocksmithLocation = Path.Combine(rocksmithFolder, "Rocksmith2014.exe");


            Clipboard.SetText(rocksmithLocation);

            if (checkBox_SpeedUpEnumeration.Checked)
            {
                Process.Start("netsh.exe", $"advfirewall firewall add rule name=\"RSMods_SpeedUpEnumeration\" dir=in action=block profile=any program=\"{rocksmithLocation}\"");
                Process.Start("netsh.exe", $"advfirewall firewall add rule name=\"RSMods_SpeedUpEnumeration\" dir=out action=block profile=any program=\"{rocksmithLocation}\"");
            }
            else
            {
                Process.Start("netsh.exe", "advfirewall firewall delete rule name=\"RSMods_SpeedUpEnumeration\" dir=in");
                Process.Start("netsh.exe", "advfirewall firewall delete rule name=\"RSMods_SpeedUpEnumeration\" dir=out");
            }

            Save_Settings(ReadSettings.SpeedUpLoadupIdentifier, checkBox_SpeedUpEnumeration.Checked.ToString().ToLower());

        }

        private void Save_Settings(string IdentifierToChange, string ChangedSettingValue)
        {
            // Right before launch, we switched from the boolean names of (true / false) to (on / off) for users to be able to edit the mods without the GUI (by hand).
            if (ChangedSettingValue == "true")
                ChangedSettingValue = "on";
            else if (ChangedSettingValue == "false")
                ChangedSettingValue = "off";

            foreach (string section in WriteSettings.saveSettingsOrDefaults.Keys)
            {
                foreach (KeyValuePair<string, string> entry in WriteSettings.saveSettingsOrDefaults[section])
                {
                    if (IdentifierToChange == entry.Key)
                    {
                        WriteSettings.saveSettingsOrDefaults[section][IdentifierToChange] = ChangedSettingValue;
                        break; // We found what we need, so let's leave.
                    }
                }
            }

            WriteSettings.WriteINI(WriteSettings.saveSettingsOrDefaults);
        }
    }
}
