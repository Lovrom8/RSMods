using RSMods.Data;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Net.Http;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Xml.Serialization;

namespace RSMods.Twitch
{
    public class TwitchSettings : INotifyPropertyChanged
    {
        public const string DateFormat = "yyyy-MM-dd";
        public const short TokenExpiry = 50; // It's actually 60 days, but doesn't matter too much

        public static readonly HttpClient client = new HttpClient();

        public List<TwitchReward> Rewards = new List<TwitchReward>(); // Choosen by the streamer
        public List<TwitchReward> DefaultRewards = new List<TwitchReward>(); // All of which can be chosen

        private string _accessToken { get; set; }
        private string _username { get; set; }
        private string _channelID { get; set; }
        private bool _authorized { get; set; }
        private string _log { get; set; }

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
        public bool Authorized
        {
            get => _authorized;
            set
            {
                if (_authorized == value)
                    return;

                _authorized = value;

                NotifyPropertyChanged();
            }
        }

        public string Log
        {
            get => _log;
            set
            {
                if (_log == value)
                    return;

                _log = value;

                NotifyPropertyChanged();
            }
        }

        public bool Reauthorized { get; set; }
        public bool ForceReauth { get; set; }

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

            bool _ForceReauth;
            Boolean.TryParse(GenUtil.GetSettingsEntry("ForceReauth"), out _ForceReauth);
            ForceReauth = _ForceReauth;

            ChatbotAccessToken = GenUtil.GetSettingsEntry("Chatbot_AccessToken");
            ChatbotRefreshToken = GenUtil.GetSettingsEntry("Chatbot_RefreshToken");
            ChatbotTokenSaved = GenUtil.GetSettingsEntry("Chatbot_TokenSaved");
            ChatbotUsername = GenUtil.GetSettingsEntry("Chatbot_Username");

            if (AccessToken != "")
            {
                Authorized = await ValidateCurrentSettings();
                Reauthorized = Authorized;
            }
            else
                Authorized = false;
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
                $"ChannelID = {ChannelID}",
                $"ForceReauth = {ForceReauth}"
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

        public void LoadDefaultEffects()
        {
            // We may want to load these from a file, but then again, these need to be added in both the GUI and the DLL so yeah 
            //if (File.Exists("TwitchDefaultEffects.xml"))
            //    return;

            DefaultRewards.AddRange(new List<TwitchReward> {
                new TwitchReward("Rainbow Strings", "Your strings will continuously shift colors like a rainbow", "rainbowstrings"),
                new TwitchReward("Rainbow Notes", "The notes on screeen will continously shift colors like a rainbow", "rainbownotes"),
                new TwitchReward("Remove Notes", "Make noteheads not show at all",  "removenotes"),
                new TwitchReward("Transparent Notes", "Make noteheads transparent",  "transparentnotes"),
                new TwitchReward("Solid color notes", "Make all notes have the same color",  "solidnotes"),
                new TwitchReward("Drunk Mode", "Have the background freak out because you're drunk",  "drunkmode"),
                new TwitchReward("F Your FC", "Makes your current note streak go to 0",  "fyourfc"),
                new TwitchReward("Shuffle your tones", "Switch your tone every few seconds",  "shuffletones"),
                new TwitchReward("Change tone - slot 1", "Switch your tone to slot 1",  "changetoneslot1"),
                new TwitchReward("Change tone - slot 2", "Switch your tone to slot 2",  "changetoneslot2"),
                new TwitchReward("Change tone - slot 3", "Switch your tone to slot 3",  "changetoneslot3"),
                new TwitchReward("Change tone - slot 4", "Switch your tone to slot 4",  "changetoneslot4"),
                new TwitchReward("Remove Instrument", "Removes your instrument (strings, fretsbars & headstock)",  "removeinstrument"),
                new TwitchReward("Kill guitar volume", "Kills volume of your guitar",  "killguitarvolume"),
                new TwitchReward("Kill music volume", "Kills volume of the music",  "killmusicvolume"),
                new TwitchReward("Big note heads", "Makes all note heads big",  "bignoteheads"),
                new TwitchReward("Small note heads", "Makes all note heads tiny",  "smallnoteheads"),
                new TwitchReward("Half scroll speed", "Makes scroll speed half (ramp-up effect - not instant!)",  "halfsongspeed"),
                new TwitchReward("Double scroll speed", "Makes scroll speed double (ramp-up effect)",  "doublesongspeed"),
                new TwitchReward("Zoom camera in", "Zooms in the main camera",  "zoomin"),
                new TwitchReward("TurboSpeed", "EXPERIMENTAL: turn up song speed, a lot. NOTE: do not ALT-TAB mid song or this won't work!",  "turbospeed") 
            });

            /*XmlSerializer xs = new XmlSerializer(TwitchSettings.Get.DefaultRewards.GetType());
            using (var sww = new StringWriter())
            {
                using (XmlWriter writer = XmlWriter.Create(sww, new XmlWriterSettings { Indent = true }))
                {

                    xs.Serialize(writer, TwitchSettings.Get.DefaultRewards);
                    File.WriteAllText("TwitchDefaultEffects.xml", sww.ToString());
                }
            }*/
        }

        public void LoadEnabledEffects()
        {
            if (!File.Exists("TwitchEnabledEffects.xml"))
                return;

            using (var reader = new StreamReader("TwitchEnabledEffects.xml"))
            {
                XmlSerializer deserializer = new XmlSerializer(typeof(List<TwitchReward>));
                Rewards = (List<TwitchReward>)deserializer.Deserialize(reader);
            }
        }

        public void AddToLog(string newEntry)
        {
            Log += newEntry;
            Log += Environment.NewLine;
        }

        private TwitchSettings() { }
        public static readonly TwitchSettings Get = new TwitchSettings();
    }
}
