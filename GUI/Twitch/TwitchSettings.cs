using RSMods.Data;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RSMods.Twitch
{
    public sealed class TwitchSettings : INotifyPropertyChanged
    {
        public List<TwitchReward> Rewards = new List<TwitchReward>();
        public List<TwitchReward> DefaultRewards = new List<TwitchReward>();

        private static TwitchTokenStore TokenStore => new TwitchTokenStore(Constants.TwitchTokenPath);
        private static TwitchRewardRepository RewardRepository => new TwitchRewardRepository(Constants.TwitchRewardsPath);

        private readonly Queue<string> _logLines = new Queue<string>();
        private string _username = string.Empty;
        private string _channelId = string.Empty;
        private bool _authorized;
        private bool _authorizationStored;
        private string _listeningStatus = "Not authorized with Twitch";
        private string _log = string.Empty;

        public string Username
        {
            get => _username;
            set => SetField(ref _username, value ?? string.Empty);
        }

        public string ChannelID
        {
            get => _channelId;
            set => SetField(ref _channelId, value ?? string.Empty);
        }

        public bool Authorized
        {
            get => _authorized;
            set => SetField(ref _authorized, value);
        }

        public bool AuthorizationStored
        {
            get => _authorizationStored;
            set => SetField(ref _authorizationStored, value);
        }

        public string ListeningStatus
        {
            get => _listeningStatus;
            set => SetField(ref _listeningStatus, value ?? string.Empty);
        }

        public string Log
        {
            get => _log;
            private set => SetField(ref _log, value ?? string.Empty);
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public void LoadSettings()
        {
            var settings = new FlatKeyValueSettingsStore(Constants.SettingsPath);
            TwitchTokenSet tokens = TokenStore.LoadOrImportLegacy(Constants.SettingsPath, TwitchOptions.RSModsClientId);
            AuthorizationStored = tokens != null;
            Username = settings.GetString("Username");
            ChannelID = settings.GetString("ChannelID");
        }

        public void SaveSettings()
        {
            try
            {
                FlatKeyValueSettingsStore.UpdateFile(Constants.SettingsPath, settings =>
                {
                    settings.SetString("RSPath", Constants.RSFolder);
                    settings.SetString("SavePath", Constants.SavePath);
                    settings.SetString("BypassSavePrompt", Constants.SavePathDeclined.ToString());
                    settings.SetString("Username", Username);
                    settings.SetString("ChannelID", ChannelID);
                    settings.Remove("AccessToken");
                    settings.Remove("ForceReauth");
                });
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Error: {ioex.Message}", "Error");
            }
        }

        public void LoadDefaultEffects() => DefaultRewards = TwitchRewardCatalog.CreateDefaults();

        public Task SaveRewards() => RewardRepository.SaveAsync(Rewards);

        public void LoadEnabledEffects() => Rewards = RewardRepository.Load();

        public void SaveLog(string path) => File.WriteAllText(path, Log);

        public void AddToLog(string newEntry)
        {
            _logLines.Enqueue(newEntry ?? string.Empty);
            while (_logLines.Count > 500)
                _logLines.Dequeue();
            Log = string.Join(Environment.NewLine, _logLines) + Environment.NewLine;
        }

        private void SetField<T>(ref T field, T value, [CallerMemberName] string propertyName = "")
        {
            if (EqualityComparer<T>.Default.Equals(field, value))
                return;
            field = value;
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private TwitchSettings()
        {
        }

        public static readonly TwitchSettings Get = new TwitchSettings();
    }
}
