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
        private void Midi_LoadDevices()
        {
            listBox_ListMidiOutDevices.Items.Clear();
            listBox_ListMidiInDevices.Items.Clear();

            foreach (string name in Midi.GetOutputDeviceNames())
                listBox_ListMidiOutDevices.Items.Add(name);

            foreach (string name in Midi.GetInputDeviceNames())
                listBox_ListMidiInDevices.Items.Add(name);

            if (!string.IsNullOrEmpty(Toggles.AutoTuneForSongDevice))
                listBox_ListMidiOutDevices.SelectedItem = Toggles.AutoTuneForSongDevice;

            if (!string.IsNullOrEmpty(Toggles.MidiInDevice))
                listBox_ListMidiInDevices.SelectedItem = Toggles.MidiInDevice;
        }

        private void CheckBox_EnabledMidiIn_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_EnabledMidiIn.Checked)
            {
                if (listBox_ListMidiInDevices.SelectedIndex == -1)
                {
                    MessageBox.Show("Please select a MIDI Input device first.", "No Device Selected", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    checkBox_EnabledMidiIn.Checked = false;
                    return;
                }

                Midi.SelectedMidiInDeviceId = (uint)listBox_ListMidiInDevices.SelectedIndex;

                Debug.WriteLine($"Opening MIDI In Device ID: {Midi.SelectedMidiInDeviceId}...");

                Midi.MidiInProcessing = MidiDebug.MidiInProc;

                uint openResult = Midi.midiInOpen(ref Midi.MidiInHandle, Midi.SelectedMidiInDeviceId.Value, Midi.MidiInProcessing, IntPtr.Zero, Midi.CALLBACK_FUNCTION);

                if (openResult != (uint)Midi.MMRESULT.MMSYSERR_NOERROR)
                {
                    MessageBox.Show($"Failed to open MIDI port. Error code: {openResult}");
                    checkBox_EnabledMidiIn.Checked = false;
                    return;
                }

                Midi.midiInStart(Midi.MidiInHandle);
                Debug.WriteLine("MIDI In Started.");
            }
            else
            {
                ShutdownMidi();
            }
        }

        private void ShutdownMidi()
        {
            if (Midi.MidiInHandle != IntPtr.Zero)
            {
                Debug.WriteLine("Shutting down MIDI In...");
                Midi.midiInStop(Midi.MidiInHandle);
                Midi.midiInClose(Midi.MidiInHandle);

                Midi.MidiInHandle = IntPtr.Zero;
                Midi.SelectedMidiInDeviceId = null;
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            ShutdownMidi();
            _twitchAuthorizationCancellation?.Cancel();
            TwitchRuntime.StopAsync().GetAwaiter().GetResult();
            _twitchAuthorizationCancellation?.Dispose();
        }
    }
}
