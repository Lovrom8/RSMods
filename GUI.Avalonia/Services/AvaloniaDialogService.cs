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
