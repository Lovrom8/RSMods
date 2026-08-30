#nullable enable
using System.Collections.Generic;
using System.Threading.Tasks;

namespace RSMods.Core
{
    /// <summary>
    /// UI-agnostic dialog surface. Implemented by a WinForms adapter today and an Avalonia
    /// adapter after the migration, so logic never depends on a specific UI framework.
    /// Async throughout: Avalonia dialogs are inherently awaitable, and several callers use the
    /// result to drive control flow (so a sync interface would force the Avalonia adapter to
    /// block the UI thread).
    /// </summary>
    public interface IDialogService
    {
        /// <summary>Informational message with an OK button.</summary>
        Task ShowInfoAsync(string message, string title = "");

        /// <summary>Error message with an OK button.</summary>
        Task ShowErrorAsync(string message, string title = "Error");

        /// <summary>Yes/No (or OK/Cancel) confirmation. Returns true for the affirmative choice.</summary>
        Task<bool> ShowConfirmAsync(string message, string title = "");

        /// <summary>
        /// Two-choice prompt with custom button labels (e.g. "Guitar"/"Bass"). Returns true for the
        /// positive (first) choice, false for the negative one.
        /// </summary>
        Task<bool> ShowChoiceAsync(string message, string title, string positiveText, string negativeText);

        /// <summary>Folder picker. Returns the chosen path, or null if the user cancelled.</summary>
        Task<string?> PickFolderAsync(string title, string? startPath = null);

        /// <summary>
        /// Multi-file open picker. <paramref name="patterns"/> are glob patterns such as "*.json" or
        /// "*.tone2014.xml". Returns the chosen file paths, or an empty list if the user cancelled.
        /// </summary>
        Task<IReadOnlyList<string>> PickFilesAsync(
            string title, string typeName, IReadOnlyList<string> patterns, bool allowMultiple);

        /// <summary>
        /// Save-file picker. <paramref name="pattern"/> is a glob such as "*.rs_soundpack". Returns the chosen
        /// path (with the extension applied), or null if the user cancelled.
        /// </summary>
        Task<string?> PickSaveFileAsync(
            string title, string suggestedFileName, string typeName, string pattern, string? startPath = null);
    }
}
