using RSMods.Data;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RSMods.Twitch
{
    public static class TwitchSettings
    {
        public static string AccessToken { get; set; }
        public static string Username { get; set; }
        public static string ChannelID { get; set; }

        public static bool Authorized { get; set; }

        public static string ChatbotUsername { get; set; }
        public static string ChatbotAccessToken { get; set; }
        public static string ChatbotRefreshToken { get; set; }
        public static string ChatbotTokenSaved { get; set; }

        public const string DateFormat = "yyyy-MM-dd";
        public const short TokenExpiry = 50; // It's actually 60 days, but doesn't matter too much

        public static readonly HttpClient client = new HttpClient();

        public static async Task<bool> ValidateCurrentSettings()
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

        public static async void LoadSettings() // At this point I feel like it would come in handy just to save it as XML/JSON and deserialize it when needed
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

        public static void SaveSettings(bool refreshDate = false)
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
    }
}
