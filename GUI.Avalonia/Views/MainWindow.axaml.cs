using System;
using Avalonia.Controls;
using RSMods.Services;
using RSMods.ViewModels;

namespace RSMods.Views;

internal sealed partial class MainWindow : Window
{
    private readonly MainWindowViewModel _viewModel;
    private readonly AutoSaveService _autoSave;
    private bool _closingAfterFlush;

    public MainWindow(MainWindowViewModel viewModel, AutoSaveService autoSave)
    {
        InitializeComponent();
        _viewModel = viewModel;
        _autoSave = autoSave;
        DataContext = viewModel;
    }

    /// <summary>Settings save a moment after each change; hold the close until the last one is on disk.</summary>
    protected override async void OnClosing(WindowClosingEventArgs e)
    {
        base.OnClosing(e);
        if (e.Cancel || _closingAfterFlush || !_autoSave.HasUnsavedChanges)
            return;

        e.Cancel = true;
        await _autoSave.FlushAllAsync();
        _closingAfterFlush = true;
        Close();
    }

    protected override async void OnOpened(EventArgs e)
    {
        base.OnOpened(e);
        // Run startup resolution once the window exists so its dialogs have an owner.
        await _viewModel.InitializeAsync();
    }
}
