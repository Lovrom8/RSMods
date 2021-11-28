using System.Windows;
using System.Reflection;
using RSMods_WPF.Pages.SettingsPages;
using MaterialDesignThemes.Wpf;
using System.Windows.Media;
using RSMods_WPF.Asio;

namespace RSMods_WPF
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private readonly PaletteHelper _paletteHelper = new();
        public MainWindow()
        {
            InitializeComponent();
            Startup_UpdateVersionText();
            Startup_LoadMods();
            Startup_LoadAsioSettings();
            Startup_LoadCustomColors();
        }

        private void Startup_UpdateVersionText() => Version.Content = "Version: " + Assembly.GetExecutingAssembly().GetName().Version.ToString();
        private static void Startup_LoadMods() => _ = new LoadMods();

        private void Startup_LoadAsioSettings()
        {
            if (!Settings.HasValidAsioSettingsFile())
            {
                ASIO_Tab.Visibility = Visibility.Hidden;
                return;
            }
            _ = new LoadAsio();
        }

        private void Startup_LoadCustomColors()
        {
            ITheme theme = _paletteHelper.GetTheme();
            IBaseTheme baseTheme = Mod.WhereSettingName("DarkMode").Value.ToString() == "on" ? new MaterialDesignDarkTheme() : new MaterialDesignLightTheme();
            theme.SetBaseTheme(baseTheme);

            string hex_primaryColor = Mod.WhereSettingName("PrimaryColor").Value.ToString();

            Color primaryColor = Color.FromRgb(
                byte.Parse(hex_primaryColor[..2], System.Globalization.NumberStyles.HexNumber),
                byte.Parse(hex_primaryColor.Substring(2, 2), System.Globalization.NumberStyles.HexNumber),
                byte.Parse(hex_primaryColor.Substring(4, 2), System.Globalization.NumberStyles.HexNumber)
            );
            theme.SetPrimaryColor(primaryColor);

            _paletteHelper.SetTheme(theme);

            ResetIconColors();
        }

        public void ResetIconColors()
        {
            SoundPack_Icon.Fill = Rocksmith_Tab.Foreground;
            Twitch_Icon.Fill = Rocksmith_Tab.Foreground;
        }

        private void Mods_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = Mods.Instance;
        private void SetForget_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = SetForget.Instance;
        private void Rocksmith_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = Rocksmith.Instance;
        private void Soundpacks_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = Soundpacks.Instance;
        private void Twitch_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = Twitch.Instance;
        private void ASIO_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = ASIO.Instance;
        private void GUISettings_Tab_Click(object sender, RoutedEventArgs e) => MainFrame.Content = GUISettings.Instance;
    }
}
