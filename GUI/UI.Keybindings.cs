using System;
using System.Windows.Forms;
using System.IO;
using System.Linq;
using System.Drawing;
using System.Drawing.Text;
using RSMods.Data;
using RSMods.Util;
using System.Reflection;
using System.Collections.Generic;
using RocksmithToolkitLib.Extensions;
using RSMods.Twitch;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;
using RocksmithToolkitLib.DLCPackage;
using NAudio.CoreAudioApi;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using ArrangementTuning = Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning;
using static RSMods.RsModsSettings;
using RSMods.ASIO;
using RSMods.Rocksmith;
using System.Runtime;

namespace RSMods
{
    public partial class MainForm : Form
    {
        private void Keypress_CheckDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = true;
                Save_Songlists_Keybindings(sender, e);
                return;
            }

            if (sender is TextBox textBox &&
               (textBox == textBox_NewKeyAssignment_MODS || textBox == textBox_NewKeyAssignment_AUDIO))
            {
                HandleKeyAssignment(textBox, e);
            }
        }

        private void HandleKeyAssignment(TextBox textBox, KeyEventArgs e)
        {
            e.SuppressKeyPress = true;

            if (KeyConversion.KeyDownDictionary.Contains(e.KeyCode))
            {
                textBox.Text = e.KeyCode.ToString();
            }
            else if (KeyConversion.IsRocksmithReservedKey(e.KeyCode))
            {
                var result = MessageBox.Show(
                    "The key you entered is currently used by Rocksmith and may interfere with being able to use the game properly. Are you sure you want to use this keybinding?",
                    "Keybinding Warning!",
                    MessageBoxButtons.OKCancel,
                    MessageBoxIcon.Warning);

                if (result == DialogResult.OK)
                {
                    textBox.Text = e.KeyCode.ToString();
                }
                else
                {
                    textBox.Text = string.Empty;
                }
            }
        }

        private void Keypress_CheckUp(object sender, KeyEventArgs e)
        {
            if (KeyConversion.KeyUpDictionary.Contains(e.KeyCode))
            {
                if (sender == textBox_NewKeyAssignment_MODS)
                    textBox_NewKeyAssignment_MODS.Text = e.KeyCode.ToString();
                else if (sender == textBox_NewKeyAssignment_AUDIO)
                    textBox_NewKeyAssignment_AUDIO.Text = e.KeyCode.ToString();
            }
        }

        private void Keypress_CheckMouse(object sender, MouseEventArgs e)
        {
            if (KeyConversion.MouseButtonDictionary.Contains(e.Button))
            {
                if (sender == textBox_NewKeyAssignment_MODS)
                    textBox_NewKeyAssignment_MODS.Text = e.Button.ToString();
                else if (sender == textBox_NewKeyAssignment_AUDIO)
                    textBox_NewKeyAssignment_AUDIO.Text = e.Button.ToString();
            }

        }

        private void Keypress_LoadKeys(object sender, EventArgs e)
        {
            if (listBox_Modlist_MODS.SelectedIndex != -1)
            {
                textBox_NewKeyAssignment_MODS.Text = KeyConversion.VKeyToUI(Dictionaries.ModKeybinds[listBox_Modlist_MODS.SelectedIndex].GetKey());
            }
        }

        private void Keypress_LoadVolumes(object sender, EventArgs e)
        {
            if (listBox_Modlist_AUDIO.SelectedIndex != -1)
            {
                textBox_NewKeyAssignment_AUDIO.Text = KeyConversion.VKeyToUI(Dictionaries.AudioKeybinds[listBox_Modlist_AUDIO.SelectedIndex].GetKey());
            }
        }
    }
}
