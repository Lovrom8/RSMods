using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace RSMods_WPF.Pages.ModPages
{
    /// <summary>
    /// Interaction logic for ToggleLoft.xaml
    /// </summary>
    public partial class ToggleLoft : ModPage
    {
        public ToggleLoft()
        {
            InitializeComponent();
        }

        public override void LoadSettings()
        {
            object when = Mod.WhereSettingName("ToggleLoftWhen").Value;

            switch (when)
            {
                case "manual":
                    When_Manual.IsChecked = true;
                    break;
                case "song":
                    When_InSong.IsChecked = true;
                    break;
                case "startup":
                    When_Always.IsChecked = true;
                    break;
                default: // We don't know what the user has set, so don't initialize anything.
                    break;
            }
        }
        private void GoBack_Click(object sender, RoutedEventArgs e)
        {
            NavigationService.GetNavigationService(this).Navigate(Mods.Instance);
        }

        private void When_Manual_Checked(object sender, RoutedEventArgs e)
        {
            Mod.WhereSettingName("ToggleLoftWhen").Value = "manual";
        }

        private void When_InSong_Checked(object sender, RoutedEventArgs e)
        {
            Mod.WhereSettingName("ToggleLoftWhen").Value = "song";
        }

        private void When_Always_Checked(object sender, RoutedEventArgs e)
        {
            Mod.WhereSettingName("ToggleLoftWhen").Value = "startup";
        }
    }
}
