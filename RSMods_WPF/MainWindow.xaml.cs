using System.Windows;
using System.Reflection;
using RSMods_WPF.Pages;

namespace RSMods_WPF
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            Startup_UpdateVersionText();
        }

        private void Startup_UpdateVersionText() => Version.Content = "Version: " + Assembly.GetExecutingAssembly().GetName().Version.ToString();

        private void Mods_Tab_Click(object sender, RoutedEventArgs e)
        {
            MainFrame.Content = Mods.Instance;
        }

        private void OtherApps_Tab_Click(object sender, RoutedEventArgs e)
        {
            MainFrame.Content = OtherApps.Instance;
        }
    }
}
