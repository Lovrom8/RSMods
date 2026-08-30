using System;
using Avalonia.Controls;
using RSMods.ViewModels;

namespace RSMods.Views;

internal sealed partial class MainWindow : Window
{
    private readonly MainWindowViewModel _viewModel;

    public MainWindow(MainWindowViewModel viewModel)
    {
        InitializeComponent();
        _viewModel = viewModel;
        DataContext = viewModel;
    }

    protected override async void OnOpened(EventArgs e)
    {
        base.OnOpened(e);
        // Run startup resolution once the window exists so its dialogs have an owner.
        await _viewModel.InitializeAsync();
    }
}
