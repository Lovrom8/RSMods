using System;
using System.Windows;

namespace RSMods_WPF.Pages.ModPages
{
    /// <summary>
    /// Interaction logic for ForceEnumeration.xaml
    /// </summary>
    public partial class ForceEnumeration : ModPage
    {
        public ForceEnumeration()
        {
            InitializeComponent();
        }

        private Mod when = null;
        private Mod howOften = null;

        public override void LoadSettings()
        {
            if (when == null)
                when = Mod.WhereSettingName("ForceReEnumerationWhen");

            if (howOften == null)
                howOften = Mod.WhereSettingName("CheckForNewSongsInterval");

            try
            {
                Convert.ToInt32(howOften.Value);
            }
            catch // User has a number bigger than Int32 can handle.
            {
                howOften.Value = 5000;
            }

            HowOften_DUD.Value = Convert.ToDecimal(howOften.Value) / 1000;
            HowOften_DUD.Visibility = Visibility.Hidden;
            HowOften_Label.Visibility = Visibility.Hidden;

            switch (when.Value)
            {
                case "manual":
                    When_Manual.IsChecked = true;
                    break;
                case "automatic":
                    When_Automatic.IsChecked = true;
                    HowOften_DUD.Visibility = Visibility.Visible;
                    HowOften_Label.Visibility = Visibility.Visible;
                    break;
                default: // We don't know what the user has set, so don't initialize anything.
                    break;
            }
        }

        private void When_Manual_Checked(object sender, RoutedEventArgs e)
        {
            when.Value = "manual";
            HowOften_DUD.Visibility = Visibility.Hidden;
            HowOften_Label.Visibility = Visibility.Hidden;
        }
        private void When_Automatic_Checked(object sender, RoutedEventArgs e)
        {
            when.Value = "automatic";
            HowOften_DUD.Visibility = Visibility.Visible;
            HowOften_Label.Visibility = Visibility.Visible;
        }

        private void HowOften_Changed(object sender, RoutedEventArgs e) => howOften.Value = Convert.ToInt32(HowOften_DUD.Value * 1000);
    }
}
