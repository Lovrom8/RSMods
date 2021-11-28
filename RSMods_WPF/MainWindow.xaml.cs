using System.Windows;
using System.Reflection;
using RSMods_WPF.Pages.SettingsPages;
using MaterialDesignThemes.Wpf;
using System.Windows.Media;
using RSMods_WPF.Asio;
using System.Windows.Controls;

namespace RSMods_WPF
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private readonly PaletteHelper _paletteHelper = new();

        public Brush HighlightedTabBrush = (Brush)Application.Current.Resources["PrimaryHueLightBrush"];
        public SettingsPages CurrentPage = SettingsPages.None;
        public Color PrimaryColor = Color.FromRgb(255,255,255);

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

            PrimaryColor = primaryColor;

            Application.Current.Resources["InvertedPrimaryColor"] = (SolidColorBrush)new BrushConverter().ConvertFromString(Color.FromRgb((byte)(255 - PrimaryColor.R), (byte)(255 - PrimaryColor.G), (byte)(255 - PrimaryColor.B)).ToString());

            _paletteHelper.SetTheme(theme);

            HighlightedTabBrush = (Brush)Application.Current.Resources["PrimaryHueLightBrush"];

            ResetIconColors();
        }

        public void ResetIconColors()
        {
            SoundPack_Icon.Fill = (Brush)Application.Current.Resources["InvertedPrimaryColor"];
            Twitch_Icon.Fill = (Brush)Application.Current.Resources["InvertedPrimaryColor"];

            ASIO.Instance.Startup_UpdateResetIcons();
        }

        private void Mods_Tab_Click(object sender, RoutedEventArgs e) => GoToNewTab(SettingsPages.Mods, (Button)sender);
        private void SetForget_Tab_Click(object sender, RoutedEventArgs e) => GoToNewTab(SettingsPages.SetForget, (Button)sender);
        private void Rocksmith_Tab_Click(object sender, RoutedEventArgs e) => GoToNewTab(SettingsPages.Rocksmith, (Button)sender);
        private void Soundpacks_Tab_Click(object sender, RoutedEventArgs e) => GoToNewTab(SettingsPages.Soundpacks, (Button)sender);
        private void Twitch_Tab_Click(object sender, RoutedEventArgs e) => GoToNewTab(SettingsPages.Twitch, (Button)sender);
        private void ASIO_Tab_Click(object sender, RoutedEventArgs e) => GoToNewTab(SettingsPages.Asio, (Button)sender);
        private void GUISettings_Tab_Click(object sender, RoutedEventArgs e) => GoToNewTab(SettingsPages.GuiSettings, (Button)sender);

        public void ResetSelectedTab()
        {
            Brush backgroundColor = (Brush)Application.Current.Resources["PrimaryHueMidBrush"];

            Mods_Tab.Background = backgroundColor;
            SetForget_Tab.Background = backgroundColor;
            Rocksmith_Tab.Background = backgroundColor;
            Soundpacks_Tab.Background = backgroundColor;
            Twitch_Tab.Background = backgroundColor;
            ASIO_Tab.Background = backgroundColor;
            GUISettings_Tab.Background = backgroundColor;
        }

        private void GoToNewTab(SettingsPages newPage, Button tabButton)
        {
            ResetSelectedTab();
            tabButton.Background = HighlightedTabBrush;

            switch (newPage)
            {
                case SettingsPages.Mods:
                    MainFrame.Content = Mods.Instance;
                    break;
                case SettingsPages.SetForget:
                    MainFrame.Content = SetForget.Instance;
                    break;
                case SettingsPages.Rocksmith:
                    MainFrame.Content = Rocksmith.Instance;
                    break;
                case SettingsPages.Soundpacks:
                    MainFrame.Content = Soundpacks.Instance;
                    break;
                case SettingsPages.Twitch:
                    MainFrame.Content = Twitch.Instance;
                    break;
                case SettingsPages.Asio:
                    MainFrame.Content = ASIO.Instance;
                    break;
                case SettingsPages.GuiSettings:
                    MainFrame.Content = GUISettings.Instance;
                    break;
                default: // Unknown tab!
                    break;
            }
        }

        public enum SettingsPages
        {
            None,
            Mods,
            SetForget,
            Rocksmith,
            Soundpacks,
            Twitch,
            Asio,
            GuiSettings
        }
    }
}
