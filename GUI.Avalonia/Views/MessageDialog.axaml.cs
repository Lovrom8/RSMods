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

    private MessageDialog(string message, string title, string positiveText, string negativeText) : this()
    {
        Title = string.IsNullOrWhiteSpace(title) ? "RSMods" : title;
        MessageText.Text = message;
        PositiveButton.Content = positiveText;
        NegativeButton.Content = negativeText;
        NegativeButton.IsVisible = true;
    }

    public static Task<bool> ShowAsync(Window owner, string message, string title, bool confirmation) =>
        new MessageDialog(message, title, confirmation).ShowDialog<bool>(owner);

    public static Task<bool> ShowChoiceAsync(
        Window owner, string message, string title, string positiveText, string negativeText) =>
        new MessageDialog(message, title, positiveText, negativeText).ShowDialog<bool>(owner);

    private void PositiveButton_OnClick(object? sender, RoutedEventArgs e) => Close(true);

    private void NegativeButton_OnClick(object? sender, RoutedEventArgs e) => Close(false);
}
