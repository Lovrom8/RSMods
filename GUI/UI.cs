using System;
using System.Threading.Tasks;
using System.Windows.Forms;
using RSMods.Core;
using RSMods.Data;
using RSMods.Util;
using System.Reflection;
using static RSMods.RsModsSettings;
using RSMods.ASIO;
using RSMods.Rocksmith;

namespace RSMods
{
    public partial class MainForm : Form
    {
        /// <summary>
        /// Leaving this boolean in-case we need to quickly disable the Profiles tab due to a bug.
        /// </summary>
        private readonly bool shipProfileEdits = true;

        int ProfileEditsTabIndex;

        private readonly bool AllowSaving = true;

        public MainForm()
        {
            InitializeComponent();
            SetupWindowMetadata();

            LoadEnvironment();

            WireUpEvents();

            LoadVisualDefaults();
            LoadKeybindingSettings();
            LoadModState();

            FinalizeStartup();
        }

        private void SetupWindowMetadata()
        {
            var version = Assembly.GetExecutingAssembly().GetName().Version;
            Text = $"{Text} - v{version.Major}.{version.Minor}.{version.Build}";
        }

        private void LoadEnvironment()
        {
            // The WinForms dialog adapter runs its dialogs inline and returns already-completed tasks, so
            // blocking here is safe and keeps the constructor synchronous. The Avalonia shell will await the
            // same resolver from its async startup instead of blocking.
            ResolveEnvironmentAsync().GetAwaiter().GetResult();
        }

        private async Task ResolveEnvironmentAsync()
        {
            await RSLocationResolver.ResolveRSFolderAsync(AppServices.Dialogs, AppServices.Environment);
            Startup_ReadIniOrCreateDefault();
            await RSLocationResolver.ResolveSaveFolderAsync(AppServices.Dialogs);
            Startup_ApplySaveFolderUI();
            ExeUtil.CheckInstallIntegrity(GenUtil.GetRSDirectory());
            Constants.SaveBaseSettings();
        }

        private void WireUpEvents()
        {
            RsModsSettings.SettingChanged += OnSettingChanged;
            RocksmithSettings.SettingChanged += OnSettingChanged;
            AsioSettings.SettingChanged += OnSettingChanged;

            this.Shown += MainForm_Shown;
        }

        private void LoadVisualDefaults()
        {
            StringColors_LoadDefaultStringColors();
            StringColors_LoadDefaultNoteColors();
            NotewayColors_LoadDefaultStringColors();
            CustomTheme_LoadCustomColors();
            Fonts_Load();
        }

        private void LoadKeybindingSettings()
        {
            Startup_LoadKeybindingModNames();
            Startup_LoadAudioKeybindings();
            Startup_ShowCurrentKeybindingValues();
            Startup_ShowCurrentAudioKeybindingValues();
        }

        private void LoadModState()
        {
            Midi_LoadDevices();
            Startup_LoadInputDevices();
            Startup_VerifyInstallOfASIO();

            PriorSettings_LoadASIOSettings();
            PriorSettings_LoadRocksmithSettings();
            PriorSettings_LoadModSettings();

            Startup_LoadRocksmithProfiles();
            Startup_UnpackCachePsarc();
            SetForget_LoadSetAndForgetMods();

            Startup_CheckStatusAudioPsarc();
            SoundPacks_LoadResultVoiceOverList();
        }

        private void FinalizeStartup()
        {
            Startup_BackupProfiles();
            Startup_DeleteOldBackups(GUISettings.NumberOfBackups);
            Startup_LockProfileEdits();
            Startup_ListAllBackups();

            Startup_FixLegacySonglistBug();
            GuitarSpeak_ResetPresets();

            this.DoubleBuffered = true;
        }

        private async void MainForm_Shown(object sender, EventArgs e)
        {
            try
            {
                await GithubUpdater.CallGithubAPI();
                button_UpdateRSMods.Visible = GithubUpdater.IsUpdateAvailable();

                PrepTwitch_LoadSettings();
                Twitch_Setup();
            }
            catch
            {
                // If this fails, it doesn't really change anything meaningful
            }
        }

        private async void CheckForUpdates_UpdateRSMods(object sender, EventArgs e) => await GithubUpdater.DownloadAndRunInstaller();
    }
}
