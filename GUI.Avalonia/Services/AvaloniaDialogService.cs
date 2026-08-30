using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Platform.Storage;
using RSMods.Core;
using RSMods.Views;

namespace RSMods.Services;

internal sealed class AvaloniaDialogService : IDialogService
{
    public Task ShowInfoAsync(string message, string title = "") =>
        MessageDialog.ShowAsync(GetOwner(), message, title, confirmation: false);

    public Task ShowErrorAsync(string message, string title = "Error") =>
        MessageDialog.ShowAsync(GetOwner(), message, title, confirmation: false);

    public Task<bool> ShowConfirmAsync(string message, string title = "") =>
        MessageDialog.ShowAsync(GetOwner(), message, title, confirmation: true);

    public Task<bool> ShowChoiceAsync(string message, string title, string positiveText, string negativeText) =>
        MessageDialog.ShowChoiceAsync(GetOwner(), message, title, positiveText, negativeText);

    public async Task<string?> PickFolderAsync(string title, string? startPath = null)
    {
        var owner = GetOwner();
        var options = new FolderPickerOpenOptions
        {
            Title = title,
            AllowMultiple = false
        };

        if (!string.IsNullOrWhiteSpace(startPath) && Directory.Exists(startPath))
            options.SuggestedStartLocation = await owner.StorageProvider.TryGetFolderFromPathAsync(startPath);

        var folders = await owner.StorageProvider.OpenFolderPickerAsync(options);
        if (folders.Count == 0)
            return null;

        var path = folders[0].Path;
        return path.IsFile ? path.LocalPath : path.ToString();
    }

    public async Task<IReadOnlyList<string>> PickFilesAsync(
        string title, string typeName, IReadOnlyList<string> patterns, bool allowMultiple)
    {
        var owner = GetOwner();
        var options = new FilePickerOpenOptions
        {
            Title = title,
            AllowMultiple = allowMultiple,
            FileTypeFilter = [new FilePickerFileType(typeName) { Patterns = [.. patterns] }],
        };

        var files = await owner.StorageProvider.OpenFilePickerAsync(options);
        return files
            .Select(file => file.Path.IsFile ? file.Path.LocalPath : file.Path.ToString())
            .ToList();
    }

    public async Task<string?> PickSaveFileAsync(
        string title, string suggestedFileName, string typeName, string pattern, string? startPath = null)
    {
        var owner = GetOwner();
        var options = new FilePickerSaveOptions
        {
            Title = title,
            SuggestedFileName = suggestedFileName,
            DefaultExtension = pattern.TrimStart('*', '.'),
            ShowOverwritePrompt = true,
            FileTypeChoices = [new FilePickerFileType(typeName) { Patterns = [pattern] }],
        };

        if (!string.IsNullOrWhiteSpace(startPath) && Directory.Exists(startPath))
            options.SuggestedStartLocation = await owner.StorageProvider.TryGetFolderFromPathAsync(startPath);

        var file = await owner.StorageProvider.SaveFilePickerAsync(options);
        if (file is null)
            return null;

        return file.Path.IsFile ? file.Path.LocalPath : file.Path.ToString();
    }

    private static Window GetOwner()
    {
        if (Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime
            {
                MainWindow: { } mainWindow
            })
        {
            return mainWindow;
        }

        throw new InvalidOperationException("The main window is not available for dialogs.");
    }
}
