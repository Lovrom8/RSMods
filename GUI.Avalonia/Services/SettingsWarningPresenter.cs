using System.Collections.Generic;
using System.IO;
using System.Text;
using RSMods.Core;

namespace RSMods.Services;

/// <summary>
/// Avalonia-owned presentation of <see cref="IniValidationWarning"/>s: the frontend decides how the
/// domain's "this value was invalid" reports are shown to the user, whether the value was reset or kept. This is
/// the Avalonia counterpart to the WinForms settings sanitizer's warning dialog.
/// </summary>
internal sealed class SettingsWarningPresenter(IDialogService dialogs)
{
    private const int MaxShown = 10;

    public Task PresentAsync(IReadOnlyList<IniValidationWarning> warnings)
    {
        if (warnings.Count == 0)
            return Task.CompletedTask;

        string fileName = GetFileName(warnings[0].FilePath);
        bool kept = warnings[0].ValueKept;
        int shown = System.Math.Min(warnings.Count, MaxShown);
        var text = new StringBuilder();
        text.AppendLine(kept
            ? $"{warnings.Count} setting(s) in {fileName} aren't values RSMods understands. They're shown as their defaults, and the file keeps them unless you change them:"
            : $"{warnings.Count} invalid setting(s) in {fileName} were reset to their defaults:");
        text.AppendLine();

        for (int i = 0; i < shown; i++)
        {
            var warning = warnings[i];
            text.AppendLine($"• {warning.Section} {warning.Key}: \"{warning.RawValue}\" → {warning.DefaultValue}");
        }

        if (warnings.Count > shown)
            text.AppendLine($"… and {warnings.Count - shown} more.");

        return dialogs.ShowInfoAsync(text.ToString().TrimEnd(), kept ? "Some settings weren't understood" : "Some settings were reset");
    }

    private static string GetFileName(string filePath)
    {
        if (string.IsNullOrEmpty(filePath))
            return "the settings file";

        string name = Path.GetFileName(filePath);
        return string.IsNullOrEmpty(name) ? "the settings file" : name;
    }
}
