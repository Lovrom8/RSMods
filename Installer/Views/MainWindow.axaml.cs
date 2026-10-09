using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;
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
        _viewModel.ConfirmAsync = ConfirmAsync;
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

    // Closing the window answers no, so nothing is replaced unless the player says so.
    private async Task<bool> ConfirmAsync(string question)
    {
        var dialog = new Window
        {
            Title = "RSMods",
            Width = 460,
            SizeToContent = SizeToContent.Height,
            CanResize = false,
            WindowStartupLocation = WindowStartupLocation.CenterOwner
        };

        var keep = new Button { Content = "Keep it", IsCancel = true };
        var replace = new Button { Content = "Replace" };
        keep.Click += (_, _) => dialog.Close(false);
        replace.Click += (_, _) => dialog.Close(true);

        dialog.Content = new StackPanel
        {
            Margin = new Thickness(20),
            Spacing = 16,
            Children =
            {
                new TextBlock { Text = question, TextWrapping = TextWrapping.Wrap },
                new StackPanel
                {
                    Orientation = Orientation.Horizontal,
                    HorizontalAlignment = HorizontalAlignment.Right,
                    Spacing = 8,
                    Children = { keep, replace }
                }
            }
        };

        return await dialog.ShowDialog<bool>(this);
    }
}
