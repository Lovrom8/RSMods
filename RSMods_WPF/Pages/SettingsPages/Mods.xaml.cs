using System;
using System.Globalization;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Navigation;

namespace RSMods_WPF.Pages.SettingsPages
{
    /// <summary>
    /// Interaction logic for Mods.xaml
    /// </summary>
    public partial class Mods : SettingsPage
    {
        public readonly static new Mods Instance = new();
        public Mods()
        {
            InitializeComponent();
            Startup_FillModsList();
        }

        private void Startup_FillModsList()
        {
            _ = new LoadMods();
            ModsDataGrid.ItemsSource = LoadMods.Mods.Where(mod => mod.ShowInModsTab == true).ToList();
        }

        private void ModEnabledStateChange(object sender, RoutedEventArgs e)
        {
            if (ModsDataGrid.SelectedItem != null && Mod.WhereSettingName(((Mod)ModsDataGrid.SelectedItem).SettingName) != null)
                Mod.WhereSettingName(((Mod)ModsDataGrid.SelectedItem).SettingName).Value = (bool)((CheckBox)sender).IsChecked ? "on" : "off";
        }

        private void ModSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ModsDataGrid.SelectedItem == null || ((Mod)ModsDataGrid.SelectedItem).ModPage == null)
            {
                MoreInfo.Visibility = Visibility.Hidden;
                return;
            }

            MoreInfo.Visibility = Visibility.Visible;
            MoreInfo.DataContext = ((Mod)ModsDataGrid.SelectedItem).ModPage;
        }

        private void MoreInfo_Button(object sender, RoutedEventArgs e)
        {
            ((Mod)ModsDataGrid.SelectedItem).ModPage.LoadSettings(); // Load settings for the Mod Page

            NavigationService.GetNavigationService(this).Navigate(((Mod)ModsDataGrid.SelectedItem).ModPage);
        }
    }

    /// <summary>
    /// Only used for initialization of a Mod's "Enabled" Checkbox.
    /// </summary>
    public class ModEnabledState : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            string modState = (string)value;
            return modState == "on";
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            bool modState = (bool)value;
            return modState ? "on" : "off";
        }
    }
}
