#nullable enable
using System;
using System.Threading.Tasks;
using System.Windows.Forms;
using RSMods.Core;
using RSMods.Util;

namespace RSMods.WinForms
{
    /// <summary>
    /// WinForms implementation of <see cref="IDialogService"/>. The sync WinForms dialogs are
    /// wrapped in already-completed tasks; the async signatures exist so the Avalonia adapter can
    /// implement them without changing any caller. This is the one place the shell-picker owner
    /// handle lookup lives.
    /// </summary>
    public class WinFormsDialogService : IDialogService
    {
        public Task ShowInfoAsync(string message, string title = "")
        {
            MessageBox.Show(message, title, MessageBoxButtons.OK, MessageBoxIcon.Information);
            return Task.CompletedTask;
        }

        public Task ShowErrorAsync(string message, string title = "Error")
        {
            MessageBox.Show(message, title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            return Task.CompletedTask;
        }

        public Task<bool> ShowConfirmAsync(string message, string title = "")
        {
            var result = MessageBox.Show(message, title, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            return Task.FromResult(result == DialogResult.Yes);
        }

        public Task<string?> PickFolderAsync(string title, string? startPath = null)
        {
            var picker = new FolderPicker { Title = title };
            if (!string.IsNullOrEmpty(startPath))
                picker.InputPath = startPath;

            IntPtr owner = Application.OpenForms.Count > 0 ? Application.OpenForms[0].Handle : IntPtr.Zero;
            bool? picked = picker.ShowDialog(owner);
            return Task.FromResult(picked == true ? picker.ResultPath : null);
        }
    }
}
