using System.Windows;
using System.Reflection;
using RSMods_WPF.Pages.SettingsPages;

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

        private void Mods_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = Mods.Instance;
        private void SetForget_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = SetForget.Instance;
        private void Rocksmith_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = Rocksmith.Instance;
        private void Soundpacks_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = Soundpacks.Instance;
        private void Twitch_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = Twitch.Instance;
        private void ASIO_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = ASIO.Instance;
    }
}
