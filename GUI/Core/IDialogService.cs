#nullable enable
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

        /// <summary>Folder picker. Returns the chosen path, or null if the user cancelled.</summary>
        Task<string?> PickFolderAsync(string title, string? startPath = null);
    }
}
