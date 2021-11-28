using System.Collections.Generic;
using RSMods_WPF.Asio;
using System.Windows.Controls;
using System.Windows;
using System;
using Xceed.Wpf.Toolkit;

namespace RSMods_WPF.Pages.SettingsPages
{
    /// <summary>
    /// Interaction logic for ASIO.xaml
    /// </summary>
    public partial class ASIO : SettingsPage
    {
        public static new ASIO Instance = new();
        public List<string> AsioDeviceNames = new();
        bool ReadyToUseSettings = false;
        public ASIO()
        {
            InitializeComponent();
            GetAsioDevices();
            Startup_LoadInitialValues();
        }

        private void GetAsioDevices()
        {
            AsioDeviceNames.Clear();
            
            Devices.FindDevices().ForEach(driver => AsioDeviceNames.Add(driver.deviceName));

            Output_Devices.ItemsSource = AsioDeviceNames;
            Input0_Devices.ItemsSource = AsioDeviceNames;
            Input1_Devices.ItemsSource = AsioDeviceNames;
            InputMic_Devices.ItemsSource = AsioDeviceNames;

            if (AsioDeviceNames.Contains(AsioSetting.WhereSettingName("Driver", "Asio.Output").Value.ToString()))
                Output_Devices.SelectedItem = AsioSetting.WhereSettingName("Driver", "Asio.Output").Value.ToString();

            if (AsioDeviceNames.Contains(AsioSetting.WhereSettingName("Driver", "Asio.Input.0").Value.ToString()))
                Input0_Devices.SelectedItem = AsioSetting.WhereSettingName("Driver", "Asio.Input.0").Value.ToString();

            if (AsioDeviceNames.Contains(AsioSetting.WhereSettingName("Driver", "Asio.Input.1").Value.ToString()))
                Input1_Devices.SelectedItem = AsioSetting.WhereSettingName("Driver", "Asio.Input.1").Value.ToString();

            if (AsioDeviceNames.Contains(AsioSetting.WhereSettingName("Driver", "Asio.Input.Mic").Value.ToString()))
                InputMic_Devices.SelectedItem = AsioSetting.WhereSettingName("Driver", "Asio.Input.Mic").Value.ToString();
        }

        private void Startup_LoadInitialValues()
        {
            // Config
            WASAPI_Output.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableWasapiOutputs", "Config").Value.ToString(), 0) == 1;
            WASAPI_Input.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableWasapiInputs", "Config").Value.ToString(), 0) == 1;
            UseAsio.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableAsio", "Config").Value.ToString(), 1) == 1;

            // Asio
            CustomBufferSize.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("CustomBufferSize", "Asio").Value.ToString(), 48);

            switch (AsioSetting.WhereSettingName("BufferSizeMode", "Asio").Value)
            {
                case "driver":
                    BufferSizeDriver.IsChecked = true;
                    CustomBufferSize.Visibility = Visibility.Hidden;
                    Label_CustomBufferSize.Visibility = Visibility.Hidden;
                    break;
                case "host":
                    BufferSizeHost.IsChecked = true;
                    CustomBufferSize.Visibility = Visibility.Hidden;
                    Label_CustomBufferSize.Visibility = Visibility.Hidden;
                    break;
                case "custom":
                    BufferSizeCustom.IsChecked = true;
                    break;
                default: // Unknown value
                    break; 
            }

            // Disabled Statuses
            Input0_Disabled.IsChecked   = LoadAsio.DisabledDevices.Contains("Asio.Input.0");
            Input1_Disabled.IsChecked   = LoadAsio.DisabledDevices.Contains("Asio.Input.1");
            InputMic_Disabled.IsChecked = LoadAsio.DisabledDevices.Contains("Asio.Input.Mic");
            Output_Disabled.IsChecked   = LoadAsio.DisabledDevices.Contains("Asio.Output");

            // Asio.Output
            Output_BaseChannel.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("BaseChannel", "Asio.Output").Value.ToString(), 0);
            Output_AltBaseChannel.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("AltBaseChannel", "Asio.Output").Value.ToString(), null);
            Output_EnableMasterVolume.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Output").Value.ToString(), 1) == 1;
            Output_EnableEndpointVolume.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Output").Value.ToString(), 1) == 1;
            Output_MasterVolume.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Output").Value.ToString(), 100);
            if (!(bool)Output_EnableMasterVolume.IsChecked)
            {
                Output_MasterVolumeLabel.Visibility = Visibility.Hidden;
                Output_MasterVolume.Visibility = Visibility.Hidden;
            }

            // Asio.Input.0
            Input0_Channel.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("Channel", "Asio.Input.0").Value.ToString(), 0);
            Input0_EnableMasterVolume.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.0").Value.ToString(), 1) == 1;
            Input0_EnableEndpointVolume.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.0").Value.ToString(), 1) == 1;
            Input0_MasterVolume.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.0").Value.ToString(), 100);
            if (!(bool)Input0_EnableMasterVolume.IsChecked)
            {
                Input0_MasterVolumeLabel.Visibility = Visibility.Hidden;
                Input0_MasterVolume.Visibility = Visibility.Hidden;
            }

            // Asio.Input.1
            Input1_Channel.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("Channel", "Asio.Input.1").Value.ToString(), 0);
            Input1_EnableMasterVolume.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.1").Value.ToString(), 1) == 1;
            Input1_EnableEndpointVolume.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.1").Value.ToString(), 1) == 1;
            Input1_MasterVolume.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.1").Value.ToString(), 100);
            if (!(bool)Input1_EnableMasterVolume.IsChecked)
            {
                Input1_MasterVolumeLabel.Visibility = Visibility.Hidden;
                Input1_MasterVolume.Visibility = Visibility.Hidden;
            }

            // Asio.Input.Mic
            InputMic_Channel.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("Channel", "Asio.Input.Mic").Value.ToString(), 0);
            InputMic_EnableMasterVolume.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.Mic").Value.ToString(), 1) == 1;
            InputMic_EnableEndpointVolume.IsChecked = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.Mic").Value.ToString(), 1) == 1;
            InputMic_MasterVolume.Value = Utils.ConvertStringToInt(AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.Mic").Value.ToString(), 100);
            if (!(bool)InputMic_EnableMasterVolume.IsChecked)
            {
                InputMic_MasterVolumeLabel.Visibility = Visibility.Hidden;
                InputMic_MasterVolume.Visibility = Visibility.Hidden;
            }

            ReadyToUseSettings = true;
        }

        private void SelectedAsioDevice(object sender, SelectionChangedEventArgs e)
        {
            switch(((ListBox)sender).Name)
            {
                case "Output_Devices":
                    AsioSetting.WhereSettingName("Driver", "Asio.Output").Value = ((ListBox)sender).SelectedItem.ToString();
                    break;
                case "Input0_Devices":
                    AsioSetting.WhereSettingName("Driver", "Asio.Input.0").Value = ((ListBox)sender).SelectedItem.ToString();
                    break;
                case "Input1_Devices":
                    AsioSetting.WhereSettingName("Driver", "Asio.Input.1").Value = ((ListBox)sender).SelectedItem.ToString();
                    break;
                case "InputMic_Devices":
                    AsioSetting.WhereSettingName("Driver", "Asio.Input.Mic").Value = ((ListBox)sender).SelectedItem.ToString();
                    break;
                default: // Unknown ListBox
                    break;
            }
        }

        private void CheckStateChanged(object sender, RoutedEventArgs e)
        {
            if (ReadyToUseSettings)
            {
                bool isChecked = (bool)((CheckBox)sender).IsChecked;
                switch (((CheckBox)sender).Name)
                {
                    // Config
                    case "WASAPI_Output":
                        AsioSetting.WhereSettingName("EnableWasapiOutputs", "Config").Value = Convert.ToInt32(isChecked);
                        break;
                    case "WASAPI_Input":
                        AsioSetting.WhereSettingName("EnableWasapiInputs", "Config").Value = Convert.ToInt32(isChecked);
                        break;
                    case "UseAsio":
                        AsioSetting.WhereSettingName("EnableAsio", "Config").Value = Convert.ToInt32(isChecked);
                        break;

                    // Disabled Statuses
                    case "Input0_Disabled":
                        if (isChecked)
                            LoadAsio.DisabledDevices.Add("Asio.Input.0");
                        else
                            LoadAsio.DisabledDevices.Remove("Asio.Input.0");
                        LoadAsio.WriteSettingsFile(AsioSetting.WhereSettingName("Driver", "Asio.Input.0"));
                        break;
                    case "Input1_Disabled":
                        if (isChecked)
                            LoadAsio.DisabledDevices.Add("Asio.Input.1");
                        else
                            LoadAsio.DisabledDevices.Remove("Asio.Input.1");
                        LoadAsio.WriteSettingsFile(AsioSetting.WhereSettingName("Driver", "Asio.Input.1"));
                        break;
                    case "InputMic_Disabled":
                        if (isChecked)
                            LoadAsio.DisabledDevices.Add("Asio.Input.Mic");
                        else
                            LoadAsio.DisabledDevices.Remove("Asio.Input.Mic");
                        LoadAsio.WriteSettingsFile(AsioSetting.WhereSettingName("Driver", "Asio.Input.Mic"));
                        break;
                    case "Output_Disabled":
                        if (isChecked)
                            LoadAsio.DisabledDevices.Add("Asio.Output");
                        else
                            LoadAsio.DisabledDevices.Remove("Asio.Output");
                        LoadAsio.WriteSettingsFile(AsioSetting.WhereSettingName("Driver", "Asio.Output"));
                        break;

                    // Volume Control
                    case "Input0_EnableMasterVolume":
                        AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.0").Value = Convert.ToInt32(isChecked);
                        Input0_MasterVolumeLabel.Visibility = (Visibility)Convert.ToInt32(!isChecked);
                        Input0_MasterVolume.Visibility = (Visibility)Convert.ToInt32(!isChecked);
                        break;
                    case "Input0_EnableEndpointVolume":
                        AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.0").Value = Convert.ToInt32(isChecked);
                        break;
                    case "Input1_EnableMasterVolume":
                        AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.1").Value = Convert.ToInt32(isChecked);
                        Input1_MasterVolumeLabel.Visibility = (Visibility)Convert.ToInt32(!isChecked);
                        Input1_MasterVolume.Visibility = (Visibility)Convert.ToInt32(!isChecked);
                        break;
                    case "Input1_EnableEndpointVolume":
                        AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.1").Value = Convert.ToInt32(isChecked);
                        break;
                    case "InputMic_EnableMasterVolume":
                        AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.Mic").Value = Convert.ToInt32(isChecked);
                        InputMic_MasterVolumeLabel.Visibility = (Visibility)Convert.ToInt32(!isChecked);
                        InputMic_MasterVolume.Visibility = (Visibility)Convert.ToInt32(!isChecked);
                        break;
                    case "InputMic_EnableEndpointVolume":
                        AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.Mic").Value = Convert.ToInt32(isChecked);
                        break;
                    case "Output_EnableMasterVolume":
                        AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Output").Value = Convert.ToInt32(isChecked);
                        Output_MasterVolumeLabel.Visibility = (Visibility)Convert.ToInt32(!isChecked);
                        Output_MasterVolume.Visibility = (Visibility)Convert.ToInt32(!isChecked);
                        break;
                    case "Output_EnableEndpointVolume":
                        AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Output").Value = Convert.ToInt32(isChecked);
                        break;

                    default: // Unknown setting!
                        break;
                }
            }
        }

        private void CustomBufferSizeChanged(object sender, RoutedPropertyChangedEventArgs<object> e) => AsioSetting.WhereSettingName("CustomBufferSize", "Asio").Value = ((IntegerUpDown)sender).Value;

        private void RadioStateChanged(object sender, RoutedEventArgs e)
        {
            switch(((RadioButton)sender).Name)
            {
                case "BufferSizeDriver":
                    AsioSetting.WhereSettingName("BufferSizeMode", "Asio").Value = "driver";
                    AsioSetting.WhereSettingName("CustomBufferSize", "Asio").Value = null;
                    CustomBufferSize.Visibility = Visibility.Hidden;
                    Label_CustomBufferSize.Visibility = Visibility.Hidden;
                    break;
                case "BufferSizeHost":
                    AsioSetting.WhereSettingName("BufferSizeMode", "Asio").Value = "host";
                    AsioSetting.WhereSettingName("CustomBufferSize", "Asio").Value = null;
                    CustomBufferSize.Visibility = Visibility.Hidden;
                    Label_CustomBufferSize.Visibility = Visibility.Hidden;
                    break;
                case "BufferSizeCustom":
                    AsioSetting.WhereSettingName("BufferSizeMode", "Asio").Value = "custom";
                    AsioSetting.WhereSettingName("CustomBufferSize", "Asio").Value = CustomBufferSize.Value;
                    CustomBufferSize.Visibility = Visibility.Visible;
                    Label_CustomBufferSize.Visibility = Visibility.Visible;
                    break;
                default: // Unknown setting!
                    break;
            }
        }

        private void ChannelChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            switch(((IntegerUpDown)sender).Name)
            {
                case "Input0_Channel":
                    AsioSetting.WhereSettingName("Channel", "Asio.Input.0").Value = ((IntegerUpDown)sender).Value;
                    break;
                case "Input1_Channel":
                    AsioSetting.WhereSettingName("Channel", "Asio.Input.1").Value = ((IntegerUpDown)sender).Value;
                    break;
                case "InputMic_Channel":
                    AsioSetting.WhereSettingName("Channel", "Asio.Input.Mic").Value = ((IntegerUpDown)sender).Value;
                    break;
                case "Output_BaseChannel":
                    AsioSetting.WhereSettingName("BaseChannel", "Asio.Output").Value = ((IntegerUpDown)sender).Value;
                    break;
                case "Output_AltBaseChannel":
                    AsioSetting.WhereSettingName("AltBaseChannel", "Asio.Output").Value = ((IntegerUpDown)sender).Value;
                    break;
                default: // Unknown Setting!
                    break;
            }
        }

        private void MasterVolumeChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            switch(((IntegerUpDown)sender).Name)
            {
                case "Input0_MasterVolume":
                    AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.0").Value = ((IntegerUpDown)sender).Value;
                    break;
                case "Input1_MasterVolume":
                    AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.1").Value = ((IntegerUpDown)sender).Value;
                    break;
                case "InputMic_MasterVolume":
                    AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.Mic").Value = ((IntegerUpDown)sender).Value;
                    break;
                case "Output_MasterVolume":
                    AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Output").Value = ((IntegerUpDown)sender).Value;
                    break;
                default: // Unknown Setting!
                    break;
            }
        }

        private void ResetDeviceSettings(object sender, RoutedEventArgs e)
        {
            if (System.Windows.MessageBox.Show($"Are you sure you want to reset your settings for {((Button)sender).Name.Replace("_ResetDevice", "")}", "Are you sure?", MessageBoxButton.YesNo, MessageBoxImage.Warning) == MessageBoxResult.Yes)
            {
                switch (((Button)sender).Name)
                {
                    case "Input0_ResetDevice":
                        AsioSetting.WhereSettingName("Driver", "Asio.Input.0").Value = string.Empty;
                        AsioSetting.WhereSettingName("Channel", "Asio.Input.0").Value = 0;
                        AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.0").Value = 1;
                        AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.0").Value = 1;
                        AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.0").Value = 100;
                        LoadAsio.DisabledDevices.Remove("Asio.Input.0");
                        break;
                    case "Input1_ResetDevice":
                        AsioSetting.WhereSettingName("Driver", "Asio.Input.1").Value = string.Empty;
                        AsioSetting.WhereSettingName("Channel", "Asio.Input.1").Value = 1;
                        AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.1").Value = 1;
                        AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.1").Value = 1;
                        AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.1").Value = 100;
                        LoadAsio.DisabledDevices.Remove("Asio.Input.1");
                        break;
                    case "InputMic_ResetDevice":
                        AsioSetting.WhereSettingName("Driver", "Asio.Input.Mic").Value = string.Empty;
                        AsioSetting.WhereSettingName("Channel", "Asio.Input.Mic").Value = 0;
                        AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Input.Mic").Value = 1;
                        AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Input.Mic").Value = 1;
                        AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Input.Mic").Value = 100;
                        LoadAsio.DisabledDevices.Remove("Asio.Input.Mic");
                        break;
                    case "Output_ResetDevice":
                        AsioSetting.WhereSettingName("Driver", "Asio.Output").Value = string.Empty;
                        AsioSetting.WhereSettingName("BaseChannel", "Asio.Output").Value = 0;
                        AsioSetting.WhereSettingName("AltBaseChannel", "Asio.Output").Value = string.Empty;
                        AsioSetting.WhereSettingName("EnableSoftwareEndpointVolumeControl", "Asio.Output").Value = 1;
                        AsioSetting.WhereSettingName("EnableSoftwareMasterVolumeControl", "Asio.Output").Value = 1;
                        AsioSetting.WhereSettingName("SoftwareMasterVolumePercent", "Asio.Output").Value = 100;
                        LoadAsio.DisabledDevices.Remove("Asio.Output");
                        break;
                    default: // Unknown Button!
                        break;
                }
                
                ResetPage();
            }
        }

        private void ResetPage()
        {
            Instance = new ASIO();
            (Window.GetWindow(Application.Current.MainWindow) as MainWindow).MainFrame.Content = Instance;
        }
    }
}
