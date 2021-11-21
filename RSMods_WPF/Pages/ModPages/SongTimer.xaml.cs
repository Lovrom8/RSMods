using System.Windows;

namespace RSMods_WPF.Pages.ModPages
{
    /// <summary>
    /// Interaction logic for SongTimer.xaml
    /// </summary>
    public partial class SongTimer : ModPage
    {
        public SongTimer()
        {
            InitializeComponent();
        }

        private Mod when = null;

        public override void LoadSettings()
        {
            if (when == null)
                when = Mod.WhereSettingName("ShowSongTimerWhen");

            switch (when.Value)
            {
                case "manual":
                    When_Hotkey.IsChecked = true;
                    break;
                case "automatic":
                    When_InSong.IsChecked = true;
                    break;
                default: // We don't know what the user has set, so don't initialize anything.
                    break;
            }
        }

        private void When_Hotkey_Checked(object sender, RoutedEventArgs e)
        {
            when.Value = "manual";
        }

        private void When_InSong_Checked(object sender, RoutedEventArgs e)
        {
            when.Value = "automatic";
        }
    }
}
