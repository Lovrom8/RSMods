using System;
using Avalonia.Controls;
using RSMods.Services;
using RSMods.ViewModels;

namespace RSMods.Views;

internal sealed partial class MainWindow : Window
{
    private readonly MainWindowViewModel _viewModel;
    private bool _closeConfirmed;
    private bool _confirmingClose;

    public MainWindow(MainWindowViewModel viewModel)
    {
        InitializeComponent();
        _viewModel = viewModel;
        DataContext = viewModel;
    }

    /// <summary>
    /// Settings save a moment after each change, and some jobs write game files for minutes; hold the close until
    /// the last change is on disk (or the user chooses to lose it) and nothing is left half-written.
    /// </summary>
    protected override async void OnClosing(WindowClosingEventArgs e)
    {
        base.OnClosing(e);
        if (e.Cancel || _closeConfirmed || !_viewModel.HasWorkBeforeClose)
            return;

        e.Cancel = true;
        if (_confirmingClose)
            return; // Already asking; a second close request waits for that answer

        _confirmingClose = true;
        try
        {
            if (!await _viewModel.ConfirmCloseAsync())
                return;
        }
        finally
        {
            _confirmingClose = false;
        }

        _closeConfirmed = true;
        Close();
    }

    protected override async void OnOpened(EventArgs e)
    {
        base.OnOpened(e);
        // Run startup resolution once the window exists so its dialogs have an owner.
        await _viewModel.InitializeAsync();
    }
}
