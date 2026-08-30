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
        // Config
        private void ASIO_WASAPI_Output(object sender, EventArgs e) =>
            _asioSettings.Config.WasapiOutputs = checkBox_ASIO_WASAPI_Output.CheckState switch
            {
                CheckState.Checked => WasapiOutputMode.On,
                CheckState.Indeterminate => WasapiOutputMode.Prompt,
                _ => WasapiOutputMode.Off
            };
        private void ASIO_WASAPI_Input(object sender, EventArgs e) => _asioSettings.Config.EnableWasapiInputs = checkBox_ASIO_WASAPI_Input.Checked;
        private void ASIO_ASIO(object sender, EventArgs e) => _asioSettings.Config.EnableAsio = checkBox_ASIO_ASIO.Checked;

        // Driver
        private void ASIO_ListAvailableInput0(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Input0.SelectedItem != null)
                _asioSettings.Input0.Driver = listBox_AvailableASIODevices_Input0.SelectedItem.ToString();
        }

        private void ASIO_ListAvailableInput1(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Input1.SelectedItem != null)
                _asioSettings.Input1.Driver = listBox_AvailableASIODevices_Input1.SelectedItem.ToString();
        }

        private void ASIO_ListAvailableOutput(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Output.SelectedItem != null)
                _asioSettings.Output.Driver = listBox_AvailableASIODevices_Output.SelectedItem.ToString();
        }

        private void ASIO_ListAvailableInputMic(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_InputMic.SelectedItem != null)
                _asioSettings.InputMic.Driver = listBox_AvailableASIODevices_InputMic.SelectedItem.ToString();
        }

        // Disable / Comment Out Driver
        private void ASIO_Output_Disable(object sender, EventArgs e)
        {
            _asioSettings.Output.Disabled = checkBox_ASIO_Output_Disabled.Checked;
            if (!checkBox_ASIO_Output_Disabled.Checked && listBox_AvailableASIODevices_Output.SelectedItem != null)
                _asioSettings.Output.Driver = listBox_AvailableASIODevices_Output.SelectedItem.ToString();
        }

        private void ASIO_Input0_Disable(object sender, EventArgs e)
        {
            _asioSettings.Input0.Disabled = checkBox_ASIO_Input0_Disabled.Checked;
            if (!checkBox_ASIO_Input0_Disabled.Checked && listBox_AvailableASIODevices_Input0.SelectedItem != null)
                _asioSettings.Input0.Driver = listBox_AvailableASIODevices_Input0.SelectedItem.ToString();
        }

        private void ASIO_Input1_Disable(object sender, EventArgs e)
        {
            _asioSettings.Input1.Disabled = checkBox_ASIO_Input1_Disabled.Checked;
            if (!checkBox_ASIO_Input1_Disabled.Checked && listBox_AvailableASIODevices_Input1.SelectedItem != null)
                _asioSettings.Input1.Driver = listBox_AvailableASIODevices_Input1.SelectedItem.ToString();
        }

        private void ASIO_InputMic_Disable(object sender, EventArgs e)
        {
            _asioSettings.InputMic.Disabled = checkBox_ASIO_InputMic_Disabled.Checked;
            if (!checkBox_ASIO_InputMic_Disabled.Checked && listBox_AvailableASIODevices_InputMic.SelectedItem != null)
                _asioSettings.InputMic.Driver = listBox_AvailableASIODevices_InputMic.SelectedItem.ToString();
        }

        // Buffer Size
        private void ASIO_BufferSize_Driver(object sender, EventArgs e)
        {
            if (radio_ASIO_BufferSize_Driver.Checked)
                _asioSettings.AsioSection.BufferSizeMode = RsAsioLimits.BufferModeDriver;
        }

        private void ASIO_BufferSize_Host(object sender, EventArgs e)
        {
            if (radio_ASIO_BufferSize_Host.Checked)
                _asioSettings.AsioSection.BufferSizeMode = RsAsioLimits.BufferModeHost;
        }

        private void ASIO_BufferSize_Custom(object sender, EventArgs e)
        {
            label_ASIO_CustomBufferSize.Visible = radio_ASIO_BufferSize_Custom.Checked;
            nUpDown_ASIO_CustomBufferSize.Visible = radio_ASIO_BufferSize_Custom.Checked;
            _asioSettings.AsioSection.BufferSizeMode = RsAsioLimits.BufferModeCustom;
        }

        private void ASIO_CustomBufferSize(object sender, EventArgs e) => _asioSettings.AsioSection.CustomBufferSize = (int)nUpDown_ASIO_CustomBufferSize.Value;

        // Input0 Settings
        private void ASIO_Input0_Channel(object sender, EventArgs e) => _asioSettings.Input0.Channel = (int)nUpDown_ASIO_Input0_Channel.Value;
        private void ASIO_Input0_MaxVolume(object sender, EventArgs e) => _asioSettings.Input0.SoftwareMasterVolumePercent = (int)nUpDown_ASIO_Input0_MaxVolume.Value;
        private void ASIO_Input0_MasterVolume(object sender, EventArgs e)
        {
            _asioSettings.Input0.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Input0_ControlMasterVolume.Checked;
            label_ASIO_Input0_MaxVolume.Visible = checkBox_ASIO_Input0_ControlMasterVolume.Checked;
            nUpDown_ASIO_Input0_MaxVolume.Visible = checkBox_ASIO_Input0_ControlMasterVolume.Checked;
        }
        private void ASIO_Input0_EndpointVolume(object sender, EventArgs e) => _asioSettings.Input0.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Input0_ControlEndpointVolume.Checked;
        private void ASIO_Input0_EnableRefHack(object sender, EventArgs e) => _asioSettings.Input0.EnableRefCountHack = checkBox_ASIO_Input0_EnableRefHack.Checked;

        // Input1 Settings
        private void ASIO_Input1_Channel(object sender, EventArgs e) => _asioSettings.Input1.Channel = (int)nUpDown_ASIO_Input1_Channel.Value;
        private void ASIO_Input1_MaxVolume(object sender, EventArgs e) => _asioSettings.Input1.SoftwareMasterVolumePercent = (int)nUpDown_ASIO_Input1_MaxVolume.Value;
        private void ASIO_Input1_MasterVolume(object sender, EventArgs e)
        {
            _asioSettings.Input1.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Input1_ControlMasterVolume.Checked;
            label_ASIO_Input1_MaxVolume.Visible = checkBox_ASIO_Input1_ControlMasterVolume.Checked;
            nUpDown_ASIO_Input1_MaxVolume.Visible = checkBox_ASIO_Input1_ControlMasterVolume.Checked;
        }
        private void ASIO_Input1_EndpointVolume(object sender, EventArgs e) => _asioSettings.Input1.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Input1_ControlEndpointVolume.Checked;
        private void ASIO_Input1_EnableRefHack(object sender, EventArgs e) => _asioSettings.Input1.EnableRefCountHack = checkBox_ASIO_Input1_EnableRefHack.Checked;

        // Output Settings
        private void ASIO_Output_BaseChannel(object sender, EventArgs e) => _asioSettings.Output.BaseChannel = (int)nUpDown_ASIO_Output_BaseChannel.Value;
        private void ASIO_Output_AltBaseChannel(object sender, EventArgs e) => _asioSettings.Output.AltBaseChannel = (int)nUpDown_ASIO_Output_AltBaseChannel.Value;
        private void ASIO_Output_MaxVolume(object sender, EventArgs e) => _asioSettings.Output.SoftwareMasterVolumePercent = (int)nUpDown_ASIO_Output_MaxVolume.Value;
        private void ASIO_Output_MasterVolume(object sender, EventArgs e)
        {
            _asioSettings.Output.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Output_ControlMasterVolume.Checked;
            label_ASIO_Output_MaxVolume.Visible = checkBox_ASIO_Output_ControlMasterVolume.Checked;
            nUpDown_ASIO_Output_MaxVolume.Visible = checkBox_ASIO_Output_ControlMasterVolume.Checked;
        }
        private void ASIO_Output_EndpointVolume(object sender, EventArgs e) => _asioSettings.Output.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Output_ControlEndpointVolume.Checked;
        private void ASIO_Output_EnableRefHack(object sender, EventArgs e) => _asioSettings.Output.EnableRefCountHack = checkBox_ASIO_Output_EnableRefHack.Checked;

        // InputMic Settings
        private void ASIO_InputMic_Channel(object sender, EventArgs e) => _asioSettings.InputMic.Channel = (int)nUpDown_ASIO_InputMic_Channel.Value;
        private void ASIO_InputMic_MaxVolume(object sender, EventArgs e) => _asioSettings.InputMic.SoftwareMasterVolumePercent = (int)nUpDown_ASIO_InputMic_MaxVolume.Value;
        private void ASIO_InputMic_MasterVolume(object sender, EventArgs e)
        {
            _asioSettings.InputMic.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_InputMic_ControlMasterVolume.Checked;
            label_ASIO_InputMic_MaxVolume.Visible = checkBox_ASIO_InputMic_ControlMasterVolume.Checked;
            nUpDown_ASIO_InputMic_MaxVolume.Visible = checkBox_ASIO_InputMic_ControlMasterVolume.Checked;
        }
        private void ASIO_InputMic_EndpointVolume(object sender, EventArgs e) => _asioSettings.InputMic.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_InputMic_ControlEndpointVolume.Checked;
        private void ASIO_InputMic_EnableRefHack(object sender, EventArgs e) => _asioSettings.InputMic.EnableRefCountHack = checkBox_ASIO_InputMic_EnableRefHack.Checked;

        // Clear Selection
        private void ASIO_ClearSelectedDevice(ListBox deviceList, EventHandler e, Action clearDriverSetting)
        {
            deviceList.SelectedIndexChanged -= e;
            deviceList.SelectedIndex = -1;
            clearDriverSetting();
            deviceList.SelectedIndexChanged += e;
        }
        private void ASIO_Input0_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Input0, ASIO_ListAvailableInput0, () => _asioSettings.Input0.Driver = "");
        private void ASIO_Input1_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Input1, ASIO_ListAvailableInput1, () => _asioSettings.Input1.Driver = "");
        private void ASIO_Output_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Output, ASIO_ListAvailableOutput, () => _asioSettings.Output.Driver = "");
        private void ASIO_InputMic_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_InputMic, ASIO_ListAvailableInputMic, () => _asioSettings.InputMic.Driver = "");
        private void ASIO_OpenGithub(object sender, EventArgs e) => Process.Start("https://github.com/mdias/rs_asio");
    }
}
