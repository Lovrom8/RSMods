using RSMods.Data;
using Newtonsoft.Json.Linq;
using RSMods.Data;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using TwitchLib.Client;
using TwitchLib.Client.Enums;
using TwitchLib.Client.Events;
using TwitchLib.Client.Models;
using TwitchLib.Communication.Clients;
using TwitchLib.Communication.Models;
using System.Globalization;

namespace RSMods.Util
{
    class RSModderBot
    {
        TwitchClient client;

        private static string twitchUsername { get; set; }
        private static string twitchAccessToken { get; set; }
        private static string twitchRefreshToken { get; set; }
        private static string tokenRefreshedDate { get; set; }

        private const string dateFormat = "yyyy-MM-dd";
        private const short tokenExpiry = 50; // It's actually 60 days, but doesn't matter too much

        /* 
         * As per Koko's suggestion, we will be parsing the chat instead of using PubSub, since it requires even more things to setup and would likely require us to 
         * use some kind of web browser to auth the user, and the integrated WebBrowser control, being based on IE7, doesn't really like Twitch (doesn't even load it on my machine due to script errors)
         * hence I would like to avoid it for now
         */

        public RSModderBot() // TODO: give it some GUI parts to work with - we need to ask users to auth with this settings https://twitchtokengenerator.com/quick/rN48qv1A3v, then they copy Access Token & Refresh Token and put in here 
        {
            if (File.Exists(Constants.SettingsPath))
            {
                twitchUsername = GenUtil.GetSettingsEntry("Username");
                twitchAccessToken = GenUtil.GetSettingsEntry("AccessToken");
                twitchRefreshToken = GenUtil.GetSettingsEntry("RefreshToken");
                tokenRefreshedDate = GenUtil.GetSettingsEntry("TokenSaved");

                Auth();
            }
            else
                SaveCreds();
        }

        public static void SaveCreds()
        {
            var configLines = new List<string>();
            configLines.Add($"$RSPath = {Constants.RSFolder}");
            configLines.Add($"Username = {twitchUsername}");
            configLines.Add($"AccessToken = {twitchAccessToken}");
            configLines.Add($"RefreshToken = {twitchRefreshToken}");
            configLines.Add($"TokenSaved = {DateTime.Now.ToString(dateFormat)}");

            try
            {
                File.WriteAllLines(Constants.SettingsPath, configLines);
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Error: {ioex.Message}", "Error");
            }
        }

        private static bool RefreshToken()
        {
            using (var wc = new WebClient())
            {
                string jsonResponse = wc.DownloadString($"https://twitchtokengenerator.com/api/refresh/{twitchRefreshToken}");
                JToken jsonToken = JObject.Parse(jsonResponse);
                string resp = (string)jsonToken.SelectToken("success");

                if (resp.ToLower() == "true")
                    twitchAccessToken = (string)jsonToken.SelectToken("token");
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
                DateTime tokenRefreshedDate = DateTime.ParseExact(dateRefreshedStr, dateFormat, CultureInfo.InvariantCulture);

                int dayDiff = (DateTime.Now - tokenRefreshedDate).Days;

                if (dayDiff >= tokenExpiry)
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
            twitchUsername = username;
            twitchAccessToken = accessToken;
            twitchRefreshToken = refreshToken;

            SaveCreds();
        }

        public void Auth()
        {
            if (!IsTokenValid())
                RefreshToken();

            ConnectionCredentials credentials = new ConnectionCredentials(twitchUsername, twitchAccessToken);
            var clientOptions = new ClientOptions
            {
                MessagesAllowedInPeriod = 750,
                ThrottlingPeriod = TimeSpan.FromSeconds(30)
            };
            WebSocketClient customClient = new WebSocketClient(clientOptions);
            client = new TwitchClient(customClient);
            client.Initialize(credentials, "channel");

            client.OnMessageReceived += Client_OnMessageReceived;
            client.OnNewSubscriber += Client_OnNewSubscriber;
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

            if (currMsg.Contains("bits")) //TODO: actually determine what happens when certain events occur (bits, subs, etc.)
                WinMsgUtil.SendMsgToRS("go rainbow");
        }

        private void Client_OnNewSubscriber(object sender, OnNewSubscriberArgs e)
        {
            //if (e.Subscriber.SubscriptionPlan == SubscriptionPlan.Prime)
            //etc.
        }
    }
}
