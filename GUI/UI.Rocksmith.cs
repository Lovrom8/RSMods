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
        // Audio Settings
        private void Rocksmith_EnableMicrophone(object sender, EventArgs e) => _rocksmithSettings.Audio.EnableMicrophone = checkBox_Rocksmith_EnableMicrophone.Checked;
        private void Rocksmith_ExclusiveMode(object sender, EventArgs e) => _rocksmithSettings.Audio.ExclusiveMode = checkBox_Rocksmith_ExclusiveMode.Checked;
        private void Rocksmith_LatencyBuffer(object sender, EventArgs e) => _rocksmithSettings.Audio.LatencyBuffer = (int)nUpDown_Rocksmith_LatencyBuffer.Value;
        private void Rocksmith_ForceWDM(object sender, EventArgs e) => _rocksmithSettings.Audio.ForceWDM = checkBox_Rocksmith_ForceWDM.Checked;
        private void Rocksmith_ForceDirextXSink(object sender, EventArgs e) => _rocksmithSettings.Audio.ForceDirectXSink = checkBox_Rocksmith_ForceDirextXSink.Checked;
        private void Rocksmith_DumpAudioLog(object sender, EventArgs e) => _rocksmithSettings.Audio.DumpAudioLog = checkBox_Rocksmith_DumpAudioLog.Checked;
        private void Rocksmith_MaxBufferSize(object sender, EventArgs e)
        {
            _rocksmithSettings.Audio.MaxOutputBufferSize = (int)nUpDown_Rocksmith_MaxOutputBuffer.Value;
            if (nUpDown_Rocksmith_MaxOutputBuffer.Value == 0)
                checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked = true;
        }
        private void Rocksmith_RTCOnly(object sender, EventArgs e) => _rocksmithSettings.Audio.RealToneCableOnly = checkBox_Rocksmith_RTCOnly.Checked;
        private void Rocksmith_LowLatencyMode(object sender, EventArgs e) => _rocksmithSettings.Audio.Win32UltraLowLatencyMode = checkBox_Rocksmith_LowLatencyMode.Checked;

        private void Rocksmith_AutomateMaxBufferSize(object sender, EventArgs e)
        {
            nUpDown_Rocksmith_MaxOutputBuffer.Enabled = !checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked;
            nUpDown_Rocksmith_MaxOutputBuffer.Value = checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked ? 0 : 32;
        }

        // Visual Settings
        private void Rocksmith_GamepadUI(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.ShowGamepadUI = checkBox_Rocksmith_GamepadUI.Checked;
        private void Rocksmith_ScreenWidth(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.ScreenWidth = (int)nUpDown_Rocksmith_ScreenWidth.Value;
        private void Rocksmith_ScreenHeight(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.ScreenHeight = (int)nUpDown_Rocksmith_ScreenHeight.Value;
        private void Rocksmith_Windowed(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.Fullscreen = FullscreenMode.Windowed;
        private void Rocksmith_NonExclusiveFullScreen(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.Fullscreen = FullscreenMode.NonExclusive;
        private void Rocksmith_ExclusiveFullScreen(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.Fullscreen = FullscreenMode.Exclusive;
        private void Rocksmith_LowQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = false;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;

            _rocksmithSettings.RendererWin32.VisualQuality = VisualQualityMode.Low;
        }
        private void Rocksmith_MediumQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = true;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;

            _rocksmithSettings.RendererWin32.VisualQuality = VisualQualityMode.Medium;
        }
        private void Rocksmith_HighQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = true;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;

            _rocksmithSettings.RendererWin32.VisualQuality = VisualQualityMode.High;
        }

        private void Rocksmith_CustomQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Enabled = true;
            checkBox_Rocksmith_PostEffects.Enabled = true;
            checkBox_Rocksmith_HighResScope.Enabled = true;

            _rocksmithSettings.RendererWin32.VisualQuality = VisualQualityMode.Custom;
        }
        private void Rocksmith_RenderWidth(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.RenderingWidth = (int)nUpDown_Rocksmith_RenderWidth.Value;
        private void Rocksmith_RenderHeight(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.RenderingHeight = (int)nUpDown_Rocksmith_RenderHeight.Value;
        private void Rocksmith_PostEffects(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.EnablePostEffects = checkBox_Rocksmith_PostEffects.Checked;
        private void Rocksmith_Shadows(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.EnableShadows = checkBox_Rocksmith_Shadows.Checked;
        private void Rocksmith_HighResScope(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.EnableHighResScope = checkBox_Rocksmith_HighResScope.Checked;
        private void Rocksmith_DepthOfField(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.EnableDepthOfField = checkBox_Rocksmith_DepthOfField.Checked;
        private void Rocksmith_PerPixelLighting(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.EnablePerPixelLighting = checkBox_Rocksmith_PerPixelLighting.Checked;
        private void Rocksmith_MSAA(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.MsaaSamples = checkBox_Rocksmith_MSAASamples.Checked ? MsaaMode.X4 : MsaaMode.Off;
        private void Rocksmith_DisableBrowser(object sender, EventArgs e) => _rocksmithSettings.RendererWin32.DisableBrowser = checkBox_Rocksmith_DisableBrowser.Checked;

        private void Rocksmith_EnableRenderRes(object sender, EventArgs e)
        {
            label_Rocksmith_RenderWidth.Visible = checkBox_Rocksmith_EnableRenderRes.Checked;
            label_Rocksmith_RenderHeight.Visible = checkBox_Rocksmith_EnableRenderRes.Checked;
            nUpDown_Rocksmith_RenderWidth.Visible = checkBox_Rocksmith_EnableRenderRes.Checked;
            nUpDown_Rocksmith_RenderHeight.Visible = checkBox_Rocksmith_EnableRenderRes.Checked;

            if (!checkBox_Rocksmith_EnableRenderRes.Checked)
            {
                nUpDown_Rocksmith_RenderWidth.Value = 0;
                nUpDown_Rocksmith_RenderHeight.Value = 0;
            }
        }

        // Network Settings
        private void Rocksmith_UseProxy(object sender, EventArgs e) => _rocksmithSettings.Net.UseProxy = checkBox_Rocksmith_UseProxy.Checked;
    }
}
