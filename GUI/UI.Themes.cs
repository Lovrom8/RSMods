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
        private static readonly Color DefaultBackgroundColor = Color.Azure;
        private static readonly Color DefaultTextColor = Color.Black;
        private static readonly Color DefaultButtonColor = SystemColors.ControlLight;

        // Not taken from here :O https://stackoverflow.com/a/3419209
        private readonly List<Control> ControlList = []; // Don't make this readonly
        private void GetAllControls(Control container)
        {
            foreach (Control c in container.Controls)
            {
                GetAllControls(c);
                if (c is ListBox || c is GroupBox || c is TabPage || c is Button)
                    ControlList.Add(c);
            }
        }

        private void CustomTheme_ChangeTheme(Color backgroundColor, Color textColor, Color buttonColor)
        {
            GetAllControls(TabController);
            BackColor = backgroundColor; // MainForm BackColor
            ForeColor = textColor; // MainForm ForeColor

            foreach (Control controlToChange in ControlList)
            {
                controlToChange.ForeColor = textColor;

                if (controlToChange is Button)
                    controlToChange.BackColor = buttonColor;
                else
                    controlToChange.BackColor = backgroundColor;
            }

            CustomTheme_DataGridView(dgv_DefaultRewards, backgroundColor, textColor);
            CustomTheme_DataGridView(dgv_EnabledRewards, backgroundColor, textColor);
            CustomTheme_DataGridView(dgv_Profiles_Songlists, backgroundColor, textColor);

            // Twitch Log. Can't be done automatically or it will break other text boxes :(
            textBox_TwitchLog.ForeColor = textColor;
            textBox_TwitchLog.BackColor = backgroundColor;
        }

        private void CustomTheme_DataGridView(DataGridView grid, Color backgroundColor, Color textColor)
        {
            grid.EnableHeadersVisualStyles = false; // Allows us to customize the color scheme

            // Background Colors
            grid.BackgroundColor = backgroundColor;
            grid.ColumnHeadersDefaultCellStyle.BackColor = backgroundColor;
            grid.ColumnHeadersDefaultCellStyle.SelectionBackColor = backgroundColor;
            grid.DefaultCellStyle.SelectionBackColor = backgroundColor;

            // Foreground Colors
            grid.ForeColor = textColor;
            grid.DefaultCellStyle.SelectionForeColor = textColor;
            grid.ColumnHeadersDefaultCellStyle.SelectionForeColor = textColor;
            grid.ColumnHeadersDefaultCellStyle.ForeColor = textColor;
        }

        private void CustomTheme_LoadCustomColors()
        {
            Color backColor = DefaultBackgroundColor, foreColor = DefaultTextColor, buttonColor = DefaultButtonColor;

            if (GUISettings.CustomTheme)
            {
                if (GUISettings.ThemeBackgroundColor != String.Empty)
                    backColor = ColorTranslator.FromHtml("#" + GUISettings.ThemeBackgroundColor);

                if (GUISettings.ThemeTextColor != String.Empty)
                    foreColor = ColorTranslator.FromHtml("#" + GUISettings.ThemeTextColor);

                if (GUISettings.ThemeButtonColor != String.Empty)
                    buttonColor = ColorTranslator.FromHtml("#" + GUISettings.ThemeButtonColor);
            }

            textBox_ChangeBackgroundColor.BackColor = backColor;
            textBox_ChangeTextColor.BackColor = foreColor;
            textBox_ChangeButtonColor.BackColor = buttonColor;

            CustomTheme_ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor, textBox_ChangeButtonColor.BackColor);
        }

        private void CustomTheme_ChangeTheme(object sender, EventArgs e)
        {
            GUISettings.CustomTheme = checkBox_ChangeTheme.Checked;
            groupBox_ChangeTheme.Visible = checkBox_ChangeTheme.Checked;

            if (!checkBox_ChangeTheme.Checked) // Turning off custom themes
                CustomTheme_ChangeTheme(DefaultBackgroundColor, DefaultTextColor, DefaultButtonColor);
        }

        private void CustomTheme_ChangeBackgroundColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false,
                Color = DefaultBackgroundColor
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                GUISettings.ThemeBackgroundColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                textBox_ChangeBackgroundColor.BackColor = colorDialog.Color;
            }
        }

        private void CustomTheme_ChangeTextColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false,
                Color = DefaultTextColor
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                GUISettings.ThemeTextColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                textBox_ChangeTextColor.BackColor = colorDialog.Color;
            }
        }

        private void CustomTheme_ChangeButtonColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false,
                Color = DefaultButtonColor
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                GUISettings.ThemeButtonColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                textBox_ChangeButtonColor.BackColor = colorDialog.Color;
            }
        }

        private void CustomTheme_Apply(object sender, EventArgs e) => CustomTheme_ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor, textBox_ChangeButtonColor.BackColor);

        private void CustomTheme_Reset(object sender, EventArgs e)
        {
            CustomTheme_ChangeTheme(DefaultBackgroundColor, DefaultTextColor, DefaultButtonColor);

            GUISettings.ThemeBackgroundColor = (DefaultBackgroundColor.ToArgb() & 0x00ffffff).ToString("X6");
            GUISettings.ThemeTextColor = (DefaultTextColor.ToArgb() & 0x00ffffff).ToString("X6");
            GUISettings.ThemeButtonColor = (DefaultButtonColor.ToArgb() & 0x00ffffff).ToString("X6");

            textBox_ChangeBackgroundColor.BackColor = DefaultBackgroundColor;
            textBox_ChangeTextColor.BackColor = DefaultTextColor;
            textBox_ChangeButtonColor.BackColor = DefaultButtonColor;
        }

        private void Fonts_Load() // Not modified from here: https://stackoverflow.com/a/8657854 :eyes:
        {
            var installedFontFamilies = new InstalledFontCollection().Families.Select(f => f.Name).ToArray();
            listBox_AvailableFonts.Items.AddRange(installedFontFamilies);

            listBox_AvailableFonts.SelectedItem = Toggles.OnScreenFont;
        }

        private void Fonts_Change(object sender, EventArgs e)
        {
            string fontName = listBox_AvailableFonts.SelectedItem.ToString();
            Font newFontSelected = new(fontName, 10.0f, Font.Style, Font.Unit);
            label_FontTestCAPITALS.Font = newFontSelected;
            label_FontTestlowercase.Font = newFontSelected;
            label_FontTestNumbers.Font = newFontSelected;

            Toggles.OnScreenFont = fontName;
        }

        private void TrackBar_FontSize_Scroll(object sender, EventArgs e) => lblCurrentFontSize.Text = $"Current font size: {trackBar_FontSize.Value}";

        private void BtnSaveFontSize_Click(object sender, EventArgs e) => Toggles.OnScreenFontSize = trackBar_FontSize.Value;
    }
}
