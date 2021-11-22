using RSMods_WPF.Pages.SettingsPages;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;

namespace RSMods_WPF.Pages.ModPages
{
    public abstract class ModPage : Page
    {
        public abstract void LoadSettings();
        public void GoBack_Click(object sender, RoutedEventArgs e)
        {
            NavigationService.GetNavigationService(this).Navigate(Mods.Instance);
        }
    }
}
