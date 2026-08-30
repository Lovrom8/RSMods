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
        private void GuitarSpeak_Enable(object sender, EventArgs e)
        {
            groupBox_GuitarSpeak.Visible = checkBox_GuitarSpeak.Checked;
            checkBox_GuitarSpeakWhileTuning.Visible = checkBox_GuitarSpeak.Checked;
            Toggles.GuitarSpeak = checkBox_GuitarSpeak.Checked;
        }

        private void GuitarSpeak_Save(object sender, EventArgs e)
        {
            if (listBox_GuitarSpeakNote.SelectedIndex >= 0 && listBox_GuitarSpeakOctave.SelectedIndex >= 0 && listBox_GuitarSpeakKeypress.SelectedIndex >= 0)
            {
                int inputNote = listBox_GuitarSpeakNote.SelectedIndex + 36; // We skip the first 3 octaves to give an accurate representation of the notes being played
                int inputOctave = listBox_GuitarSpeakOctave.SelectedIndex - 3; // -1 for the offset, and -2 for octave offset in DLL.
                int outputNoteOctave = inputNote + (inputOctave * 12);

                MessageBox.Show($"{listBox_GuitarSpeakNote.SelectedItem}{listBox_GuitarSpeakOctave.SelectedItem} was saved to {listBox_GuitarSpeakKeypress.SelectedItem}", "Note Saved!", MessageBoxButtons.OK, MessageBoxIcon.Information);

                int index = listBox_GuitarSpeakKeypress.SelectedIndex;
                Dictionaries.GuitarSpeakKeybinds[index].SetKey(outputNoteOctave.ToString());
                GuitarSpeak_ResetPresets();

                listBox_GuitarSpeakNote.ClearSelected();
                listBox_GuitarSpeakOctave.ClearSelected();
                listBox_GuitarSpeakKeypress.ClearSelected();
            }
            else
            {
                MessageBox.Show("One, or more, of the Guitar Speak boxes not selected", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void GuitarSpeak_WhileTuning(object sender, EventArgs e) => RsModsSettings.GuitarSpeak.GuitarSpeakWhileTuning = checkBox_GuitarSpeakWhileTuning.Checked;

        private void GuitarSpeak_Help(object sender, EventArgs e) => Process.Start("https://pastebin.com/raw/PZ0FQTn0");

        private void GuitarSpeak_ResetPresets()
        {
            listBox_GuitarSpeakSaved.Items.Clear();

            foreach (var keybind in Dictionaries.GuitarSpeakKeybinds)
                listBox_GuitarSpeakSaved.Items.Add($"{keybind.DisplayName}: {GuitarSpeak.GuitarSpeakNoteOctaveMath(keybind.GetKey())}");
        }

        private void GuitarSpeak_ClearSavedValue(object sender, EventArgs e)
        {
            int valueToRemove = listBox_GuitarSpeakSaved.SelectedIndex;

            if (valueToRemove == -1)
                return;

            listBox_GuitarSpeakSaved.SelectedIndex = -1;

            Dictionaries.GuitarSpeakKeybinds[valueToRemove].SetKey("");

            GuitarSpeak_ResetPresets();
        }
    }
}
