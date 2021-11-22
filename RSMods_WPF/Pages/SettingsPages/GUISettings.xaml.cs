using MaterialDesignThemes.Wpf;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using Forms = System.Windows.Forms;
using Drawing = System.Drawing;

namespace RSMods_WPF.Pages.SettingsPages
{
    /// <summary>
    /// Interaction logic for GUISettings.xaml
    /// </summary>
    public partial class GUISettings : SettingsPage
    {
        public static new GUISettings Instance = new();
        private readonly PaletteHelper _paletteHelper = new();


        public GUISettings()
        {
            InitializeComponent();
            Startup_LoadInitialValues();
        }

        private void Startup_LoadInitialValues()
        {
            DarkMode.IsChecked = Mod.WhereSettingName("DarkMode").Value.ToString() == "on";
        }

        public void ChangeDarkMode(object sender, RoutedEventArgs e)
        {
            bool darkMode = (bool)((CheckBox)sender).IsChecked;
            ITheme theme = _paletteHelper.GetTheme();
            IBaseTheme baseTheme = darkMode ? new MaterialDesignDarkTheme() : new MaterialDesignLightTheme();
            theme.SetBaseTheme(baseTheme);
            _paletteHelper.SetTheme(theme);

            Mod.WhereSettingName("DarkMode").Value = darkMode ? "on" : "off";
        }

        private void ChangePrimaryColor_Click(object sender, RoutedEventArgs e)
        {
            Forms.ColorDialog colorDialog = new Forms.ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false
            };

            if (colorDialog.ShowDialog() == Forms.DialogResult.OK)
            {
                ITheme theme = _paletteHelper.GetTheme();
                theme.SetPrimaryColor(DrawingColorToMediaColor(colorDialog.Color));
                _paletteHelper.SetTheme(theme);

                Mod.WhereSettingName("PrimaryColor").Value = colorDialog.Color.R.ToString("X2") + colorDialog.Color.G.ToString("X2") + colorDialog.Color.B.ToString("X2");

                (Window.GetWindow(Application.Current.MainWindow) as MainWindow).ResetIconColors();
            }
        }

        private static Color DrawingColorToMediaColor(Drawing.Color color) => Color.FromRgb(color.R, color.G, color.B);
    }
}
