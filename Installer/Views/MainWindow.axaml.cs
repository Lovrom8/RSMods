using Avalonia.Controls;
using Avalonia.Platform.Storage;
using RS2014_Mod_Installer.ViewModels;

namespace RS2014_Mod_Installer.Views;

internal sealed partial class MainWindow : Window
{
    private readonly InstallerViewModel _viewModel;

    public MainWindow(InstallerViewModel viewModel)
    {
        InitializeComponent();
        _viewModel = viewModel;
        _viewModel.BrowseForRocksmithFolderAsync = BrowseForRocksmithFolderAsync;
        DataContext = viewModel;
    }

    protected override void OnOpened(EventArgs e)
    {
        base.OnOpened(e);
        _viewModel.Initialize();
    }

    private async Task<string?> BrowseForRocksmithFolderAsync()
    {
        IReadOnlyList<IStorageFolder> folders = await StorageProvider.OpenFolderPickerAsync(
            new FolderPickerOpenOptions
            {
                Title = "Select the Rocksmith 2014 folder",
                AllowMultiple = false
            });

        return folders.Count == 0 ? null : folders[0].TryGetLocalPath();
    }
}
