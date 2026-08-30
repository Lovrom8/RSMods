using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace RSMods.Util
{
    public static class SettingsSanitizer
    {
        private static readonly List<string> Warnings = new List<string>();

        public static void Clear() => Warnings.Clear();

        public static bool HasWarnings => Warnings.Count > 0;

        public static decimal SanitizeDecimal(
            string rawValue,
            decimal defaultValue,
            decimal min,
            decimal max,
            string sourceFile,
            string settingName)
        {
            if (string.IsNullOrWhiteSpace(rawValue))
                return Clamp(defaultValue, min, max);

            if (!decimal.TryParse(rawValue.Trim(), out decimal parsed))
            {
                AddWarning(sourceFile, settingName, rawValue, defaultValue.ToString(),
                    "not a valid number");
                return Clamp(defaultValue, min, max);
            }

            if (parsed < min || parsed > max)
            {
                AddWarning(sourceFile, settingName, rawValue, defaultValue.ToString(),
                    $"outside allowed range [{min} .. {max}]");
                return Clamp(defaultValue, min, max);
            }

            return parsed;
        }

        public static int SanitizeInt(
            string rawValue,
            int defaultValue,
            int min,
            int max,
            string sourceFile,
            string settingName)
        {
            return (int)SanitizeDecimal(rawValue, defaultValue, min, max, sourceFile, settingName);
        }

        public static decimal SafeSetNumericUpDown(
            NumericUpDown control,
            string rawValue,
            decimal defaultValue,
            string sourceFile,
            string settingName)
        {
            return SafeSetNumericUpDown(control, rawValue, defaultValue,
                control.Minimum, control.Maximum, sourceFile, settingName);
        }

        public static decimal SafeSetNumericUpDown(
            NumericUpDown control,
            string rawValue,
            decimal defaultValue,
            decimal min,
            decimal max,
            string sourceFile,
            string settingName)
        {
            if (control.Maximum < max)
                control.Maximum = max;
            if (control.Minimum > min)
                control.Minimum = min;

            decimal value = SanitizeDecimal(
                rawValue,
                defaultValue,
                min,
                max,
                sourceFile,
                settingName);

            control.Value = value;
            return value;
        }

        public static int SafeSetTrackBar(
            TrackBar control,
            string rawValue,
            int defaultValue,
            string sourceFile,
            string settingName)
        {
            int value = SanitizeInt(
                rawValue,
                defaultValue,
                control.Minimum,
                control.Maximum,
                sourceFile,
                settingName);

            control.Value = value;
            return value;
        }

        public static int SafeSetSelectedIndex(
            ListBox listBox,
            int desiredIndex,
            int defaultIndex,
            string sourceFile,
            string settingName,
            string rawValueForMessage)
        {
            if (desiredIndex >= 0 && desiredIndex < listBox.Items.Count)
            {
                listBox.SelectedIndex = desiredIndex;
                return desiredIndex;
            }

            int fallback = (defaultIndex >= 0 && defaultIndex < listBox.Items.Count) ? defaultIndex : -1;

            string shownRaw = string.IsNullOrWhiteSpace(rawValueForMessage) ? "(empty / missing)" : rawValueForMessage;
            string used = fallback >= 0 ? fallback.ToString() : "(none)";
            AddWarning(sourceFile, settingName, shownRaw, used,
                $"maps to list index {desiredIndex}, valid range is [0 .. {Math.Max(0, listBox.Items.Count - 1)}]");

            if (fallback >= 0)
                listBox.SelectedIndex = fallback;
            else
                listBox.SelectedIndex = -1;

            return fallback;
        }

        public static bool SanitizeBool01(
            string rawValue,
            bool defaultValue,
            string sourceFile,
            string settingName)
        {
            if (string.IsNullOrWhiteSpace(rawValue))
                return defaultValue;

            string trimmed = rawValue.Trim();

            if (trimmed == "1" || trimmed.Equals("true", StringComparison.OrdinalIgnoreCase)
                || trimmed.Equals("on", StringComparison.OrdinalIgnoreCase)
                || trimmed.Equals("yes", StringComparison.OrdinalIgnoreCase))
                return true;

            if (trimmed == "0" || trimmed.Equals("false", StringComparison.OrdinalIgnoreCase)
                || trimmed.Equals("off", StringComparison.OrdinalIgnoreCase)
                || trimmed.Equals("no", StringComparison.OrdinalIgnoreCase))
                return false;

            AddWarning(sourceFile, settingName, rawValue, defaultValue ? "1" : "0",
                "not a valid boolean (expected 0/1/true/false/yes/no)");
            return defaultValue;
        }

        public static void ReportInvalid(
            string sourceFile,
            string settingName,
            string badValue,
            string usedDefault,
            string reason)
        {
            AddWarning(sourceFile, settingName, badValue ?? "(empty / missing)", usedDefault, reason);
        }

        public static CheckState SanitizeWasapiOutputsTriState(
            string rawValue,
            string sourceFile,
            string settingName)
        {
            if (string.IsNullOrWhiteSpace(rawValue))
                return CheckState.Unchecked;

            string trimmed = rawValue.Trim();

            if (int.TryParse(trimmed, out int asInt) && asInt < 0)
                return CheckState.Indeterminate;

            if (trimmed == "1" || trimmed.Equals("true", StringComparison.OrdinalIgnoreCase)
                || trimmed.Equals("on", StringComparison.OrdinalIgnoreCase)
                || trimmed.Equals("yes", StringComparison.OrdinalIgnoreCase))
                return CheckState.Checked;

            if (trimmed == "0" || trimmed.Equals("false", StringComparison.OrdinalIgnoreCase)
                || trimmed.Equals("off", StringComparison.OrdinalIgnoreCase)
                || trimmed.Equals("no", StringComparison.OrdinalIgnoreCase))
                return CheckState.Unchecked;

            AddWarning(sourceFile, settingName, rawValue, "0",
                "not a valid EnableWasapiOutputs value (expected 0, 1, or -1 for prompt)");
            return CheckState.Unchecked;
        }

        public static void ShowWarningsIfAny()
        {
            if (Warnings.Count == 0)
                return;

            var sb = new StringBuilder();
            sb.AppendLine("One or more settings files contain invalid values.");
            sb.AppendLine("The GUI would previously crash on these. They have been replaced with safe defaults:");
            sb.AppendLine();

            foreach (string warning in Warnings)
                sb.AppendLine("• " + warning);

            sb.AppendLine();
            sb.AppendLine("If you edited Rocksmith.ini, RS_ASIO.ini, or RSMods.ini by hand, fix the values above (or re-save from this GUI) so the game and mods behave as expected.");

            MessageBox.Show(
                sb.ToString(),
                "Invalid Settings Detected",
                MessageBoxButtons.OK,
                MessageBoxIcon.Warning);

            Warnings.Clear();
        }

        private static void AddWarning(
            string sourceFile,
            string settingName,
            string badValue,
            string usedDefault,
            string reason)
        {
            string cleanName = settingName.TrimEnd(' ', '=');
            Warnings.Add(
                $"{sourceFile} → {cleanName}: '{badValue}' ({reason}). Defaulted to: {usedDefault}.");
        }

        private static decimal Clamp(decimal value, decimal min, decimal max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }
    }
}
