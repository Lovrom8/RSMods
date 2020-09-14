using RSMods.Data;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RSMods.Twitch
{
    public class TwitchSettings : INotifyPropertyChanged
    {
        public const string DateFormat = "yyyy-MM-dd";
        public const short TokenExpiry = 50; // It's actually 60 days, but doesn't matter too much

        public static readonly HttpClient client = new HttpClient();

        private string _accessToken { get; set; }
        private string _username { get; set; }
        private string _channelID { get; set; }

        public SynchronizationContext _context { get; set; } // Gotta send this from the main thread, so we change it in MainForm

        public string AccessToken
        {
            get => _accessToken;
            set
            {
                if (_accessToken == value)
                    return;

                _accessToken = value;

                NotifyPropertyChanged();
            }
        }

        public string Username
        {
            get => _username;
            set
            {
                if (_username == value)
                    return;

                _username = value;

                NotifyPropertyChanged();
            }
        }

        public string ChannelID 
        { 
            get => _channelID;
            set
            {
                if (_channelID == value)
                    return;

                _channelID = value;

                NotifyPropertyChanged();
            }
        }
        public bool Authorized { get; set; }

        public string Info { get => $"{TwitchSettings.Get.Username} with channel ID: {TwitchSettings.Get.ChannelID} and access token: {TwitchSettings.Get.AccessToken}"; }

        public string ChatbotUsername { get; set; }
        public string ChatbotAccessToken { get; set; }
        public string ChatbotRefreshToken { get; set; }
        public string ChatbotTokenSaved { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged([CallerMemberName] String propertyName = "")
        {
            _context.Post(delegate 
             {
                 PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
             }, null);

            // In order to do data-binding in multi-threaded WinForms, you gotta do a bit of fancy codenz :(
        }

        public async Task<bool> ValidateCurrentSettings()
        {
            var userAPIRequest = new HttpRequestMessage
            {
                RequestUri = new Uri("https://id.twitch.tv/oauth2/validate"),
                Method = HttpMethod.Get,
                Headers =
                {
                    { "Authorization", $"OAuth {AccessToken}"}
                }
            };

            var response = await client.SendAsync(userAPIRequest);
            var responseDataJson = await response.Content.ReadAsStringAsync();

            return !responseDataJson.Contains("invalid access token");
        }

        public async void LoadSettings() // At this point I feel like it would come in handy just to save it as XML/JSON and deserialize it when needed
        {
            var settingsLines = GenUtil.GetSettingsLines();
            GenUtil.GetSettingsPairs(settingsLines);

            AccessToken = GenUtil.GetSettingsEntry("AccessToken");
            Username = GenUtil.GetSettingsEntry("Username");
            ChannelID = GenUtil.GetSettingsEntry("ChannelID");

            ChatbotAccessToken = GenUtil.GetSettingsEntry("Chatbot_AccessToken");
            ChatbotRefreshToken = GenUtil.GetSettingsEntry("Chatbot_RefreshToken");
            ChatbotTokenSaved = GenUtil.GetSettingsEntry("Chatbot_TokenSaved");
            ChatbotUsername = GenUtil.GetSettingsEntry("Chatbot_Username");

            Authorized = await ValidateCurrentSettings();
        }

        public void SaveSettings(bool refreshDate = false)
        {
            var configLines = new List<string>
            {
                $"RSPath = {Constants.RSFolder}",

                $"Chatbot_Username = {ChatbotUsername}",
                $"Chatbot_AccessToken = {ChatbotAccessToken}",
                $"Chatbot_RefreshToken = {ChatbotRefreshToken}",
                $"Chatbot_TokenSaved = {(refreshDate ? ChatbotTokenSaved = DateTime.Now.ToString(DateFormat) : ChatbotTokenSaved )}",

                $"AccessToken = {AccessToken}",
                $"Username = {Username}",
                $"ChannelID = {ChannelID}"
            };

            try
            {
                File.WriteAllLines(Constants.SettingsPath, configLines);
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Error: {ioex.Message}", "Error");
            }
        }

        private TwitchSettings() { }
        public static readonly TwitchSettings Get = new TwitchSettings();
    }
}
