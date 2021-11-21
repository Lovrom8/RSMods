using System.Windows;

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

        private Mod when = null;

        public override void LoadSettings()
        {
            if (when == null)
                when = Mod.WhereSettingName("ToggleLoftWhen");

            switch (when.Value)
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

        private void When_Manual_Checked(object sender, RoutedEventArgs e) => when.Value = "manual";

        private void When_InSong_Checked(object sender, RoutedEventArgs e) => when.Value = "song";

        private void When_Always_Checked(object sender, RoutedEventArgs e) => when.Value = "startup";
    }
}
