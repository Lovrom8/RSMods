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
    public partial class ToggleLoft : Page
    {
        public static ToggleLoft Instance = new();
        public ToggleLoft()
        {
            InitializeComponent();
        }

        private void GoBack_Click(object sender, RoutedEventArgs e)
        {
            NavigationService.GetNavigationService(this).Navigate(Mods.Instance);
        }
    }
}
