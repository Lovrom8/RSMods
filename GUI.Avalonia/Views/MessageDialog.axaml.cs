using Avalonia.Controls;
using Avalonia.Interactivity;

namespace RSMods.Views;

internal sealed partial class MessageDialog : Window
{
    public MessageDialog()
    {
        InitializeComponent();
    }

    private MessageDialog(string message, string title, bool confirmation) : this()
    {
        Title = string.IsNullOrWhiteSpace(title) ? "RSMods" : title;
        MessageText.Text = message;
        NegativeButton.IsVisible = confirmation;
    }

    public static Task<bool> ShowAsync(Window owner, string message, string title, bool confirmation) =>
        new MessageDialog(message, title, confirmation).ShowDialog<bool>(owner);

    private void PositiveButton_OnClick(object? sender, RoutedEventArgs e) => Close(true);

    private void NegativeButton_OnClick(object? sender, RoutedEventArgs e) => Close(false);
}
