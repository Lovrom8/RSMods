using System.ComponentModel;
using Avalonia.Controls;
using RSMods.ViewModels;

namespace RSMods.Views;

internal sealed partial class ModSettingsView : UserControl
{
    private ModSettingsViewModel? _viewModel;

    public ModSettingsView()
    {
        InitializeComponent();
    }

    protected override void OnDataContextChanged(System.EventArgs e)
    {
        base.OnDataContextChanged(e);

        if (_viewModel is not null)
            _viewModel.PropertyChanged -= OnViewModelPropertyChanged;

        _viewModel = DataContext as ModSettingsViewModel;

        if (_viewModel is not null)
            _viewModel.PropertyChanged += OnViewModelPropertyChanged;
    }

    // A newly picked mod starts at the top of its pane, not wherever the last one was scrolled to.
    private void OnViewModelPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(ModSettingsViewModel.SelectedMod))
            DetailScroller.ScrollToHome();
    }
}
