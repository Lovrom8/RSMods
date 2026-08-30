#nullable enable
using System;
using System.Collections.Generic;
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

        public Task<bool> ShowChoiceAsync(string message, string title, string positiveText, string negativeText)
        {
            // The WinForms Profiles tab does its own MessageBoxManager relabelling in-place; this adapter
            // method exists for interface parity and maps the positive choice to Yes.
            var result = MessageBox.Show(
                $"{message}\n\nYes = {positiveText}\nNo = {negativeText}",
                title, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
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

        public Task<IReadOnlyList<string>> PickFilesAsync(
            string title, string typeName, IReadOnlyList<string> patterns, bool allowMultiple)
        {
            using var dialog = new OpenFileDialog
            {
                Title = title,
                Multiselect = allowMultiple,
                Filter = $"{typeName}|{string.Join(";", patterns)}",
            };

            IReadOnlyList<string> files = dialog.ShowDialog() == DialogResult.OK
                ? dialog.FileNames
                : Array.Empty<string>();
            return Task.FromResult(files);
        }

        public Task<string?> PickSaveFileAsync(
            string title, string suggestedFileName, string typeName, string pattern, string? startPath = null)
        {
            using var dialog = new SaveFileDialog
            {
                Title = title,
                FileName = suggestedFileName,
                Filter = $"{typeName}|{pattern}",
            };

            if (!string.IsNullOrEmpty(startPath))
                dialog.InitialDirectory = startPath;

            string? path = dialog.ShowDialog() == DialogResult.OK ? dialog.FileName : null;
            return Task.FromResult(path);
        }
    }
}
