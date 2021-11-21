using System.Windows;

namespace RSMods_WPF.Pages.ModPages
{
    /// <summary>
    /// Interaction logic for DisableHeadstock.xaml
    /// </summary>
    public partial class DisableHeadstock : ModPage
    {
        public DisableHeadstock()
        {
            InitializeComponent();
        }

        private Mod when = null;

        public override void LoadSettings()
        {
            if (when == null)
                when = Mod.WhereSettingName("RemoveHeadstockWhen");

            switch (when.Value)
            {
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

        private void When_InSong_Checked(object sender, RoutedEventArgs e) => when.Value = "song";

        private void When_Always_Checked(object sender, RoutedEventArgs e) => when.Value = "startup";
    }
}
