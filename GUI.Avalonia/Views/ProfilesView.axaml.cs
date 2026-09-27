using System;
using Avalonia.Controls;
using Avalonia.Data;
using RSMods.ViewModels;

namespace RSMods.Views;

internal sealed partial class ProfilesView : UserControl
{
    private ProfilesViewModel? _viewModel;

    public ProfilesView()
    {
        InitializeComponent();
        DataContextChanged += OnDataContextChanged;
    }

    private void OnDataContextChanged(object? sender, EventArgs e)
    {
        if (_viewModel is not null)
            _viewModel.SongColumnsChanged -= OnSongColumnsChanged;

        _viewModel = DataContext as ProfilesViewModel;

        if (_viewModel is not null)
            _viewModel.SongColumnsChanged += OnSongColumnsChanged;

        RebuildSongColumns();
    }

    private void OnSongColumnsChanged(object? sender, EventArgs e) => RebuildSongColumns();

    /// <summary>
    /// Avalonia's DataGrid columns are created in code, so the song grid's columns are (re)built here from
    /// the view model's descriptors: the fixed Artist/Title columns plus one checkbox column per song list.
    /// Reflection bindings are used deliberately (indexer paths like <c>Cells[2].IsChecked</c>).
    /// No column is star-sized: with one, the grid squeezes every other column to fit the viewport. Fixed and
    /// header-sized columns make it scroll sideways instead (a profile has 6 to 20 lists), with Artist and Title
    /// frozen in the view so they stay put while it does.
    /// </summary>
    private void RebuildSongColumns()
    {
        SongsGrid.Columns.Clear();

        SongsGrid.Columns.Add(new DataGridTextColumn
        {
            Header = "Artist",
            Binding = new Binding(nameof(SongRowViewModel.Artist)),
            IsReadOnly = true,
            Width = new DataGridLength(190),
            MinWidth = 100,
        });
        SongsGrid.Columns.Add(new DataGridTextColumn
        {
            Header = "Title",
            Binding = new Binding(nameof(SongRowViewModel.Title)),
            IsReadOnly = true,
            Width = new DataGridLength(250),
            MinWidth = 100,
        });

        if (_viewModel is null)
            return;

        foreach (SongListColumn column in _viewModel.SongListColumns)
        {
            SongsGrid.Columns.Add(new DataGridCheckBoxColumn
            {
                Header = column.Header,
                Binding = new Binding($"Cells[{column.CellIndex}].IsChecked") { Mode = BindingMode.TwoWay },
                Width = new DataGridLength(1, DataGridLengthUnitType.SizeToHeader),
                MinWidth = 90,
            });
        }
    }
}
