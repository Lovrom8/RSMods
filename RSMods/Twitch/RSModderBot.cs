using Newtonsoft.Json.Linq;
using RSMods.Data;
using RSMods.Twitch;
using System;
using System.Globalization;
using System.IO;
using System.Net;
using System.Windows.Forms;
using TwitchLib.Client;
using TwitchLib.Client.Events;
using TwitchLib.Client.Models;
using TwitchLib.Communication.Clients;
using TwitchLib.Communication.Models;

namespace RSMods.Util
{
    class RSModderBot // Effectively just a chatbot with questionable usefulness
    {
        private TwitchClient client;

        /* 
         * As per Koko's suggestion, we will be parsing the chat instead of using PubSub, since it requires even more things to setup and would likely require us to 
         * use some kind of web browser to auth the user, and the integrated WebBrowser control, being based on IE7, doesn't really like Twitch (doesn't even load it on my machine due to script errors)
         * hence I would like to avoid it for now
         */

        public RSModderBot() 
        {
            if (File.Exists(Constants.SettingsPath))
            {
                TwitchSettings.Get.LoadSettings();

                Auth();
            }
            else // If it doesn't exist, write in defaults
                TwitchSettings.Get.SaveSettings(true);
        }

        private static bool RefreshToken()
        {
            using (var wc = new WebClient())
            {
                string jsonResponse = wc.DownloadString($"https://twitchtokengenerator.com/api/refresh/{TwitchSettings.Get.ChatbotRefreshToken}");
                JToken jsonToken = JObject.Parse(jsonResponse);
                string resp = (string)jsonToken.SelectToken("success");

                if (resp.ToLower() == "true")
                    TwitchSettings.Get.ChatbotAccessToken = (string)jsonToken.SelectToken("token");
                else
                {
                    MessageBox.Show("Error: we were unable to refresh your token, please generate it manually again!");
                    return false;
                }
            }

            return true;
        }

        private static bool IsTokenValid()
        {
            try
            {
                string dateRefreshedStr = GenUtil.GetSettingsEntry("TokenSaved");
                DateTime tokenRefreshedDate = DateTime.ParseExact(dateRefreshedStr, TwitchSettings.DateFormat, CultureInfo.InvariantCulture);

                int dayDiff = (DateTime.Now - tokenRefreshedDate).Days;

                if (dayDiff >= TwitchSettings.TokenExpiry)
                    return RefreshToken();

                return true;
            }
            catch (FormatException)
            {
                MessageBox.Show("We were unable to refresh your access token, please correct the date in GUI_Settings.ini or re-authorize manually on twitchtokengenerator.com!", "Error");
                return false;
            }
        }

        public static void SetCreds(string username, string accessToken, string refreshToken)
        {
            TwitchSettings.Get.ChatbotUsername = username;
            TwitchSettings.Get.ChatbotAccessToken = accessToken;
            TwitchSettings.Get.ChatbotRefreshToken = refreshToken;

            TwitchSettings.Get.SaveSettings(true);
        }

        public void Auth()
        {
            if (!IsTokenValid())
                RefreshToken();

            ConnectionCredentials credentials = new ConnectionCredentials(TwitchSettings.Get.Username, TwitchSettings.Get.AccessToken);
            var clientOptions = new ClientOptions
            {
                MessagesAllowedInPeriod = 750,
                ThrottlingPeriod = TimeSpan.FromSeconds(30)
            };
            WebSocketClient customClient = new WebSocketClient(clientOptions);
            client = new TwitchClient(customClient);
            client.Initialize(credentials, "channel");

            client.OnMessageReceived += Client_OnMessageReceived;
            client.OnConnected += Client_OnConnected;

            client.Connect();
        }

        private void Client_OnConnected(object sender, OnConnectedArgs e)
        {
            MessageBox.Show($"Connected to {e.AutoJoinChannel}");
        }

        private void Client_OnMessageReceived(object sender, OnMessageReceivedArgs e)
        {
            string currMsg = e.ChatMessage.Message.ToLower();

            if (currMsg.Contains("bits")) // It's likely better to use PubSub for bits, but we may find a use for this as well
                WinMsgUtil.SendMsgToRS("go rainbow");
        }
    }
}
