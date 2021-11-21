﻿using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;

namespace RSMods_WPF.Pages.ModPages
{
    public abstract class ModPage : Page
    {
        abstract public void LoadSettings();
        public void GoBack_Click(object sender, RoutedEventArgs e)
        {
            NavigationService.GetNavigationService(this).Navigate(Mods.Instance);
        }
    }
}
