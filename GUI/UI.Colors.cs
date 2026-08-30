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
        private void StringColors_ChangeStringColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false
            };
            bool isNormalStrings = radio_DefaultStringColors.Checked; // True = Normal, False = Colorblind
            int stringNumber = -1;
            string senderText = sender.ToString();

            if (senderText.Contains("E String")) stringNumber = 0;
            else if (senderText.Contains("A String")) stringNumber = 1;
            else if (senderText.Contains("D String")) stringNumber = 2;
            else if (senderText.Contains("G String")) stringNumber = 3;
            else if (senderText.Contains("B String")) stringNumber = 4;
            else if (senderText.Contains("e String")) stringNumber = 5;

            if (stringNumber == -1) return;

            StringColors_FillStringNumberToColorDictionary();

            colorDialog.Color = ColorTranslator.FromHtml("#" + StringColors.GetStringColor(stringNumber, isNormalStrings));

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                Toggles.CustomStringColors = CustomStringColorMode.Custom;
                string newColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                StringColors.SetStringColor(stringNumber, isNormalStrings, newColor);
                stringNumberToColorTextBox[stringNumber].BackColor = colorDialog.Color;
            }
        }

        private void OnSettingChanged()
        {
            if (!AllowSaving)
                return;

            RsModsSettings.Save();

            if (this.InvokeRequired)
            {
                this.Invoke(new Action(SaveSettings_ShowLabel));
            }
            else
            {
                SaveSettings_ShowLabel();
            }

            WinMsgUtil.SendMsgToRS("update all");
        }

        private void StringColors_ChangeNoteColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false
            };
            bool isNormalNotes = radio_DefaultNoteColors.Checked; // True = Normal, False = Colorblind
            int noteNumber = -1;
            string senderText = sender.ToString();

            if (senderText.Contains("E String")) noteNumber = 0;
            else if (senderText.Contains("A String")) noteNumber = 1;
            else if (senderText.Contains("D String")) noteNumber = 2;
            else if (senderText.Contains("G String")) noteNumber = 3;
            else if (senderText.Contains("B String")) noteNumber = 4;
            else if (senderText.Contains("e String")) noteNumber = 5;

            if (noteNumber == -1) return;

            StringColors_FillNoteNumberToColorDictionary();

            colorDialog.Color = ColorTranslator.FromHtml("#" + RsModsSettings.StringColors.GetNoteColor(noteNumber, isNormalNotes));

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                ModSettings.SeparateNoteColorsMode = NoteColorMode.Custom;
                string newColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                StringColors.SetNoteColor(noteNumber, isNormalNotes, newColor);
                stringNumberToColorTextBox[noteNumber].BackColor = colorDialog.Color;
            }
        }

        private void StringColors_LoadDefaultStringColors(bool colorBlind = false)
        {
            if (string.IsNullOrEmpty(StringColors.GetNoteColor(0, true))) // Fixes a small use case where the GUI moves faster than the writing of the INI.
            {
                RsModsSettings.Save();
                return;
            }

            var stringColorTextboxes = new[]
            {
                textBox_String0Color, textBox_String1Color, textBox_String2Color,
                textBox_String3Color, textBox_String4Color, textBox_String5Color
            };

            for (int i = 0; i < stringColorTextboxes.Length; i++)
            {
                string hexColor = $"#{StringColors.GetStringColor(i, !colorBlind)}";
                stringColorTextboxes[i].BackColor = ColorTranslator.FromHtml(hexColor);
            }
        }

        private void StringColors_LoadDefaultNoteColors(bool colorBlind = false)
        {
            if (string.IsNullOrEmpty(StringColors.GetNoteColor(0, true))) // Fixes a small use case where the GUI moves faster than the writing of the INI.
            {
                RsModsSettings.Save();
                return;
            }

            var noteColorTextBoxes = new[]
            {
                    textBox_Note0Color, textBox_Note1Color, textBox_Note2Color,
                    textBox_Note3Color, textBox_Note4Color, textBox_Note5Color
                };

            for (int i = 0; i < noteColorTextBoxes.Length; i++)
            {
                string hexColor = $"#{StringColors.GetNoteColor(i, !colorBlind)}";
                noteColorTextBoxes[i].BackColor = ColorTranslator.FromHtml(hexColor);
            }
        }

        private void StringColors_DefaultStringColors(object sender, EventArgs e) => StringColors_LoadDefaultStringColors();

        private void StringColors_ColorBlindStringColors(object sender, EventArgs e) => StringColors_LoadDefaultStringColors(true);

        private void StringColors_DefaultNoteColors(object sender, EventArgs e) => StringColors_LoadDefaultNoteColors();

        private void StringColors_ColorBlindNoteColors(object sender, EventArgs e) => StringColors_LoadDefaultNoteColors(true);

        private void NotewayColors_ChangeNotewayColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false
            };

            NotewayColors_FillNotewayButtonToColorDictionary();

            string senderStr = sender.ToString();
            var matchedColor = Dictionaries.NotewayColors.FirstOrDefault(c => senderStr.Contains(c.DisplayName));

            if (matchedColor != null)
            {
                string currentColor = matchedColor.GetColor();
                if (!string.IsNullOrEmpty(currentColor))
                    colorDialog.Color = ColorTranslator.FromHtml("#" + currentColor);

                if (colorDialog.ShowDialog() == DialogResult.OK)
                {
                    matchedColor.SetColor((colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                    notewayButtonToColorTextbox[(Button)sender].BackColor = colorDialog.Color;
                }
            }
        }

        private void NotewayColors_LoadDefaultStringColors()
        {
            if (HighwayColors.CustomHighwayNumbered != "")
                textBox_ShowNumberedFrets.BackColor = ColorTranslator.FromHtml("#" + HighwayColors.CustomHighwayNumbered);
            if (HighwayColors.CustomHighwayUnNumbered != "")
                textBox_ShowUnNumberedFrets.BackColor = ColorTranslator.FromHtml("#" + HighwayColors.CustomHighwayUnNumbered);
            if (HighwayColors.CustomHighwayGutter != "")
                textBox_ShowNotewayGutter.BackColor = ColorTranslator.FromHtml("#" + HighwayColors.CustomHighwayGutter);
            if (HighwayColors.CustomFretNubmers != "")
                textBox_ShowFretNumber.BackColor = ColorTranslator.FromHtml("#" + HighwayColors.CustomFretNubmers);
        }


        public Dictionary<int, TextBox> stringNumberToColorTextBox = [];
        private void StringColors_FillStringNumberToColorDictionary()
        {
            stringNumberToColorTextBox.Clear();

            stringNumberToColorTextBox.Add(0, textBox_String0Color);
            stringNumberToColorTextBox.Add(1, textBox_String1Color);
            stringNumberToColorTextBox.Add(2, textBox_String2Color);
            stringNumberToColorTextBox.Add(3, textBox_String3Color);
            stringNumberToColorTextBox.Add(4, textBox_String4Color);
            stringNumberToColorTextBox.Add(5, textBox_String5Color);
        }

        public Dictionary<Control, Control> notewayButtonToColorTextbox = [];

        private void NotewayColors_FillNotewayButtonToColorDictionary()
        {
            notewayButtonToColorTextbox.Clear();

            notewayButtonToColorTextbox.Add(button_ChangeNumberedFrets, textBox_ShowNumberedFrets);
            notewayButtonToColorTextbox.Add(button_ChangeUnNumberedFrets, textBox_ShowUnNumberedFrets);
            notewayButtonToColorTextbox.Add(button_ChangeNotewayGutter, textBox_ShowNotewayGutter);
            notewayButtonToColorTextbox.Add(button_ChangeFretNumber, textBox_ShowFretNumber);
        }

        private void StringColors_FillNoteNumberToColorDictionary()
        {
            stringNumberToColorTextBox.Clear();

            stringNumberToColorTextBox.Add(0, textBox_Note0Color);
            stringNumberToColorTextBox.Add(1, textBox_Note1Color);
            stringNumberToColorTextBox.Add(2, textBox_Note2Color);
            stringNumberToColorTextBox.Add(3, textBox_Note3Color);
            stringNumberToColorTextBox.Add(4, textBox_Note4Color);
            stringNumberToColorTextBox.Add(5, textBox_Note5Color);
        }
    }
}
