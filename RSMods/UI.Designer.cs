namespace RSMods
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
                components.Dispose();
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.listBox_Songlist = new System.Windows.Forms.ListBox();
            this.listBox_Modlist_MODS = new System.Windows.Forms.ListBox();
            this.checkBox_ToggleLoft = new System.Windows.Forms.CheckBox();
            this.checkBox_SongTimer = new System.Windows.Forms.CheckBox();
            this.label_ToggleLoftKey = new System.Windows.Forms.Label();
            this.label_SongTimerKey = new System.Windows.Forms.Label();
            this.label_ReEnumerationKey = new System.Windows.Forms.Label();
            this.textBox_NewKeyAssignment_MODS = new System.Windows.Forms.TextBox();
            this.label_NewKeyAssignment_MODS = new System.Windows.Forms.Label();
            this.label_ChangeSonglistName = new System.Windows.Forms.Label();
            this.textBox_NewSonglistName = new System.Windows.Forms.TextBox();
            this.checkBox_ExtendedRange = new System.Windows.Forms.CheckBox();
            this.label_RainbowStringsKey = new System.Windows.Forms.Label();
            this.listBox_ExtendedRangeTunings = new System.Windows.Forms.ListBox();
            this.checkBox_ForceEnumeration = new System.Windows.Forms.CheckBox();
            this.checkBox_RemoveHeadstock = new System.Windows.Forms.CheckBox();
            this.checkBox_RemoveSkyline = new System.Windows.Forms.CheckBox();
            this.checkBox_GreenScreen = new System.Windows.Forms.CheckBox();
            this.checkBox_AutoLoadProfile = new System.Windows.Forms.CheckBox();
            this.checkBox_Fretless = new System.Windows.Forms.CheckBox();
            this.checkBox_RemoveInlays = new System.Windows.Forms.CheckBox();
            this.radio_LoftAlwaysOff = new System.Windows.Forms.RadioButton();
            this.radio_LoftOffHotkey = new System.Windows.Forms.RadioButton();
            this.radio_LoftOffInSong = new System.Windows.Forms.RadioButton();
            this.radio_ForceEnumerationManual = new System.Windows.Forms.RadioButton();
            this.radio_ForceEnumerationAutomatic = new System.Windows.Forms.RadioButton();
            this.groupBox_HowToEnumerate = new System.Windows.Forms.GroupBox();
            this.label_ForceEnumerationXMS = new System.Windows.Forms.Label();
            this.nUpDown_ForceEnumerationXMS = new System.Windows.Forms.NumericUpDown();
            this.groupBox_LoftOffWhen = new System.Windows.Forms.GroupBox();
            this.groupBox_ExtendedRangeWhen = new System.Windows.Forms.GroupBox();
            this.checkBox_ExtendedRangeDrop = new System.Windows.Forms.CheckBox();
            this.groupBox_EnabledMods = new System.Windows.Forms.GroupBox();
            this.checkBox_CustomHighway = new System.Windows.Forms.CheckBox();
            this.checkBox_RainbowNotes = new System.Windows.Forms.CheckBox();
            this.checkBox_BackupProfile = new System.Windows.Forms.CheckBox();
            this.checkBox_ShowCurrentNote = new System.Windows.Forms.CheckBox();
            this.checkBox_useMidiAutoTuning = new System.Windows.Forms.CheckBox();
            this.checkBox_RiffRepeaterSpeedAboveOneHundred = new System.Windows.Forms.CheckBox();
            this.checkBox_ScreenShotScores = new System.Windows.Forms.CheckBox();
            this.checkBox_ControlVolume = new System.Windows.Forms.CheckBox();
            this.checkBox_GuitarSpeak = new System.Windows.Forms.CheckBox();
            this.checkBox_RemoveLyrics = new System.Windows.Forms.CheckBox();
            this.checkBox_CustomColors = new System.Windows.Forms.CheckBox();
            this.checkBox_RemoveLaneMarkers = new System.Windows.Forms.CheckBox();
            this.checkBox_RainbowStrings = new System.Windows.Forms.CheckBox();
            this.groupBox_Songlist = new System.Windows.Forms.GroupBox();
            this.label_SonglistWarning = new System.Windows.Forms.Label();
            this.groupBox_Keybindings_MODS = new System.Windows.Forms.GroupBox();
            this.label_RainbowNotesKey = new System.Windows.Forms.Label();
            this.label_RRSpeedKey = new System.Windows.Forms.Label();
            this.label_RemoveLyricsKey = new System.Windows.Forms.Label();
            this.button_ClearSelectedKeybind_MODS = new System.Windows.Forms.Button();
            this.button_ResetModsToDefault = new System.Windows.Forms.Button();
            this.button_String0ColorButton = new System.Windows.Forms.Button();
            this.textBox_String0Color = new System.Windows.Forms.TextBox();
            this.textBox_String1Color = new System.Windows.Forms.TextBox();
            this.button_String1ColorButton = new System.Windows.Forms.Button();
            this.textBox_String2Color = new System.Windows.Forms.TextBox();
            this.button_String2ColorButton = new System.Windows.Forms.Button();
            this.textBox_String3Color = new System.Windows.Forms.TextBox();
            this.button_String3ColorButton = new System.Windows.Forms.Button();
            this.textBox_String4Color = new System.Windows.Forms.TextBox();
            this.button_String4ColorButton = new System.Windows.Forms.Button();
            this.textBox_String5Color = new System.Windows.Forms.TextBox();
            this.button_String5ColorButton = new System.Windows.Forms.Button();
            this.groupBox_StringColors = new System.Windows.Forms.GroupBox();
            this.radio_colorBlindERColors = new System.Windows.Forms.RadioButton();
            this.radio_DefaultStringColors = new System.Windows.Forms.RadioButton();
            this.groupBox_SetAndForget = new System.Windows.Forms.GroupBox();
            this.radio_GoneWailinTone = new System.Windows.Forms.RadioButton();
            this.radio_RainbowLaserTone = new System.Windows.Forms.RadioButton();
            this.radio_DucksReduxTone = new System.Windows.Forms.RadioButton();
            this.radio_HarmonicHeistTone = new System.Windows.Forms.RadioButton();
            this.radio_HurtlinHurdlesTone = new System.Windows.Forms.RadioButton();
            this.radio_NinjaSlideNTone = new System.Windows.Forms.RadioButton();
            this.radio_ScaleRacerTone = new System.Windows.Forms.RadioButton();
            this.radio_StringsSkipSaloonTone = new System.Windows.Forms.RadioButton();
            this.radio_ScaleWarriorsTone = new System.Windows.Forms.RadioButton();
            this.radio_TempleOfBendsTone = new System.Windows.Forms.RadioButton();
            this.button_AssignNewGuitarArcadeTone = new System.Windows.Forms.Button();
            this.label_SetAndForgetGuitarArcade = new System.Windows.Forms.Label();
            this.button_AddDCInput = new System.Windows.Forms.Button();
            this.label_ChangeTonesHeader = new System.Windows.Forms.Label();
            this.label_HorizontalRuleSetAndForget = new System.Windows.Forms.Label();
            this.label_HorizontalRulerGuitarArcade = new System.Windows.Forms.Label();
            this.button_ImportExistingSettings = new System.Windows.Forms.Button();
            this.label_CustomTuningHighEStringLetter = new System.Windows.Forms.Label();
            this.label_CustomTuningBStringLetter = new System.Windows.Forms.Label();
            this.label_CustomTuningGStringLetter = new System.Windows.Forms.Label();
            this.label_CustomTuningDStringLetter = new System.Windows.Forms.Label();
            this.label_CustomTuningAStringLetter = new System.Windows.Forms.Label();
            this.label_CustomTuningLowEStringLetter = new System.Windows.Forms.Label();
            this.radio_DefaultBassTone = new System.Windows.Forms.RadioButton();
            this.radio_DefaultLeadTone = new System.Windows.Forms.RadioButton();
            this.radio_DefaultRhythmTone = new System.Windows.Forms.RadioButton();
            this.button_LoadTones = new System.Windows.Forms.Button();
            this.listBox_ProfileTones = new System.Windows.Forms.ListBox();
            this.button_AssignNewDefaultTone = new System.Windows.Forms.Button();
            this.button_RemoveTemp = new System.Windows.Forms.Button();
            this.button_AddExitGame = new System.Windows.Forms.Button();
            this.button_AddFastLoad = new System.Windows.Forms.Button();
            this.button_CleanUpUnpackedCache = new System.Windows.Forms.Button();
            this.button_RestoreCacheBackup = new System.Windows.Forms.Button();
            this.button_AddCustomTunings = new System.Windows.Forms.Button();
            this.button_SaveTuningChanges = new System.Windows.Forms.Button();
            this.label_CustomStringTunings = new System.Windows.Forms.Label();
            this.nUpDown_String0 = new System.Windows.Forms.NumericUpDown();
            this.label_UIIndex = new System.Windows.Forms.Label();
            this.nUpDown_UIIndex = new System.Windows.Forms.NumericUpDown();
            this.label_UITuningName = new System.Windows.Forms.Label();
            this.label_InternalTuningName = new System.Windows.Forms.Label();
            this.nUpDown_String1 = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_String5 = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_String4 = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_String3 = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_String2 = new System.Windows.Forms.NumericUpDown();
            this.textBox_UIName = new System.Windows.Forms.TextBox();
            this.textBox_InternalTuningName = new System.Windows.Forms.TextBox();
            this.button_AddTuning = new System.Windows.Forms.Button();
            this.button_RemoveTuning = new System.Windows.Forms.Button();
            this.listBox_Tunings = new System.Windows.Forms.ListBox();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.groupBox_ToggleSkylineWhen = new System.Windows.Forms.GroupBox();
            this.radio_SkylineOffInSong = new System.Windows.Forms.RadioButton();
            this.radio_SkylineAlwaysOff = new System.Windows.Forms.RadioButton();
            this.groupBox_ToggleLyricsOffWhen = new System.Windows.Forms.GroupBox();
            this.radio_LyricsOffHotkey = new System.Windows.Forms.RadioButton();
            this.radio_LyricsAlwaysOff = new System.Windows.Forms.RadioButton();
            this.groupBox_GuitarSpeak = new System.Windows.Forms.GroupBox();
            this.button_GuitarSpeak_DeleteSavedValue = new System.Windows.Forms.Button();
            this.button_GuitarSpeakHelp = new System.Windows.Forms.Button();
            this.label_GuitarSpeakSaved = new System.Windows.Forms.Label();
            this.listBox_GuitarSpeakSaved = new System.Windows.Forms.ListBox();
            this.checkbox_GuitarSpeakWhileTuning = new System.Windows.Forms.CheckBox();
            this.button_GuitarSpeakSave = new System.Windows.Forms.Button();
            this.label_GuitarSpeakKeypress = new System.Windows.Forms.Label();
            this.label_GuitarSpeakOctave = new System.Windows.Forms.Label();
            this.label_GuitarSpeakNote = new System.Windows.Forms.Label();
            this.listBox_GuitarSpeakKeypress = new System.Windows.Forms.ListBox();
            this.listBox_GuitarSpeakOctave = new System.Windows.Forms.ListBox();
            this.listBox_GuitarSpeakNote = new System.Windows.Forms.ListBox();
            this.label_Credits = new System.Windows.Forms.Label();
            this.groupBox_ToggleHeadstockOffWhen = new System.Windows.Forms.GroupBox();
            this.radio_HeadstockOffInSong = new System.Windows.Forms.RadioButton();
            this.radio_HeadstockAlwaysOff = new System.Windows.Forms.RadioButton();
            this.TabController = new System.Windows.Forms.TabControl();
            this.tab_Songlists = new System.Windows.Forms.TabPage();
            this.tab_Keybindings = new System.Windows.Forms.TabPage();
            this.groupBox_Keybindings_AUDIO = new System.Windows.Forms.GroupBox();
            this.label_ChangeSelectedVolumeKey = new System.Windows.Forms.Label();
            this.label_SFXVolumeKey = new System.Windows.Forms.Label();
            this.label_VoiceOverVolumeKey = new System.Windows.Forms.Label();
            this.label_MicrophoneVolumeKey = new System.Windows.Forms.Label();
            this.label_Player2VolumeKey = new System.Windows.Forms.Label();
            this.label_Player1VolumeKey = new System.Windows.Forms.Label();
            this.label_SongVolumeKey = new System.Windows.Forms.Label();
            this.label_MasterVolumeKey = new System.Windows.Forms.Label();
            this.button_ClearSelectedKeybind_AUDIO = new System.Windows.Forms.Button();
            this.label_NewAssignmentAUDIO = new System.Windows.Forms.Label();
            this.textBox_NewKeyAssignment_AUDIO = new System.Windows.Forms.TextBox();
            this.listBox_Modlist_AUDIO = new System.Windows.Forms.ListBox();
            this.tab_ModToggles = new System.Windows.Forms.TabPage();
            this.tab_SetAndForget = new System.Windows.Forms.TabPage();
            this.tab_ModSettings = new System.Windows.Forms.TabPage();
            this.TabController_ModSettings = new System.Windows.Forms.TabControl();
            this.tabPage_ModSettings_ER = new System.Windows.Forms.TabPage();
            this.tabPage_ModSettings_Automation = new System.Windows.Forms.TabPage();
            this.groupBox_Backups = new System.Windows.Forms.GroupBox();
            this.checkBox_UnlimitedBackups = new System.Windows.Forms.CheckBox();
            this.nUpDown_NumberOfBackups = new System.Windows.Forms.NumericUpDown();
            this.groupBox_AutoLoadProfiles = new System.Windows.Forms.GroupBox();
            this.button_AutoLoadProfile_ClearSelection = new System.Windows.Forms.Button();
            this.listBox_AutoLoadProfiles = new System.Windows.Forms.ListBox();
            this.groupBox_MidiAutoTuneDevice = new System.Windows.Forms.GroupBox();
            this.checkBox_WhammyChordsMode = new System.Windows.Forms.CheckBox();
            this.radio_WhammyORBass = new System.Windows.Forms.RadioButton();
            this.radio_WhammyDT = new System.Windows.Forms.RadioButton();
            this.label_MidiWhatTuningPedalDoYouUse = new System.Windows.Forms.Label();
            this.label_SelectedMidiDevice = new System.Windows.Forms.Label();
            this.listBox_ListMidiDevices = new System.Windows.Forms.ListBox();
            this.tabPage_ModSettings_Misc = new System.Windows.Forms.TabPage();
            this.groupBox_CustomHighway = new System.Windows.Forms.GroupBox();
            this.button_ResetNotewayColors = new System.Windows.Forms.Button();
            this.textBox_ShowFretNumber = new System.Windows.Forms.TextBox();
            this.textBox_ShowNotewayGutter = new System.Windows.Forms.TextBox();
            this.textBox_ShowUnNumberedFrets = new System.Windows.Forms.TextBox();
            this.textBox_ShowNumberedFrets = new System.Windows.Forms.TextBox();
            this.button_ChangeFretNumber = new System.Windows.Forms.Button();
            this.button_ChangeNotewayGutter = new System.Windows.Forms.Button();
            this.button_ChangeUnNumberedFrets = new System.Windows.Forms.Button();
            this.button_ChangeNumberedFrets = new System.Windows.Forms.Button();
            this.groupBox_OnScreenFont = new System.Windows.Forms.GroupBox();
            this.label_FontTestNumbers = new System.Windows.Forms.Label();
            this.label_FontTestlowercase = new System.Windows.Forms.Label();
            this.label_FontTestCAPITALS = new System.Windows.Forms.Label();
            this.listBox_AvailableFonts = new System.Windows.Forms.ListBox();
            this.groupBox_RRSpeed = new System.Windows.Forms.GroupBox();
            this.nUpDown_RiffRepeaterSpeed = new System.Windows.Forms.NumericUpDown();
            this.groupBox_ControlVolumeIncrement = new System.Windows.Forms.GroupBox();
            this.nUpDown_VolumeInterval = new System.Windows.Forms.NumericUpDown();
            this.tab_RSASIO = new System.Windows.Forms.TabPage();
            this.groupBox_ASIO_InputMic = new System.Windows.Forms.GroupBox();
            this.button_ASIO_InputMic_ClearSelection = new System.Windows.Forms.Button();
            this.checkBox_ASIO_InputMic_Disabled = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_InputMic_ControlEndpointVolume = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_InputMic_ControlMasterVolume = new System.Windows.Forms.CheckBox();
            this.label_ASIO_InputMic_MaxVolume = new System.Windows.Forms.Label();
            this.nUpDown_ASIO_InputMic_MaxVolume = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_ASIO_InputMic_Channel = new System.Windows.Forms.NumericUpDown();
            this.label_ASIO_InputMic_Channel = new System.Windows.Forms.Label();
            this.listBox_AvailableASIODevices_InputMic = new System.Windows.Forms.ListBox();
            this.label_ASIO_Credits = new System.Windows.Forms.Label();
            this.groupBox_ASIO_Output = new System.Windows.Forms.GroupBox();
            this.button_ASIO_Output_ClearSelection = new System.Windows.Forms.Button();
            this.checkBox_ASIO_Output_Disabled = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_Output_ControlEndpointVolume = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_Output_ControlMasterVolume = new System.Windows.Forms.CheckBox();
            this.label_ASIO_Output_MaxVolume = new System.Windows.Forms.Label();
            this.nUpDown_ASIO_Output_MaxVolume = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_ASIO_Output_BaseChannel = new System.Windows.Forms.NumericUpDown();
            this.label_ASIO_Output_BaseChannel = new System.Windows.Forms.Label();
            this.listBox_AvailableASIODevices_Output = new System.Windows.Forms.ListBox();
            this.groupBox_ASIO_Input1 = new System.Windows.Forms.GroupBox();
            this.button_ASIO_Input1_ClearSelection = new System.Windows.Forms.Button();
            this.checkBox_ASIO_Input1_Disabled = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_Input1_ControlEndpointVolume = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_Input1_ControlMasterVolume = new System.Windows.Forms.CheckBox();
            this.label_ASIO_Input1_MaxVolume = new System.Windows.Forms.Label();
            this.nUpDown_ASIO_Input1_MaxVolume = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_ASIO_Input1_Channel = new System.Windows.Forms.NumericUpDown();
            this.label_ASIO_Input1_Channel = new System.Windows.Forms.Label();
            this.listBox_AvailableASIODevices_Input1 = new System.Windows.Forms.ListBox();
            this.groupBox_ASIO_Input0 = new System.Windows.Forms.GroupBox();
            this.button_ASIO_Input0_ClearSelection = new System.Windows.Forms.Button();
            this.checkBox_ASIO_Input0_Disabled = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_Input0_ControlEndpointVolume = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_Input0_ControlMasterVolume = new System.Windows.Forms.CheckBox();
            this.label_ASIO_Input0_MaxVolume = new System.Windows.Forms.Label();
            this.nUpDown_ASIO_Input0_MaxVolume = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_ASIO_Input0_Channel = new System.Windows.Forms.NumericUpDown();
            this.label_ASIO_Input0_Channel = new System.Windows.Forms.Label();
            this.listBox_AvailableASIODevices_Input0 = new System.Windows.Forms.ListBox();
            this.groupBox_ASIO_Config = new System.Windows.Forms.GroupBox();
            this.checkBox_ASIO_ASIO = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_WASAPI_Input = new System.Windows.Forms.CheckBox();
            this.checkBox_ASIO_WASAPI_Output = new System.Windows.Forms.CheckBox();
            this.groupBox_ASIO_BufferSize = new System.Windows.Forms.GroupBox();
            this.label_ASIO_CustomBufferSize = new System.Windows.Forms.Label();
            this.nUpDown_ASIO_CustomBufferSize = new System.Windows.Forms.NumericUpDown();
            this.radio_ASIO_BufferSize_Custom = new System.Windows.Forms.RadioButton();
            this.radio_ASIO_BufferSize_Host = new System.Windows.Forms.RadioButton();
            this.radio_ASIO_BufferSize_Driver = new System.Windows.Forms.RadioButton();
            this.tab_Rocksmith = new System.Windows.Forms.TabPage();
            this.label_Rocksmith_Thanks = new System.Windows.Forms.Label();
            this.groupBox_Rocksmith_AudioSettings = new System.Windows.Forms.GroupBox();
            this.checkBox_Rocksmith_LowLatencyMode = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_RTCOnly = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_DumpAudioLog = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_ForceDirextXSink = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_ForceWDM = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_ExclusiveMode = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_EnableMicrophone = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize = new System.Windows.Forms.CheckBox();
            this.label_Rocksmith_MaxOutputBuffer = new System.Windows.Forms.Label();
            this.label_Rocksmith_LatencyBuffer = new System.Windows.Forms.Label();
            this.nUpDown_Rocksmith_MaxOutputBuffer = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_Rocksmith_LatencyBuffer = new System.Windows.Forms.NumericUpDown();
            this.groupBox_Rocksmith_VisualSettings = new System.Windows.Forms.GroupBox();
            this.checkBox_Rocksmith_EnableRenderRes = new System.Windows.Forms.CheckBox();
            this.groupBox_Rocksmith_Network = new System.Windows.Forms.GroupBox();
            this.checkBox_Rocksmith_UseProxy = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_MSAASamples = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_PostEffects = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_PerPixelLighting = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_DepthOfField = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_HighResScope = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_Shadows = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_GamepadUI = new System.Windows.Forms.CheckBox();
            this.checkBox_Rocksmith_DisableBrowser = new System.Windows.Forms.CheckBox();
            this.groupBox_Rocksmith_Fullscreen = new System.Windows.Forms.GroupBox();
            this.radio_Rocksmith_ExclusiveFullScreen = new System.Windows.Forms.RadioButton();
            this.radio_Rocksmith_NonExclusiveFullScreen = new System.Windows.Forms.RadioButton();
            this.radio_Rocksmith_Windowed = new System.Windows.Forms.RadioButton();
            this.groupBox_Rocksmith_VisualQuality = new System.Windows.Forms.GroupBox();
            this.radio_Rocksmith_HighQuality = new System.Windows.Forms.RadioButton();
            this.radio_Rocksmith_MediumQuality = new System.Windows.Forms.RadioButton();
            this.radio_Rocksmith_LowQuality = new System.Windows.Forms.RadioButton();
            this.groupBox_Rocksmith_Resolutions = new System.Windows.Forms.GroupBox();
            this.label_Rocksmith_RenderHeight = new System.Windows.Forms.Label();
            this.label_Rocksmith_RenderWidth = new System.Windows.Forms.Label();
            this.label_Rocksmith_ScreenHeight = new System.Windows.Forms.Label();
            this.label_Rocksmith_ScreenWidth = new System.Windows.Forms.Label();
            this.nUpDown_Rocksmith_RenderHeight = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_Rocksmith_RenderWidth = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_Rocksmith_ScreenHeight = new System.Windows.Forms.NumericUpDown();
            this.nUpDown_Rocksmith_ScreenWidth = new System.Windows.Forms.NumericUpDown();
            this.tab_Twitch = new System.Windows.Forms.TabPage();
            this.checkBox_RevealTwitchAuthToken = new System.Windows.Forms.CheckBox();
            this.button_SaveLogToFile = new System.Windows.Forms.Button();
            this.checkBox_TwitchForceReauth = new System.Windows.Forms.CheckBox();
            this.button_TestTwitchReward = new System.Windows.Forms.Button();
            this.button_SolidNoteColorRandom = new System.Windows.Forms.Button();
            this.textBox_SolidNoteColorPicker = new System.Windows.Forms.TextBox();
            this.button_SolidNoteColorPicker = new System.Windows.Forms.Button();
            this.button_RemoveReward = new System.Windows.Forms.Button();
            this.button_AddSelectedReward = new System.Windows.Forms.Button();
            this.dgv_EnabledRewards = new System.Windows.Forms.DataGridView();
            this.colEnabledRewardsEnabled = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.colEnabledRewardsName = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colEnabledRewardsLength = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colEnabledRewardsAmount = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colEnabledRewardsType = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colEnabledRewardsID = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.label_TwitchLog = new System.Windows.Forms.Label();
            this.textBox_TwitchLog = new System.Windows.Forms.TextBox();
            this.label_IsListeningToEvents = new System.Windows.Forms.Label();
            this.dgv_DefaultRewards = new System.Windows.Forms.DataGridView();
            this.colDefaultRewardsName = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colDefaultRewardsDescription = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.label_HorizontalLineTwitch = new System.Windows.Forms.Label();
            this.label_TwitchAccessTokenVal = new System.Windows.Forms.Label();
            this.label_TwitchChannelIDVal = new System.Windows.Forms.Label();
            this.label_TwitchUsernameVal = new System.Windows.Forms.Label();
            this.label_TwitchAccessToken = new System.Windows.Forms.Label();
            this.label_TwitchChannelID = new System.Windows.Forms.Label();
            this.label_TwitchUsername = new System.Windows.Forms.Label();
            this.label_AuthorizedAs = new System.Windows.Forms.Label();
            this.label_TwitchAuthorized = new System.Windows.Forms.Label();
            this.button_TwitchReAuthorize = new System.Windows.Forms.Button();
            this.tab_GUISettings = new System.Windows.Forms.TabPage();
            this.groupBox_ChangeTheme = new System.Windows.Forms.GroupBox();
            this.button_ApplyThemeColors = new System.Windows.Forms.Button();
            this.textBox_ChangeTextColor = new System.Windows.Forms.TextBox();
            this.textBox_ChangeBackgroundColor = new System.Windows.Forms.TextBox();
            this.button_ChangeTextColor = new System.Windows.Forms.Button();
            this.button_ChangeBackgroundColor = new System.Windows.Forms.Button();
            this.checkBox_ChangeTheme = new System.Windows.Forms.CheckBox();
            this.timerValidateTwitch = new System.Windows.Forms.Timer(this.components);
            this.label_SettingsSaved = new System.Windows.Forms.Label();
            this.groupBox_HowToEnumerate.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ForceEnumerationXMS)).BeginInit();
            this.groupBox_LoftOffWhen.SuspendLayout();
            this.groupBox_ExtendedRangeWhen.SuspendLayout();
            this.groupBox_EnabledMods.SuspendLayout();
            this.groupBox_Songlist.SuspendLayout();
            this.groupBox_Keybindings_MODS.SuspendLayout();
            this.groupBox_StringColors.SuspendLayout();
            this.groupBox_SetAndForget.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String0)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_UIIndex)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String5)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String4)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String3)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String2)).BeginInit();
            this.groupBox_ToggleSkylineWhen.SuspendLayout();
            this.groupBox_ToggleLyricsOffWhen.SuspendLayout();
            this.groupBox_GuitarSpeak.SuspendLayout();
            this.groupBox_ToggleHeadstockOffWhen.SuspendLayout();
            this.TabController.SuspendLayout();
            this.tab_Songlists.SuspendLayout();
            this.tab_Keybindings.SuspendLayout();
            this.groupBox_Keybindings_AUDIO.SuspendLayout();
            this.tab_ModToggles.SuspendLayout();
            this.tab_SetAndForget.SuspendLayout();
            this.tab_ModSettings.SuspendLayout();
            this.TabController_ModSettings.SuspendLayout();
            this.tabPage_ModSettings_ER.SuspendLayout();
            this.tabPage_ModSettings_Automation.SuspendLayout();
            this.groupBox_Backups.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_NumberOfBackups)).BeginInit();
            this.groupBox_AutoLoadProfiles.SuspendLayout();
            this.groupBox_MidiAutoTuneDevice.SuspendLayout();
            this.tabPage_ModSettings_Misc.SuspendLayout();
            this.groupBox_CustomHighway.SuspendLayout();
            this.groupBox_OnScreenFont.SuspendLayout();
            this.groupBox_RRSpeed.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_RiffRepeaterSpeed)).BeginInit();
            this.groupBox_ControlVolumeIncrement.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_VolumeInterval)).BeginInit();
            this.tab_RSASIO.SuspendLayout();
            this.groupBox_ASIO_InputMic.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_InputMic_MaxVolume)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_InputMic_Channel)).BeginInit();
            this.groupBox_ASIO_Output.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Output_MaxVolume)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Output_BaseChannel)).BeginInit();
            this.groupBox_ASIO_Input1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Input1_MaxVolume)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Input1_Channel)).BeginInit();
            this.groupBox_ASIO_Input0.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Input0_MaxVolume)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Input0_Channel)).BeginInit();
            this.groupBox_ASIO_Config.SuspendLayout();
            this.groupBox_ASIO_BufferSize.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_CustomBufferSize)).BeginInit();
            this.tab_Rocksmith.SuspendLayout();
            this.groupBox_Rocksmith_AudioSettings.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_MaxOutputBuffer)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_LatencyBuffer)).BeginInit();
            this.groupBox_Rocksmith_VisualSettings.SuspendLayout();
            this.groupBox_Rocksmith_Network.SuspendLayout();
            this.groupBox_Rocksmith_Fullscreen.SuspendLayout();
            this.groupBox_Rocksmith_VisualQuality.SuspendLayout();
            this.groupBox_Rocksmith_Resolutions.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_RenderHeight)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_RenderWidth)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_ScreenHeight)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_ScreenWidth)).BeginInit();
            this.tab_Twitch.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgv_EnabledRewards)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgv_DefaultRewards)).BeginInit();
            this.tab_GUISettings.SuspendLayout();
            this.groupBox_ChangeTheme.SuspendLayout();
            this.SuspendLayout();
            // 
            // listBox_Songlist
            // 
            this.listBox_Songlist.FormattingEnabled = true;
            this.listBox_Songlist.Location = new System.Drawing.Point(21, 26);
            this.listBox_Songlist.Name = "listBox_Songlist";
            this.listBox_Songlist.Size = new System.Drawing.Size(185, 147);
            this.listBox_Songlist.TabIndex = 2;
            this.listBox_Songlist.SelectedIndexChanged += new System.EventHandler(this.Fill_Songlist_Name);
            // 
            // listBox_Modlist_MODS
            // 
            this.listBox_Modlist_MODS.FormattingEnabled = true;
            this.listBox_Modlist_MODS.Location = new System.Drawing.Point(22, 29);
            this.listBox_Modlist_MODS.Name = "listBox_Modlist_MODS";
            this.listBox_Modlist_MODS.Size = new System.Drawing.Size(185, 134);
            this.listBox_Modlist_MODS.TabIndex = 5;
            this.listBox_Modlist_MODS.SelectedIndexChanged += new System.EventHandler(this.LoadPreviousKeyAssignment);
            // 
            // checkBox_ToggleLoft
            // 
            this.checkBox_ToggleLoft.AutoSize = true;
            this.checkBox_ToggleLoft.Location = new System.Drawing.Point(11, 15);
            this.checkBox_ToggleLoft.Name = "checkBox_ToggleLoft";
            this.checkBox_ToggleLoft.Size = new System.Drawing.Size(80, 17);
            this.checkBox_ToggleLoft.TabIndex = 6;
            this.checkBox_ToggleLoft.Text = "Toggle Loft";
            this.checkBox_ToggleLoft.UseVisualStyleBackColor = true;
            this.checkBox_ToggleLoft.CheckedChanged += new System.EventHandler(this.Save_ToggleLoft);
            this.checkBox_ToggleLoft.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_ToggleLoft.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_SongTimer
            // 
            this.checkBox_SongTimer.AutoSize = true;
            this.checkBox_SongTimer.Location = new System.Drawing.Point(131, 38);
            this.checkBox_SongTimer.Name = "checkBox_SongTimer";
            this.checkBox_SongTimer.Size = new System.Drawing.Size(110, 17);
            this.checkBox_SongTimer.TabIndex = 10;
            this.checkBox_SongTimer.Text = "Show Song Timer";
            this.checkBox_SongTimer.UseVisualStyleBackColor = true;
            this.checkBox_SongTimer.CheckedChanged += new System.EventHandler(this.Save_SongTimer);
            this.checkBox_SongTimer.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_SongTimer.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_ToggleLoftKey
            // 
            this.label_ToggleLoftKey.AutoSize = true;
            this.label_ToggleLoftKey.Location = new System.Drawing.Point(464, 14);
            this.label_ToggleLoftKey.Name = "label_ToggleLoftKey";
            this.label_ToggleLoftKey.Size = new System.Drawing.Size(67, 13);
            this.label_ToggleLoftKey.TabIndex = 14;
            this.label_ToggleLoftKey.Text = "Toggle Loft: ";
            // 
            // label_SongTimerKey
            // 
            this.label_SongTimerKey.AutoSize = true;
            this.label_SongTimerKey.Location = new System.Drawing.Point(464, 35);
            this.label_SongTimerKey.Name = "label_SongTimerKey";
            this.label_SongTimerKey.Size = new System.Drawing.Size(67, 13);
            this.label_SongTimerKey.TabIndex = 17;
            this.label_SongTimerKey.Text = "Song Timer: ";
            // 
            // label_ReEnumerationKey
            // 
            this.label_ReEnumerationKey.AutoSize = true;
            this.label_ReEnumerationKey.Location = new System.Drawing.Point(464, 57);
            this.label_ReEnumerationKey.Name = "label_ReEnumerationKey";
            this.label_ReEnumerationKey.Size = new System.Drawing.Size(72, 13);
            this.label_ReEnumerationKey.TabIndex = 18;
            this.label_ReEnumerationKey.Text = "Enumeration: ";
            // 
            // textBox_NewKeyAssignment_MODS
            // 
            this.textBox_NewKeyAssignment_MODS.Location = new System.Drawing.Point(222, 93);
            this.textBox_NewKeyAssignment_MODS.MaxLength = 1;
            this.textBox_NewKeyAssignment_MODS.Name = "textBox_NewKeyAssignment_MODS";
            this.textBox_NewKeyAssignment_MODS.Size = new System.Drawing.Size(188, 20);
            this.textBox_NewKeyAssignment_MODS.TabIndex = 20;
            this.textBox_NewKeyAssignment_MODS.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.textBox_NewKeyAssignment_MODS.KeyDown += new System.Windows.Forms.KeyEventHandler(this.CheckKeyPressesDown);
            this.textBox_NewKeyAssignment_MODS.KeyUp += new System.Windows.Forms.KeyEventHandler(this.CheckKeyPressesUp);
            this.textBox_NewKeyAssignment_MODS.MouseDown += new System.Windows.Forms.MouseEventHandler(this.CheckMouseInput);
            // 
            // label_NewKeyAssignment_MODS
            // 
            this.label_NewKeyAssignment_MODS.AutoSize = true;
            this.label_NewKeyAssignment_MODS.Location = new System.Drawing.Point(219, 59);
            this.label_NewKeyAssignment_MODS.Name = "label_NewKeyAssignment_MODS";
            this.label_NewKeyAssignment_MODS.Size = new System.Drawing.Size(196, 13);
            this.label_NewKeyAssignment_MODS.TabIndex = 21;
            this.label_NewKeyAssignment_MODS.Text = "New Assignment (Press Enter To Save):";
            // 
            // label_ChangeSonglistName
            // 
            this.label_ChangeSonglistName.AutoSize = true;
            this.label_ChangeSonglistName.Location = new System.Drawing.Point(218, 44);
            this.label_ChangeSonglistName.Name = "label_ChangeSonglistName";
            this.label_ChangeSonglistName.Size = new System.Drawing.Size(225, 13);
            this.label_ChangeSonglistName.TabIndex = 22;
            this.label_ChangeSonglistName.Text = "Change Songlist Name (Press Enter To Save):";
            // 
            // textBox_NewSonglistName
            // 
            this.textBox_NewSonglistName.Location = new System.Drawing.Point(221, 83);
            this.textBox_NewSonglistName.Name = "textBox_NewSonglistName";
            this.textBox_NewSonglistName.Size = new System.Drawing.Size(222, 20);
            this.textBox_NewSonglistName.TabIndex = 23;
            this.textBox_NewSonglistName.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.textBox_NewSonglistName.KeyDown += new System.Windows.Forms.KeyEventHandler(this.CheckKeyPressesDown);
            // 
            // checkBox_ExtendedRange
            // 
            this.checkBox_ExtendedRange.AutoSize = true;
            this.checkBox_ExtendedRange.Location = new System.Drawing.Point(131, 107);
            this.checkBox_ExtendedRange.Name = "checkBox_ExtendedRange";
            this.checkBox_ExtendedRange.Size = new System.Drawing.Size(146, 17);
            this.checkBox_ExtendedRange.TabIndex = 27;
            this.checkBox_ExtendedRange.Text = "Extended Range Support";
            this.checkBox_ExtendedRange.UseVisualStyleBackColor = true;
            this.checkBox_ExtendedRange.CheckedChanged += new System.EventHandler(this.Save_ExtendedRange);
            this.checkBox_ExtendedRange.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_ExtendedRange.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_RainbowStringsKey
            // 
            this.label_RainbowStringsKey.AutoSize = true;
            this.label_RainbowStringsKey.Location = new System.Drawing.Point(464, 79);
            this.label_RainbowStringsKey.Name = "label_RainbowStringsKey";
            this.label_RainbowStringsKey.Size = new System.Drawing.Size(90, 13);
            this.label_RainbowStringsKey.TabIndex = 28;
            this.label_RainbowStringsKey.Text = "Rainbow Strings: ";
            // 
            // listBox_ExtendedRangeTunings
            // 
            this.listBox_ExtendedRangeTunings.FormattingEnabled = true;
            this.listBox_ExtendedRangeTunings.Items.AddRange(new object[] {
            "D",
            "C#",
            "C",
            "B",
            "A#",
            "A",
            "G#",
            "G",
            "F#",
            "F",
            "Octave Down"});
            this.listBox_ExtendedRangeTunings.Location = new System.Drawing.Point(60, 27);
            this.listBox_ExtendedRangeTunings.Name = "listBox_ExtendedRangeTunings";
            this.listBox_ExtendedRangeTunings.Size = new System.Drawing.Size(79, 147);
            this.listBox_ExtendedRangeTunings.TabIndex = 29;
            this.listBox_ExtendedRangeTunings.Visible = false;
            this.listBox_ExtendedRangeTunings.SelectedIndexChanged += new System.EventHandler(this.Save_ExtendedRangeTuningAt);
            // 
            // checkBox_ForceEnumeration
            // 
            this.checkBox_ForceEnumeration.AutoSize = true;
            this.checkBox_ForceEnumeration.Location = new System.Drawing.Point(11, 130);
            this.checkBox_ForceEnumeration.Name = "checkBox_ForceEnumeration";
            this.checkBox_ForceEnumeration.Size = new System.Drawing.Size(115, 17);
            this.checkBox_ForceEnumeration.TabIndex = 31;
            this.checkBox_ForceEnumeration.Text = "Force Enumeration";
            this.checkBox_ForceEnumeration.UseVisualStyleBackColor = true;
            this.checkBox_ForceEnumeration.CheckedChanged += new System.EventHandler(this.Save_ForceEnumeration);
            this.checkBox_ForceEnumeration.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_ForceEnumeration.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_RemoveHeadstock
            // 
            this.checkBox_RemoveHeadstock.AutoSize = true;
            this.checkBox_RemoveHeadstock.Location = new System.Drawing.Point(11, 84);
            this.checkBox_RemoveHeadstock.Name = "checkBox_RemoveHeadstock";
            this.checkBox_RemoveHeadstock.Size = new System.Drawing.Size(121, 17);
            this.checkBox_RemoveHeadstock.TabIndex = 34;
            this.checkBox_RemoveHeadstock.Text = "Remove Headstock";
            this.checkBox_RemoveHeadstock.UseVisualStyleBackColor = true;
            this.checkBox_RemoveHeadstock.CheckedChanged += new System.EventHandler(this.Save_RemoveHeadstockCheckbox);
            this.checkBox_RemoveHeadstock.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_RemoveHeadstock.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_RemoveSkyline
            // 
            this.checkBox_RemoveSkyline.AutoSize = true;
            this.checkBox_RemoveSkyline.Location = new System.Drawing.Point(131, 153);
            this.checkBox_RemoveSkyline.Name = "checkBox_RemoveSkyline";
            this.checkBox_RemoveSkyline.Size = new System.Drawing.Size(103, 17);
            this.checkBox_RemoveSkyline.TabIndex = 36;
            this.checkBox_RemoveSkyline.Text = "Remove Skyline";
            this.checkBox_RemoveSkyline.UseVisualStyleBackColor = true;
            this.checkBox_RemoveSkyline.CheckedChanged += new System.EventHandler(this.Save_RemoveSkyline);
            this.checkBox_RemoveSkyline.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_RemoveSkyline.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_GreenScreen
            // 
            this.checkBox_GreenScreen.AutoSize = true;
            this.checkBox_GreenScreen.Location = new System.Drawing.Point(11, 107);
            this.checkBox_GreenScreen.Name = "checkBox_GreenScreen";
            this.checkBox_GreenScreen.Size = new System.Drawing.Size(111, 17);
            this.checkBox_GreenScreen.TabIndex = 37;
            this.checkBox_GreenScreen.Text = "Greenscreen Wall";
            this.checkBox_GreenScreen.UseVisualStyleBackColor = true;
            this.checkBox_GreenScreen.CheckedChanged += new System.EventHandler(this.Save_GreenScreenWall);
            this.checkBox_GreenScreen.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_GreenScreen.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_AutoLoadProfile
            // 
            this.checkBox_AutoLoadProfile.AutoSize = true;
            this.checkBox_AutoLoadProfile.Location = new System.Drawing.Point(131, 176);
            this.checkBox_AutoLoadProfile.Name = "checkBox_AutoLoadProfile";
            this.checkBox_AutoLoadProfile.Size = new System.Drawing.Size(151, 17);
            this.checkBox_AutoLoadProfile.TabIndex = 38;
            this.checkBox_AutoLoadProfile.Text = "Autoload Last Used Profile";
            this.checkBox_AutoLoadProfile.UseVisualStyleBackColor = true;
            this.checkBox_AutoLoadProfile.CheckedChanged += new System.EventHandler(this.Save_AutoLoadLastProfile);
            this.checkBox_AutoLoadProfile.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_AutoLoadProfile.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Fretless
            // 
            this.checkBox_Fretless.AutoSize = true;
            this.checkBox_Fretless.Location = new System.Drawing.Point(11, 153);
            this.checkBox_Fretless.Name = "checkBox_Fretless";
            this.checkBox_Fretless.Size = new System.Drawing.Size(92, 17);
            this.checkBox_Fretless.TabIndex = 41;
            this.checkBox_Fretless.Text = "Fretless Mode";
            this.checkBox_Fretless.UseVisualStyleBackColor = true;
            this.checkBox_Fretless.CheckedChanged += new System.EventHandler(this.Save_Fretless);
            this.checkBox_Fretless.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_Fretless.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_RemoveInlays
            // 
            this.checkBox_RemoveInlays.AutoSize = true;
            this.checkBox_RemoveInlays.Location = new System.Drawing.Point(11, 38);
            this.checkBox_RemoveInlays.Name = "checkBox_RemoveInlays";
            this.checkBox_RemoveInlays.Size = new System.Drawing.Size(96, 17);
            this.checkBox_RemoveInlays.TabIndex = 42;
            this.checkBox_RemoveInlays.Text = "Remove Inlays";
            this.checkBox_RemoveInlays.UseVisualStyleBackColor = true;
            this.checkBox_RemoveInlays.CheckedChanged += new System.EventHandler(this.Save_RemoveInlays);
            this.checkBox_RemoveInlays.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_RemoveInlays.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_LoftAlwaysOff
            // 
            this.radio_LoftAlwaysOff.AutoSize = true;
            this.radio_LoftAlwaysOff.Location = new System.Drawing.Point(11, 19);
            this.radio_LoftAlwaysOff.Name = "radio_LoftAlwaysOff";
            this.radio_LoftAlwaysOff.Size = new System.Drawing.Size(96, 17);
            this.radio_LoftAlwaysOff.TabIndex = 44;
            this.radio_LoftAlwaysOff.TabStop = true;
            this.radio_LoftAlwaysOff.Text = "Loft Always Off";
            this.radio_LoftAlwaysOff.UseVisualStyleBackColor = true;
            this.radio_LoftAlwaysOff.Visible = false;
            this.radio_LoftAlwaysOff.CheckedChanged += new System.EventHandler(this.Save_ToggleLoftWhenStartup);
            // 
            // radio_LoftOffHotkey
            // 
            this.radio_LoftOffHotkey.AutoSize = true;
            this.radio_LoftOffHotkey.Location = new System.Drawing.Point(11, 42);
            this.radio_LoftOffHotkey.Name = "radio_LoftOffHotkey";
            this.radio_LoftOffHotkey.Size = new System.Drawing.Size(115, 17);
            this.radio_LoftOffHotkey.TabIndex = 45;
            this.radio_LoftOffHotkey.TabStop = true;
            this.radio_LoftOffHotkey.Text = "Loft Off Via Hotkey";
            this.radio_LoftOffHotkey.UseVisualStyleBackColor = true;
            this.radio_LoftOffHotkey.Visible = false;
            this.radio_LoftOffHotkey.CheckedChanged += new System.EventHandler(this.Save_ToggleLoftWhenManual);
            // 
            // radio_LoftOffInSong
            // 
            this.radio_LoftOffInSong.AutoSize = true;
            this.radio_LoftOffInSong.Location = new System.Drawing.Point(11, 65);
            this.radio_LoftOffInSong.Name = "radio_LoftOffInSong";
            this.radio_LoftOffInSong.Size = new System.Drawing.Size(100, 17);
            this.radio_LoftOffInSong.TabIndex = 46;
            this.radio_LoftOffInSong.TabStop = true;
            this.radio_LoftOffInSong.Text = "Loft Off In Song";
            this.radio_LoftOffInSong.UseVisualStyleBackColor = true;
            this.radio_LoftOffInSong.Visible = false;
            this.radio_LoftOffInSong.CheckedChanged += new System.EventHandler(this.Save_ToggleLoftWhenSong);
            // 
            // radio_ForceEnumerationManual
            // 
            this.radio_ForceEnumerationManual.AutoSize = true;
            this.radio_ForceEnumerationManual.Checked = true;
            this.radio_ForceEnumerationManual.Location = new System.Drawing.Point(14, 23);
            this.radio_ForceEnumerationManual.Name = "radio_ForceEnumerationManual";
            this.radio_ForceEnumerationManual.Size = new System.Drawing.Size(112, 17);
            this.radio_ForceEnumerationManual.TabIndex = 24;
            this.radio_ForceEnumerationManual.TabStop = true;
            this.radio_ForceEnumerationManual.Text = "Manual (Keypress)";
            this.radio_ForceEnumerationManual.UseVisualStyleBackColor = true;
            this.radio_ForceEnumerationManual.Visible = false;
            this.radio_ForceEnumerationManual.CheckedChanged += new System.EventHandler(this.Save_ForceEnumerationManual);
            // 
            // radio_ForceEnumerationAutomatic
            // 
            this.radio_ForceEnumerationAutomatic.AutoSize = true;
            this.radio_ForceEnumerationAutomatic.Location = new System.Drawing.Point(14, 48);
            this.radio_ForceEnumerationAutomatic.Name = "radio_ForceEnumerationAutomatic";
            this.radio_ForceEnumerationAutomatic.Size = new System.Drawing.Size(180, 17);
            this.radio_ForceEnumerationAutomatic.TabIndex = 25;
            this.radio_ForceEnumerationAutomatic.Text = "Automatic (Scan For New CDLC)";
            this.radio_ForceEnumerationAutomatic.UseVisualStyleBackColor = true;
            this.radio_ForceEnumerationAutomatic.Visible = false;
            this.radio_ForceEnumerationAutomatic.CheckedChanged += new System.EventHandler(this.Save_ForceEnumerationAutomatic);
            // 
            // groupBox_HowToEnumerate
            // 
            this.groupBox_HowToEnumerate.Controls.Add(this.label_ForceEnumerationXMS);
            this.groupBox_HowToEnumerate.Controls.Add(this.nUpDown_ForceEnumerationXMS);
            this.groupBox_HowToEnumerate.Controls.Add(this.radio_ForceEnumerationAutomatic);
            this.groupBox_HowToEnumerate.Controls.Add(this.radio_ForceEnumerationManual);
            this.groupBox_HowToEnumerate.Location = new System.Drawing.Point(12, 240);
            this.groupBox_HowToEnumerate.Name = "groupBox_HowToEnumerate";
            this.groupBox_HowToEnumerate.Size = new System.Drawing.Size(293, 77);
            this.groupBox_HowToEnumerate.TabIndex = 47;
            this.groupBox_HowToEnumerate.TabStop = false;
            this.groupBox_HowToEnumerate.Text = "How Do You Want To Enumerate?";
            this.groupBox_HowToEnumerate.Visible = false;
            this.groupBox_HowToEnumerate.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.groupBox_HowToEnumerate.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_ForceEnumerationXMS
            // 
            this.label_ForceEnumerationXMS.AutoSize = true;
            this.label_ForceEnumerationXMS.Location = new System.Drawing.Point(164, 24);
            this.label_ForceEnumerationXMS.Name = "label_ForceEnumerationXMS";
            this.label_ForceEnumerationXMS.Size = new System.Drawing.Size(123, 13);
            this.label_ForceEnumerationXMS.TabIndex = 27;
            this.label_ForceEnumerationXMS.Text = "Check Every X Seconds";
            this.label_ForceEnumerationXMS.Visible = false;
            // 
            // nUpDown_ForceEnumerationXMS
            // 
            this.nUpDown_ForceEnumerationXMS.DecimalPlaces = 1;
            this.nUpDown_ForceEnumerationXMS.Increment = new decimal(new int[] {
            5,
            0,
            0,
            65536});
            this.nUpDown_ForceEnumerationXMS.Location = new System.Drawing.Point(220, 48);
            this.nUpDown_ForceEnumerationXMS.Maximum = new decimal(new int[] {
            100000,
            0,
            0,
            0});
            this.nUpDown_ForceEnumerationXMS.Name = "nUpDown_ForceEnumerationXMS";
            this.nUpDown_ForceEnumerationXMS.Size = new System.Drawing.Size(58, 20);
            this.nUpDown_ForceEnumerationXMS.TabIndex = 26;
            this.nUpDown_ForceEnumerationXMS.Value = new decimal(new int[] {
            5,
            0,
            0,
            0});
            this.nUpDown_ForceEnumerationXMS.Visible = false;
            this.nUpDown_ForceEnumerationXMS.ValueChanged += new System.EventHandler(this.Save_EnumerateEveryXMS);
            // 
            // groupBox_LoftOffWhen
            // 
            this.groupBox_LoftOffWhen.Controls.Add(this.radio_LoftOffInSong);
            this.groupBox_LoftOffWhen.Controls.Add(this.radio_LoftOffHotkey);
            this.groupBox_LoftOffWhen.Controls.Add(this.radio_LoftAlwaysOff);
            this.groupBox_LoftOffWhen.Location = new System.Drawing.Point(511, 169);
            this.groupBox_LoftOffWhen.Name = "groupBox_LoftOffWhen";
            this.groupBox_LoftOffWhen.Size = new System.Drawing.Size(145, 86);
            this.groupBox_LoftOffWhen.TabIndex = 48;
            this.groupBox_LoftOffWhen.TabStop = false;
            this.groupBox_LoftOffWhen.Text = "Toggle Loft Off When:";
            this.groupBox_LoftOffWhen.Visible = false;
            this.groupBox_LoftOffWhen.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.groupBox_LoftOffWhen.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_ExtendedRangeWhen
            // 
            this.groupBox_ExtendedRangeWhen.Controls.Add(this.checkBox_ExtendedRangeDrop);
            this.groupBox_ExtendedRangeWhen.Controls.Add(this.listBox_ExtendedRangeTunings);
            this.groupBox_ExtendedRangeWhen.Location = new System.Drawing.Point(15, 20);
            this.groupBox_ExtendedRangeWhen.Name = "groupBox_ExtendedRangeWhen";
            this.groupBox_ExtendedRangeWhen.Size = new System.Drawing.Size(214, 205);
            this.groupBox_ExtendedRangeWhen.TabIndex = 49;
            this.groupBox_ExtendedRangeWhen.TabStop = false;
            this.groupBox_ExtendedRangeWhen.Text = "Enable Extended Range When Low E Is";
            this.groupBox_ExtendedRangeWhen.Visible = false;
            this.groupBox_ExtendedRangeWhen.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.groupBox_ExtendedRangeWhen.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ExtendedRangeDrop
            // 
            this.checkBox_ExtendedRangeDrop.AutoSize = true;
            this.checkBox_ExtendedRangeDrop.Location = new System.Drawing.Point(51, 180);
            this.checkBox_ExtendedRangeDrop.Name = "checkBox_ExtendedRangeDrop";
            this.checkBox_ExtendedRangeDrop.Size = new System.Drawing.Size(97, 17);
            this.checkBox_ExtendedRangeDrop.TabIndex = 30;
            this.checkBox_ExtendedRangeDrop.Text = "In Drop Tuning";
            this.checkBox_ExtendedRangeDrop.UseVisualStyleBackColor = true;
            this.checkBox_ExtendedRangeDrop.CheckedChanged += new System.EventHandler(this.Save_ExtendedRangeDrop);
            this.checkBox_ExtendedRangeDrop.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_EnabledMods
            // 
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_CustomHighway);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_RainbowNotes);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_BackupProfile);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_ShowCurrentNote);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_useMidiAutoTuning);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_RiffRepeaterSpeedAboveOneHundred);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_ScreenShotScores);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_ControlVolume);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_GuitarSpeak);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_RemoveLyrics);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_CustomColors);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_RemoveLaneMarkers);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_RemoveInlays);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_SongTimer);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_ToggleLoft);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_Fretless);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_AutoLoadProfile);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_RainbowStrings);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_ExtendedRange);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_GreenScreen);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_ForceEnumeration);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_RemoveSkyline);
            this.groupBox_EnabledMods.Controls.Add(this.checkBox_RemoveHeadstock);
            this.groupBox_EnabledMods.Location = new System.Drawing.Point(15, 14);
            this.groupBox_EnabledMods.Name = "groupBox_EnabledMods";
            this.groupBox_EnabledMods.Size = new System.Drawing.Size(297, 298);
            this.groupBox_EnabledMods.TabIndex = 50;
            this.groupBox_EnabledMods.TabStop = false;
            this.groupBox_EnabledMods.Text = "Enabled Mods";
            // 
            // checkBox_CustomHighway
            // 
            this.checkBox_CustomHighway.AutoSize = true;
            this.checkBox_CustomHighway.Location = new System.Drawing.Point(11, 266);
            this.checkBox_CustomHighway.Name = "checkBox_CustomHighway";
            this.checkBox_CustomHighway.Size = new System.Drawing.Size(105, 17);
            this.checkBox_CustomHighway.TabIndex = 54;
            this.checkBox_CustomHighway.Text = "Custom Highway";
            this.checkBox_CustomHighway.UseVisualStyleBackColor = true;
            this.checkBox_CustomHighway.CheckedChanged += new System.EventHandler(this.Save_CustomHighway);
            this.checkBox_CustomHighway.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_RainbowNotes
            // 
            this.checkBox_RainbowNotes.AutoSize = true;
            this.checkBox_RainbowNotes.Location = new System.Drawing.Point(131, 61);
            this.checkBox_RainbowNotes.Name = "checkBox_RainbowNotes";
            this.checkBox_RainbowNotes.Size = new System.Drawing.Size(99, 17);
            this.checkBox_RainbowNotes.TabIndex = 53;
            this.checkBox_RainbowNotes.Text = "Rainbow Notes";
            this.checkBox_RainbowNotes.UseVisualStyleBackColor = true;
            this.checkBox_RainbowNotes.CheckedChanged += new System.EventHandler(this.Save_RainbowNotes);
            this.checkBox_RainbowNotes.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_BackupProfile
            // 
            this.checkBox_BackupProfile.AutoSize = true;
            this.checkBox_BackupProfile.Location = new System.Drawing.Point(11, 243);
            this.checkBox_BackupProfile.Name = "checkBox_BackupProfile";
            this.checkBox_BackupProfile.Size = new System.Drawing.Size(95, 17);
            this.checkBox_BackupProfile.TabIndex = 52;
            this.checkBox_BackupProfile.Text = "Backup Profile";
            this.checkBox_BackupProfile.UseVisualStyleBackColor = true;
            this.checkBox_BackupProfile.CheckedChanged += new System.EventHandler(this.Save_BackupProfile);
            this.checkBox_BackupProfile.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ShowCurrentNote
            // 
            this.checkBox_ShowCurrentNote.AutoSize = true;
            this.checkBox_ShowCurrentNote.Location = new System.Drawing.Point(131, 243);
            this.checkBox_ShowCurrentNote.Name = "checkBox_ShowCurrentNote";
            this.checkBox_ShowCurrentNote.Size = new System.Drawing.Size(116, 17);
            this.checkBox_ShowCurrentNote.TabIndex = 51;
            this.checkBox_ShowCurrentNote.Text = "Show Current Note";
            this.checkBox_ShowCurrentNote.UseVisualStyleBackColor = true;
            this.checkBox_ShowCurrentNote.CheckedChanged += new System.EventHandler(this.Save_ShowCurrentNote);
            this.checkBox_ShowCurrentNote.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_useMidiAutoTuning
            // 
            this.checkBox_useMidiAutoTuning.AutoSize = true;
            this.checkBox_useMidiAutoTuning.Location = new System.Drawing.Point(11, 220);
            this.checkBox_useMidiAutoTuning.Name = "checkBox_useMidiAutoTuning";
            this.checkBox_useMidiAutoTuning.Size = new System.Drawing.Size(104, 17);
            this.checkBox_useMidiAutoTuning.TabIndex = 50;
            this.checkBox_useMidiAutoTuning.Text = "Auto Tune Song";
            this.checkBox_useMidiAutoTuning.UseVisualStyleBackColor = true;
            this.checkBox_useMidiAutoTuning.CheckedChanged += new System.EventHandler(this.Save_useMidiAutoTuning);
            this.checkBox_useMidiAutoTuning.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_RiffRepeaterSpeedAboveOneHundred
            // 
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.AutoSize = true;
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.Location = new System.Drawing.Point(131, 220);
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.Name = "checkBox_RiffRepeaterSpeedAboveOneHundred";
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.Size = new System.Drawing.Size(159, 17);
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.TabIndex = 49;
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.Text = "Allow RR Speed Above 100";
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.UseVisualStyleBackColor = true;
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.CheckedChanged += new System.EventHandler(this.Save_RiffRepeaterSpeedAboveOneHundred);
            this.checkBox_RiffRepeaterSpeedAboveOneHundred.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ScreenShotScores
            // 
            this.checkBox_ScreenShotScores.AutoSize = true;
            this.checkBox_ScreenShotScores.Location = new System.Drawing.Point(11, 197);
            this.checkBox_ScreenShotScores.Name = "checkBox_ScreenShotScores";
            this.checkBox_ScreenShotScores.Size = new System.Drawing.Size(116, 17);
            this.checkBox_ScreenShotScores.TabIndex = 48;
            this.checkBox_ScreenShotScores.Text = "Screenshot Scores";
            this.checkBox_ScreenShotScores.UseVisualStyleBackColor = true;
            this.checkBox_ScreenShotScores.CheckedChanged += new System.EventHandler(this.Save_ScreenShotScores);
            this.checkBox_ScreenShotScores.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ControlVolume
            // 
            this.checkBox_ControlVolume.AutoSize = true;
            this.checkBox_ControlVolume.Location = new System.Drawing.Point(131, 198);
            this.checkBox_ControlVolume.Name = "checkBox_ControlVolume";
            this.checkBox_ControlVolume.Size = new System.Drawing.Size(128, 17);
            this.checkBox_ControlVolume.TabIndex = 47;
            this.checkBox_ControlVolume.Text = "Control Game Volume";
            this.checkBox_ControlVolume.UseVisualStyleBackColor = true;
            this.checkBox_ControlVolume.CheckedChanged += new System.EventHandler(this.Save_VolumeControls);
            this.checkBox_ControlVolume.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_ControlVolume.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_GuitarSpeak
            // 
            this.checkBox_GuitarSpeak.AutoSize = true;
            this.checkBox_GuitarSpeak.Location = new System.Drawing.Point(11, 175);
            this.checkBox_GuitarSpeak.Name = "checkBox_GuitarSpeak";
            this.checkBox_GuitarSpeak.Size = new System.Drawing.Size(88, 17);
            this.checkBox_GuitarSpeak.TabIndex = 46;
            this.checkBox_GuitarSpeak.Text = "Guitar Speak";
            this.checkBox_GuitarSpeak.UseVisualStyleBackColor = true;
            this.checkBox_GuitarSpeak.CheckedChanged += new System.EventHandler(this.GuitarSpeakEnableCheckbox_CheckedChanged);
            this.checkBox_GuitarSpeak.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_GuitarSpeak.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_RemoveLyrics
            // 
            this.checkBox_RemoveLyrics.AutoSize = true;
            this.checkBox_RemoveLyrics.Location = new System.Drawing.Point(131, 15);
            this.checkBox_RemoveLyrics.Name = "checkBox_RemoveLyrics";
            this.checkBox_RemoveLyrics.Size = new System.Drawing.Size(96, 17);
            this.checkBox_RemoveLyrics.TabIndex = 45;
            this.checkBox_RemoveLyrics.Text = "Remove Lyrics";
            this.checkBox_RemoveLyrics.UseVisualStyleBackColor = true;
            this.checkBox_RemoveLyrics.CheckedChanged += new System.EventHandler(this.Save_RemoveLyrics);
            this.checkBox_RemoveLyrics.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_RemoveLyrics.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_CustomColors
            // 
            this.checkBox_CustomColors.AutoSize = true;
            this.checkBox_CustomColors.Location = new System.Drawing.Point(131, 130);
            this.checkBox_CustomColors.Name = "checkBox_CustomColors";
            this.checkBox_CustomColors.Size = new System.Drawing.Size(123, 17);
            this.checkBox_CustomColors.TabIndex = 44;
            this.checkBox_CustomColors.Text = "Custom String Colors";
            this.checkBox_CustomColors.UseVisualStyleBackColor = true;
            this.checkBox_CustomColors.CheckedChanged += new System.EventHandler(this.Save_CustomStringColors);
            this.checkBox_CustomColors.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_CustomColors.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_RemoveLaneMarkers
            // 
            this.checkBox_RemoveLaneMarkers.AutoSize = true;
            this.checkBox_RemoveLaneMarkers.Location = new System.Drawing.Point(131, 84);
            this.checkBox_RemoveLaneMarkers.Name = "checkBox_RemoveLaneMarkers";
            this.checkBox_RemoveLaneMarkers.Size = new System.Drawing.Size(134, 17);
            this.checkBox_RemoveLaneMarkers.TabIndex = 43;
            this.checkBox_RemoveLaneMarkers.Text = "Remove Lane Markers";
            this.checkBox_RemoveLaneMarkers.UseVisualStyleBackColor = true;
            this.checkBox_RemoveLaneMarkers.CheckedChanged += new System.EventHandler(this.Save_RemoveLaneMarkers);
            this.checkBox_RemoveLaneMarkers.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_RemoveLaneMarkers.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_RainbowStrings
            // 
            this.checkBox_RainbowStrings.AutoSize = true;
            this.checkBox_RainbowStrings.Location = new System.Drawing.Point(11, 61);
            this.checkBox_RainbowStrings.Name = "checkBox_RainbowStrings";
            this.checkBox_RainbowStrings.Size = new System.Drawing.Size(103, 17);
            this.checkBox_RainbowStrings.TabIndex = 26;
            this.checkBox_RainbowStrings.Text = "Rainbow Strings";
            this.checkBox_RainbowStrings.UseVisualStyleBackColor = true;
            this.checkBox_RainbowStrings.CheckedChanged += new System.EventHandler(this.Save_RainbowStrings);
            this.checkBox_RainbowStrings.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkBox_RainbowStrings.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_Songlist
            // 
            this.groupBox_Songlist.Controls.Add(this.label_SonglistWarning);
            this.groupBox_Songlist.Controls.Add(this.textBox_NewSonglistName);
            this.groupBox_Songlist.Controls.Add(this.label_ChangeSonglistName);
            this.groupBox_Songlist.Controls.Add(this.listBox_Songlist);
            this.groupBox_Songlist.Location = new System.Drawing.Point(8, 8);
            this.groupBox_Songlist.Name = "groupBox_Songlist";
            this.groupBox_Songlist.Size = new System.Drawing.Size(459, 200);
            this.groupBox_Songlist.TabIndex = 51;
            this.groupBox_Songlist.TabStop = false;
            this.groupBox_Songlist.Text = "Song Lists";
            this.groupBox_Songlist.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.groupBox_Songlist.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_SonglistWarning
            // 
            this.label_SonglistWarning.AutoSize = true;
            this.label_SonglistWarning.Location = new System.Drawing.Point(245, 114);
            this.label_SonglistWarning.Name = "label_SonglistWarning";
            this.label_SonglistWarning.Size = new System.Drawing.Size(183, 78);
            this.label_SonglistWarning.TabIndex = 24;
            this.label_SonglistWarning.Text = "Unlimited (untested) length available\r\nas the text streches\r\n\r\nWarning: This work" +
    "s best if your\r\nsonglist name is below 25 characters,\r\nas otherwise the text str" +
    "eches oddly.\r\n";
            this.label_SonglistWarning.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // groupBox_Keybindings_MODS
            // 
            this.groupBox_Keybindings_MODS.Controls.Add(this.label_RainbowNotesKey);
            this.groupBox_Keybindings_MODS.Controls.Add(this.label_RRSpeedKey);
            this.groupBox_Keybindings_MODS.Controls.Add(this.label_RemoveLyricsKey);
            this.groupBox_Keybindings_MODS.Controls.Add(this.button_ClearSelectedKeybind_MODS);
            this.groupBox_Keybindings_MODS.Controls.Add(this.label_NewKeyAssignment_MODS);
            this.groupBox_Keybindings_MODS.Controls.Add(this.textBox_NewKeyAssignment_MODS);
            this.groupBox_Keybindings_MODS.Controls.Add(this.label_ReEnumerationKey);
            this.groupBox_Keybindings_MODS.Controls.Add(this.label_SongTimerKey);
            this.groupBox_Keybindings_MODS.Controls.Add(this.label_ToggleLoftKey);
            this.groupBox_Keybindings_MODS.Controls.Add(this.label_RainbowStringsKey);
            this.groupBox_Keybindings_MODS.Controls.Add(this.listBox_Modlist_MODS);
            this.groupBox_Keybindings_MODS.Location = new System.Drawing.Point(15, 17);
            this.groupBox_Keybindings_MODS.Name = "groupBox_Keybindings_MODS";
            this.groupBox_Keybindings_MODS.Size = new System.Drawing.Size(701, 178);
            this.groupBox_Keybindings_MODS.TabIndex = 52;
            this.groupBox_Keybindings_MODS.TabStop = false;
            this.groupBox_Keybindings_MODS.Text = "Mod Key Bindings";
            this.groupBox_Keybindings_MODS.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.groupBox_Keybindings_MODS.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_RainbowNotesKey
            // 
            this.label_RainbowNotesKey.AutoSize = true;
            this.label_RainbowNotesKey.Location = new System.Drawing.Point(464, 101);
            this.label_RainbowNotesKey.Name = "label_RainbowNotesKey";
            this.label_RainbowNotesKey.Size = new System.Drawing.Size(86, 13);
            this.label_RainbowNotesKey.TabIndex = 54;
            this.label_RainbowNotesKey.Text = "Rainbow Notes: ";
            // 
            // label_RRSpeedKey
            // 
            this.label_RRSpeedKey.AutoSize = true;
            this.label_RRSpeedKey.Location = new System.Drawing.Point(464, 147);
            this.label_RRSpeedKey.Name = "label_RRSpeedKey";
            this.label_RRSpeedKey.Size = new System.Drawing.Size(110, 13);
            this.label_RRSpeedKey.TabIndex = 53;
            this.label_RRSpeedKey.Text = "Riff Repeater Speed: ";
            // 
            // label_RemoveLyricsKey
            // 
            this.label_RemoveLyricsKey.AutoSize = true;
            this.label_RemoveLyricsKey.Location = new System.Drawing.Point(464, 123);
            this.label_RemoveLyricsKey.Name = "label_RemoveLyricsKey";
            this.label_RemoveLyricsKey.Size = new System.Drawing.Size(83, 13);
            this.label_RemoveLyricsKey.TabIndex = 30;
            this.label_RemoveLyricsKey.Text = "Remove Lyrics: ";
            // 
            // button_ClearSelectedKeybind_MODS
            // 
            this.button_ClearSelectedKeybind_MODS.Location = new System.Drawing.Point(222, 130);
            this.button_ClearSelectedKeybind_MODS.Name = "button_ClearSelectedKeybind_MODS";
            this.button_ClearSelectedKeybind_MODS.Size = new System.Drawing.Size(188, 23);
            this.button_ClearSelectedKeybind_MODS.TabIndex = 29;
            this.button_ClearSelectedKeybind_MODS.Text = "Clear Selected Keybind";
            this.button_ClearSelectedKeybind_MODS.UseVisualStyleBackColor = true;
            this.button_ClearSelectedKeybind_MODS.Click += new System.EventHandler(this.Delete_Keybind_MODS);
            // 
            // button_ResetModsToDefault
            // 
            this.button_ResetModsToDefault.Location = new System.Drawing.Point(1009, 485);
            this.button_ResetModsToDefault.Name = "button_ResetModsToDefault";
            this.button_ResetModsToDefault.Size = new System.Drawing.Size(127, 22);
            this.button_ResetModsToDefault.TabIndex = 100000;
            this.button_ResetModsToDefault.Text = "Reset Mods To Default";
            this.button_ResetModsToDefault.UseVisualStyleBackColor = true;
            this.button_ResetModsToDefault.Click += new System.EventHandler(this.ResetToDefaultSettings);
            this.button_ResetModsToDefault.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_ResetModsToDefault.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_String0ColorButton
            // 
            this.button_String0ColorButton.Location = new System.Drawing.Point(26, 63);
            this.button_String0ColorButton.Name = "button_String0ColorButton";
            this.button_String0ColorButton.Size = new System.Drawing.Size(120, 21);
            this.button_String0ColorButton.TabIndex = 53;
            this.button_String0ColorButton.Text = "E String";
            this.button_String0ColorButton.UseVisualStyleBackColor = true;
            this.button_String0ColorButton.Click += new System.EventHandler(this.StringColors_ChangeStringColor);
            // 
            // textBox_String0Color
            // 
            this.textBox_String0Color.Enabled = false;
            this.textBox_String0Color.Location = new System.Drawing.Point(152, 63);
            this.textBox_String0Color.Name = "textBox_String0Color";
            this.textBox_String0Color.ReadOnly = true;
            this.textBox_String0Color.Size = new System.Drawing.Size(108, 20);
            this.textBox_String0Color.TabIndex = 54;
            // 
            // textBox_String1Color
            // 
            this.textBox_String1Color.Enabled = false;
            this.textBox_String1Color.Location = new System.Drawing.Point(152, 93);
            this.textBox_String1Color.Name = "textBox_String1Color";
            this.textBox_String1Color.ReadOnly = true;
            this.textBox_String1Color.Size = new System.Drawing.Size(108, 20);
            this.textBox_String1Color.TabIndex = 56;
            // 
            // button_String1ColorButton
            // 
            this.button_String1ColorButton.Location = new System.Drawing.Point(26, 93);
            this.button_String1ColorButton.Name = "button_String1ColorButton";
            this.button_String1ColorButton.Size = new System.Drawing.Size(120, 21);
            this.button_String1ColorButton.TabIndex = 55;
            this.button_String1ColorButton.Text = "A String";
            this.button_String1ColorButton.UseVisualStyleBackColor = true;
            this.button_String1ColorButton.Click += new System.EventHandler(this.StringColors_ChangeStringColor);
            // 
            // textBox_String2Color
            // 
            this.textBox_String2Color.Enabled = false;
            this.textBox_String2Color.Location = new System.Drawing.Point(152, 122);
            this.textBox_String2Color.Name = "textBox_String2Color";
            this.textBox_String2Color.ReadOnly = true;
            this.textBox_String2Color.Size = new System.Drawing.Size(108, 20);
            this.textBox_String2Color.TabIndex = 58;
            // 
            // button_String2ColorButton
            // 
            this.button_String2ColorButton.Location = new System.Drawing.Point(26, 122);
            this.button_String2ColorButton.Name = "button_String2ColorButton";
            this.button_String2ColorButton.Size = new System.Drawing.Size(120, 21);
            this.button_String2ColorButton.TabIndex = 57;
            this.button_String2ColorButton.Text = "D String";
            this.button_String2ColorButton.UseVisualStyleBackColor = true;
            this.button_String2ColorButton.Click += new System.EventHandler(this.StringColors_ChangeStringColor);
            // 
            // textBox_String3Color
            // 
            this.textBox_String3Color.Enabled = false;
            this.textBox_String3Color.Location = new System.Drawing.Point(152, 153);
            this.textBox_String3Color.Name = "textBox_String3Color";
            this.textBox_String3Color.ReadOnly = true;
            this.textBox_String3Color.Size = new System.Drawing.Size(108, 20);
            this.textBox_String3Color.TabIndex = 60;
            // 
            // button_String3ColorButton
            // 
            this.button_String3ColorButton.Location = new System.Drawing.Point(26, 153);
            this.button_String3ColorButton.Name = "button_String3ColorButton";
            this.button_String3ColorButton.Size = new System.Drawing.Size(120, 21);
            this.button_String3ColorButton.TabIndex = 59;
            this.button_String3ColorButton.Text = "G String";
            this.button_String3ColorButton.UseVisualStyleBackColor = true;
            this.button_String3ColorButton.Click += new System.EventHandler(this.StringColors_ChangeStringColor);
            // 
            // textBox_String4Color
            // 
            this.textBox_String4Color.Enabled = false;
            this.textBox_String4Color.Location = new System.Drawing.Point(152, 185);
            this.textBox_String4Color.Name = "textBox_String4Color";
            this.textBox_String4Color.ReadOnly = true;
            this.textBox_String4Color.Size = new System.Drawing.Size(108, 20);
            this.textBox_String4Color.TabIndex = 62;
            // 
            // button_String4ColorButton
            // 
            this.button_String4ColorButton.Location = new System.Drawing.Point(26, 185);
            this.button_String4ColorButton.Name = "button_String4ColorButton";
            this.button_String4ColorButton.Size = new System.Drawing.Size(120, 21);
            this.button_String4ColorButton.TabIndex = 61;
            this.button_String4ColorButton.Text = "B String";
            this.button_String4ColorButton.UseVisualStyleBackColor = true;
            this.button_String4ColorButton.Click += new System.EventHandler(this.StringColors_ChangeStringColor);
            // 
            // textBox_String5Color
            // 
            this.textBox_String5Color.Enabled = false;
            this.textBox_String5Color.Location = new System.Drawing.Point(152, 215);
            this.textBox_String5Color.Name = "textBox_String5Color";
            this.textBox_String5Color.ReadOnly = true;
            this.textBox_String5Color.Size = new System.Drawing.Size(108, 20);
            this.textBox_String5Color.TabIndex = 64;
            // 
            // button_String5ColorButton
            // 
            this.button_String5ColorButton.Location = new System.Drawing.Point(26, 215);
            this.button_String5ColorButton.Name = "button_String5ColorButton";
            this.button_String5ColorButton.Size = new System.Drawing.Size(120, 21);
            this.button_String5ColorButton.TabIndex = 63;
            this.button_String5ColorButton.Text = "e String";
            this.button_String5ColorButton.UseVisualStyleBackColor = true;
            this.button_String5ColorButton.Click += new System.EventHandler(this.StringColors_ChangeStringColor);
            // 
            // groupBox_StringColors
            // 
            this.groupBox_StringColors.Controls.Add(this.radio_colorBlindERColors);
            this.groupBox_StringColors.Controls.Add(this.radio_DefaultStringColors);
            this.groupBox_StringColors.Controls.Add(this.textBox_String5Color);
            this.groupBox_StringColors.Controls.Add(this.button_String5ColorButton);
            this.groupBox_StringColors.Controls.Add(this.textBox_String4Color);
            this.groupBox_StringColors.Controls.Add(this.button_String4ColorButton);
            this.groupBox_StringColors.Controls.Add(this.textBox_String3Color);
            this.groupBox_StringColors.Controls.Add(this.button_String3ColorButton);
            this.groupBox_StringColors.Controls.Add(this.textBox_String2Color);
            this.groupBox_StringColors.Controls.Add(this.button_String2ColorButton);
            this.groupBox_StringColors.Controls.Add(this.textBox_String1Color);
            this.groupBox_StringColors.Controls.Add(this.button_String1ColorButton);
            this.groupBox_StringColors.Controls.Add(this.textBox_String0Color);
            this.groupBox_StringColors.Controls.Add(this.button_String0ColorButton);
            this.groupBox_StringColors.Location = new System.Drawing.Point(273, 20);
            this.groupBox_StringColors.Name = "groupBox_StringColors";
            this.groupBox_StringColors.Size = new System.Drawing.Size(282, 250);
            this.groupBox_StringColors.TabIndex = 65;
            this.groupBox_StringColors.TabStop = false;
            this.groupBox_StringColors.Text = "Change String Colors";
            this.groupBox_StringColors.Visible = false;
            // 
            // radio_colorBlindERColors
            // 
            this.radio_colorBlindERColors.AutoSize = true;
            this.radio_colorBlindERColors.Location = new System.Drawing.Point(138, 14);
            this.radio_colorBlindERColors.Name = "radio_colorBlindERColors";
            this.radio_colorBlindERColors.Size = new System.Drawing.Size(138, 43);
            this.radio_colorBlindERColors.TabIndex = 66;
            this.radio_colorBlindERColors.TabStop = true;
            this.radio_colorBlindERColors.Text = "Colorblind and \r\nExtended Range Mode \r\nColors";
            this.radio_colorBlindERColors.UseVisualStyleBackColor = true;
            this.radio_colorBlindERColors.CheckedChanged += new System.EventHandler(this.StringColors_ColorBlindStringColors);
            this.radio_colorBlindERColors.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.radio_colorBlindERColors.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_DefaultStringColors
            // 
            this.radio_DefaultStringColors.AutoSize = true;
            this.radio_DefaultStringColors.Checked = true;
            this.radio_DefaultStringColors.Location = new System.Drawing.Point(26, 27);
            this.radio_DefaultStringColors.Name = "radio_DefaultStringColors";
            this.radio_DefaultStringColors.Size = new System.Drawing.Size(91, 17);
            this.radio_DefaultStringColors.TabIndex = 65;
            this.radio_DefaultStringColors.TabStop = true;
            this.radio_DefaultStringColors.Text = "Default Colors";
            this.radio_DefaultStringColors.UseVisualStyleBackColor = true;
            this.radio_DefaultStringColors.CheckedChanged += new System.EventHandler(this.StringColors_DefaultStringColors);
            // 
            // groupBox_SetAndForget
            // 
            this.groupBox_SetAndForget.BackColor = System.Drawing.Color.Azure;
            this.groupBox_SetAndForget.Controls.Add(this.radio_GoneWailinTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_RainbowLaserTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_DucksReduxTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_HarmonicHeistTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_HurtlinHurdlesTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_NinjaSlideNTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_ScaleRacerTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_StringsSkipSaloonTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_ScaleWarriorsTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_TempleOfBendsTone);
            this.groupBox_SetAndForget.Controls.Add(this.button_AssignNewGuitarArcadeTone);
            this.groupBox_SetAndForget.Controls.Add(this.label_SetAndForgetGuitarArcade);
            this.groupBox_SetAndForget.Controls.Add(this.button_AddDCInput);
            this.groupBox_SetAndForget.Controls.Add(this.label_ChangeTonesHeader);
            this.groupBox_SetAndForget.Controls.Add(this.label_HorizontalRuleSetAndForget);
            this.groupBox_SetAndForget.Controls.Add(this.label_HorizontalRulerGuitarArcade);
            this.groupBox_SetAndForget.Controls.Add(this.button_ImportExistingSettings);
            this.groupBox_SetAndForget.Controls.Add(this.label_CustomTuningHighEStringLetter);
            this.groupBox_SetAndForget.Controls.Add(this.label_CustomTuningBStringLetter);
            this.groupBox_SetAndForget.Controls.Add(this.label_CustomTuningGStringLetter);
            this.groupBox_SetAndForget.Controls.Add(this.label_CustomTuningDStringLetter);
            this.groupBox_SetAndForget.Controls.Add(this.label_CustomTuningAStringLetter);
            this.groupBox_SetAndForget.Controls.Add(this.label_CustomTuningLowEStringLetter);
            this.groupBox_SetAndForget.Controls.Add(this.radio_DefaultBassTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_DefaultLeadTone);
            this.groupBox_SetAndForget.Controls.Add(this.radio_DefaultRhythmTone);
            this.groupBox_SetAndForget.Controls.Add(this.button_LoadTones);
            this.groupBox_SetAndForget.Controls.Add(this.listBox_ProfileTones);
            this.groupBox_SetAndForget.Controls.Add(this.button_AssignNewDefaultTone);
            this.groupBox_SetAndForget.Controls.Add(this.button_RemoveTemp);
            this.groupBox_SetAndForget.Controls.Add(this.button_AddExitGame);
            this.groupBox_SetAndForget.Controls.Add(this.button_AddFastLoad);
            this.groupBox_SetAndForget.Controls.Add(this.button_CleanUpUnpackedCache);
            this.groupBox_SetAndForget.Controls.Add(this.button_RestoreCacheBackup);
            this.groupBox_SetAndForget.Controls.Add(this.button_AddCustomTunings);
            this.groupBox_SetAndForget.Controls.Add(this.button_SaveTuningChanges);
            this.groupBox_SetAndForget.Controls.Add(this.label_CustomStringTunings);
            this.groupBox_SetAndForget.Controls.Add(this.nUpDown_String0);
            this.groupBox_SetAndForget.Controls.Add(this.label_UIIndex);
            this.groupBox_SetAndForget.Controls.Add(this.nUpDown_UIIndex);
            this.groupBox_SetAndForget.Controls.Add(this.label_UITuningName);
            this.groupBox_SetAndForget.Controls.Add(this.label_InternalTuningName);
            this.groupBox_SetAndForget.Controls.Add(this.nUpDown_String1);
            this.groupBox_SetAndForget.Controls.Add(this.nUpDown_String5);
            this.groupBox_SetAndForget.Controls.Add(this.nUpDown_String4);
            this.groupBox_SetAndForget.Controls.Add(this.nUpDown_String3);
            this.groupBox_SetAndForget.Controls.Add(this.nUpDown_String2);
            this.groupBox_SetAndForget.Controls.Add(this.textBox_UIName);
            this.groupBox_SetAndForget.Controls.Add(this.textBox_InternalTuningName);
            this.groupBox_SetAndForget.Controls.Add(this.button_AddTuning);
            this.groupBox_SetAndForget.Controls.Add(this.button_RemoveTuning);
            this.groupBox_SetAndForget.Controls.Add(this.listBox_Tunings);
            this.groupBox_SetAndForget.Location = new System.Drawing.Point(6, 11);
            this.groupBox_SetAndForget.Name = "groupBox_SetAndForget";
            this.groupBox_SetAndForget.Size = new System.Drawing.Size(1131, 499);
            this.groupBox_SetAndForget.TabIndex = 68;
            this.groupBox_SetAndForget.TabStop = false;
            this.groupBox_SetAndForget.Text = "Set-and-forget mods";
            // 
            // radio_GoneWailinTone
            // 
            this.radio_GoneWailinTone.AutoSize = true;
            this.radio_GoneWailinTone.Location = new System.Drawing.Point(702, 424);
            this.radio_GoneWailinTone.Name = "radio_GoneWailinTone";
            this.radio_GoneWailinTone.Size = new System.Drawing.Size(256, 17);
            this.radio_GoneWailinTone.TabIndex = 124;
            this.radio_GoneWailinTone.Text = "Set Highlighted Tone As New Gone Wailin\' Tone";
            this.radio_GoneWailinTone.UseVisualStyleBackColor = true;
            // 
            // radio_RainbowLaserTone
            // 
            this.radio_RainbowLaserTone.AutoSize = true;
            this.radio_RainbowLaserTone.Location = new System.Drawing.Point(702, 401);
            this.radio_RainbowLaserTone.Name = "radio_RainbowLaserTone";
            this.radio_RainbowLaserTone.Size = new System.Drawing.Size(267, 17);
            this.radio_RainbowLaserTone.TabIndex = 123;
            this.radio_RainbowLaserTone.Text = "Set Highlighted Tone As New Rainbow Laser Tone";
            this.radio_RainbowLaserTone.UseVisualStyleBackColor = true;
            // 
            // radio_DucksReduxTone
            // 
            this.radio_DucksReduxTone.AutoSize = true;
            this.radio_DucksReduxTone.Location = new System.Drawing.Point(702, 378);
            this.radio_DucksReduxTone.Name = "radio_DucksReduxTone";
            this.radio_DucksReduxTone.Size = new System.Drawing.Size(261, 17);
            this.radio_DucksReduxTone.TabIndex = 122;
            this.radio_DucksReduxTone.Text = "Set Highlighted Tone As New Ducks Redux Tone";
            this.radio_DucksReduxTone.UseVisualStyleBackColor = true;
            // 
            // radio_HarmonicHeistTone
            // 
            this.radio_HarmonicHeistTone.AutoSize = true;
            this.radio_HarmonicHeistTone.Location = new System.Drawing.Point(702, 355);
            this.radio_HarmonicHeistTone.Name = "radio_HarmonicHeistTone";
            this.radio_HarmonicHeistTone.Size = new System.Drawing.Size(268, 17);
            this.radio_HarmonicHeistTone.TabIndex = 121;
            this.radio_HarmonicHeistTone.Text = "Set Highlighted Tone As New Harmonic Heist Tone";
            this.radio_HarmonicHeistTone.UseVisualStyleBackColor = true;
            // 
            // radio_HurtlinHurdlesTone
            // 
            this.radio_HurtlinHurdlesTone.AutoSize = true;
            this.radio_HurtlinHurdlesTone.Location = new System.Drawing.Point(702, 332);
            this.radio_HurtlinHurdlesTone.Name = "radio_HurtlinHurdlesTone";
            this.radio_HurtlinHurdlesTone.Size = new System.Drawing.Size(267, 17);
            this.radio_HurtlinHurdlesTone.TabIndex = 120;
            this.radio_HurtlinHurdlesTone.Text = "Set Highlighted Tone As New Hurtlin\' Hurdles Tone";
            this.radio_HurtlinHurdlesTone.UseVisualStyleBackColor = true;
            // 
            // radio_NinjaSlideNTone
            // 
            this.radio_NinjaSlideNTone.AutoSize = true;
            this.radio_NinjaSlideNTone.Location = new System.Drawing.Point(398, 424);
            this.radio_NinjaSlideNTone.Name = "radio_NinjaSlideNTone";
            this.radio_NinjaSlideNTone.Size = new System.Drawing.Size(257, 17);
            this.radio_NinjaSlideNTone.TabIndex = 119;
            this.radio_NinjaSlideNTone.Text = "Set Highlighted Tone As New Ninja Slide N Tone";
            this.radio_NinjaSlideNTone.UseVisualStyleBackColor = true;
            // 
            // radio_ScaleRacerTone
            // 
            this.radio_ScaleRacerTone.AutoSize = true;
            this.radio_ScaleRacerTone.Location = new System.Drawing.Point(398, 401);
            this.radio_ScaleRacerTone.Name = "radio_ScaleRacerTone";
            this.radio_ScaleRacerTone.Size = new System.Drawing.Size(255, 17);
            this.radio_ScaleRacerTone.TabIndex = 118;
            this.radio_ScaleRacerTone.Text = "Set Highlighted Tone As New Scale Racer Tone";
            this.radio_ScaleRacerTone.UseVisualStyleBackColor = true;
            // 
            // radio_StringsSkipSaloonTone
            // 
            this.radio_StringsSkipSaloonTone.AutoSize = true;
            this.radio_StringsSkipSaloonTone.Location = new System.Drawing.Point(398, 378);
            this.radio_StringsSkipSaloonTone.Name = "radio_StringsSkipSaloonTone";
            this.radio_StringsSkipSaloonTone.Size = new System.Drawing.Size(283, 17);
            this.radio_StringsSkipSaloonTone.TabIndex = 117;
            this.radio_StringsSkipSaloonTone.Text = "Set Highlighted Tone As New String Skip Saloon Tone";
            this.radio_StringsSkipSaloonTone.UseVisualStyleBackColor = true;
            // 
            // radio_ScaleWarriorsTone
            // 
            this.radio_ScaleWarriorsTone.AutoSize = true;
            this.radio_ScaleWarriorsTone.Location = new System.Drawing.Point(398, 355);
            this.radio_ScaleWarriorsTone.Name = "radio_ScaleWarriorsTone";
            this.radio_ScaleWarriorsTone.Size = new System.Drawing.Size(265, 17);
            this.radio_ScaleWarriorsTone.TabIndex = 116;
            this.radio_ScaleWarriorsTone.Text = "Set Highlighted Tone As New Scale Warriors Tone";
            this.radio_ScaleWarriorsTone.UseVisualStyleBackColor = true;
            // 
            // radio_TempleOfBendsTone
            // 
            this.radio_TempleOfBendsTone.AutoSize = true;
            this.radio_TempleOfBendsTone.Checked = true;
            this.radio_TempleOfBendsTone.Location = new System.Drawing.Point(398, 332);
            this.radio_TempleOfBendsTone.Name = "radio_TempleOfBendsTone";
            this.radio_TempleOfBendsTone.Size = new System.Drawing.Size(276, 17);
            this.radio_TempleOfBendsTone.TabIndex = 115;
            this.radio_TempleOfBendsTone.TabStop = true;
            this.radio_TempleOfBendsTone.Text = "Set Highlighted Tone As New Temple of Bends Tone";
            this.radio_TempleOfBendsTone.UseVisualStyleBackColor = true;
            // 
            // button_AssignNewGuitarArcadeTone
            // 
            this.button_AssignNewGuitarArcadeTone.Location = new System.Drawing.Point(398, 458);
            this.button_AssignNewGuitarArcadeTone.Name = "button_AssignNewGuitarArcadeTone";
            this.button_AssignNewGuitarArcadeTone.Size = new System.Drawing.Size(224, 35);
            this.button_AssignNewGuitarArcadeTone.TabIndex = 114;
            this.button_AssignNewGuitarArcadeTone.Text = "Assign Tone As New GuitarArcade Default";
            this.button_AssignNewGuitarArcadeTone.UseVisualStyleBackColor = true;
            this.button_AssignNewGuitarArcadeTone.Click += new System.EventHandler(this.SetAndForget_AssignNewGuitarArcadeTone);
            // 
            // label_SetAndForgetGuitarArcade
            // 
            this.label_SetAndForgetGuitarArcade.Location = new System.Drawing.Point(3, 316);
            this.label_SetAndForgetGuitarArcade.Name = "label_SetAndForgetGuitarArcade";
            this.label_SetAndForgetGuitarArcade.Size = new System.Drawing.Size(1132, 18);
            this.label_SetAndForgetGuitarArcade.TabIndex = 112;
            this.label_SetAndForgetGuitarArcade.Text = "Change GuitarArcade tones";
            // 
            // button_AddDCInput
            // 
            this.button_AddDCInput.Location = new System.Drawing.Point(723, 135);
            this.button_AddDCInput.Name = "button_AddDCInput";
            this.button_AddDCInput.Size = new System.Drawing.Size(187, 20);
            this.button_AddDCInput.TabIndex = 111;
            this.button_AddDCInput.Text = "Add Direct Connect Mode";
            this.button_AddDCInput.UseVisualStyleBackColor = true;
            this.button_AddDCInput.Click += new System.EventHandler(this.SetForget_AddDCMode);
            this.button_AddDCInput.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_ChangeTonesHeader
            // 
            this.label_ChangeTonesHeader.Location = new System.Drawing.Point(5, 158);
            this.label_ChangeTonesHeader.Name = "label_ChangeTonesHeader";
            this.label_ChangeTonesHeader.Size = new System.Drawing.Size(1132, 19);
            this.label_ChangeTonesHeader.TabIndex = 97;
            this.label_ChangeTonesHeader.Text = "Change Default Tones";
            this.label_ChangeTonesHeader.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.label_ChangeTonesHeader.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_HorizontalRuleSetAndForget
            // 
            this.label_HorizontalRuleSetAndForget.AutoSize = true;
            this.label_HorizontalRuleSetAndForget.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.label_HorizontalRuleSetAndForget.Location = new System.Drawing.Point(8, 156);
            this.label_HorizontalRuleSetAndForget.Name = "label_HorizontalRuleSetAndForget";
            this.label_HorizontalRuleSetAndForget.Size = new System.Drawing.Size(1119, 15);
            this.label_HorizontalRuleSetAndForget.TabIndex = 110;
            this.label_HorizontalRuleSetAndForget.Text = resources.GetString("label_HorizontalRuleSetAndForget.Text");
            // 
            // label_HorizontalRulerGuitarArcade
            // 
            this.label_HorizontalRulerGuitarArcade.AutoSize = true;
            this.label_HorizontalRulerGuitarArcade.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.label_HorizontalRulerGuitarArcade.Location = new System.Drawing.Point(6, 314);
            this.label_HorizontalRulerGuitarArcade.Name = "label_HorizontalRulerGuitarArcade";
            this.label_HorizontalRulerGuitarArcade.Size = new System.Drawing.Size(1119, 15);
            this.label_HorizontalRulerGuitarArcade.TabIndex = 113;
            this.label_HorizontalRulerGuitarArcade.Text = resources.GetString("label_HorizontalRulerGuitarArcade.Text");
            // 
            // button_ImportExistingSettings
            // 
            this.button_ImportExistingSettings.Location = new System.Drawing.Point(916, 14);
            this.button_ImportExistingSettings.Name = "button_ImportExistingSettings";
            this.button_ImportExistingSettings.Size = new System.Drawing.Size(190, 40);
            this.button_ImportExistingSettings.TabIndex = 109;
            this.button_ImportExistingSettings.Text = "Import Existing Settings";
            this.button_ImportExistingSettings.UseVisualStyleBackColor = true;
            this.button_ImportExistingSettings.Click += new System.EventHandler(this.SetForget_ImportExistingSettings);
            // 
            // label_CustomTuningHighEStringLetter
            // 
            this.label_CustomTuningHighEStringLetter.AutoSize = true;
            this.label_CustomTuningHighEStringLetter.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_CustomTuningHighEStringLetter.Location = new System.Drawing.Point(674, 41);
            this.label_CustomTuningHighEStringLetter.Name = "label_CustomTuningHighEStringLetter";
            this.label_CustomTuningHighEStringLetter.Size = new System.Drawing.Size(21, 13);
            this.label_CustomTuningHighEStringLetter.TabIndex = 108;
            this.label_CustomTuningHighEStringLetter.Text = "e4";
            // 
            // label_CustomTuningBStringLetter
            // 
            this.label_CustomTuningBStringLetter.AutoSize = true;
            this.label_CustomTuningBStringLetter.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_CustomTuningBStringLetter.Location = new System.Drawing.Point(627, 41);
            this.label_CustomTuningBStringLetter.Name = "label_CustomTuningBStringLetter";
            this.label_CustomTuningBStringLetter.Size = new System.Drawing.Size(22, 13);
            this.label_CustomTuningBStringLetter.TabIndex = 107;
            this.label_CustomTuningBStringLetter.Text = "B3";
            // 
            // label_CustomTuningGStringLetter
            // 
            this.label_CustomTuningGStringLetter.AutoSize = true;
            this.label_CustomTuningGStringLetter.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_CustomTuningGStringLetter.Location = new System.Drawing.Point(582, 41);
            this.label_CustomTuningGStringLetter.Name = "label_CustomTuningGStringLetter";
            this.label_CustomTuningGStringLetter.Size = new System.Drawing.Size(23, 13);
            this.label_CustomTuningGStringLetter.TabIndex = 106;
            this.label_CustomTuningGStringLetter.Text = "G3";
            // 
            // label_CustomTuningDStringLetter
            // 
            this.label_CustomTuningDStringLetter.AutoSize = true;
            this.label_CustomTuningDStringLetter.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_CustomTuningDStringLetter.Location = new System.Drawing.Point(537, 41);
            this.label_CustomTuningDStringLetter.Name = "label_CustomTuningDStringLetter";
            this.label_CustomTuningDStringLetter.Size = new System.Drawing.Size(23, 13);
            this.label_CustomTuningDStringLetter.TabIndex = 105;
            this.label_CustomTuningDStringLetter.Text = "D3";
            // 
            // label_CustomTuningAStringLetter
            // 
            this.label_CustomTuningAStringLetter.AutoSize = true;
            this.label_CustomTuningAStringLetter.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_CustomTuningAStringLetter.Location = new System.Drawing.Point(494, 41);
            this.label_CustomTuningAStringLetter.Name = "label_CustomTuningAStringLetter";
            this.label_CustomTuningAStringLetter.Size = new System.Drawing.Size(22, 13);
            this.label_CustomTuningAStringLetter.TabIndex = 104;
            this.label_CustomTuningAStringLetter.Text = "A2";
            // 
            // label_CustomTuningLowEStringLetter
            // 
            this.label_CustomTuningLowEStringLetter.AutoSize = true;
            this.label_CustomTuningLowEStringLetter.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_CustomTuningLowEStringLetter.Location = new System.Drawing.Point(448, 41);
            this.label_CustomTuningLowEStringLetter.Name = "label_CustomTuningLowEStringLetter";
            this.label_CustomTuningLowEStringLetter.Size = new System.Drawing.Size(22, 13);
            this.label_CustomTuningLowEStringLetter.TabIndex = 103;
            this.label_CustomTuningLowEStringLetter.Text = "E2";
            // 
            // radio_DefaultBassTone
            // 
            this.radio_DefaultBassTone.AutoSize = true;
            this.radio_DefaultBassTone.Location = new System.Drawing.Point(398, 225);
            this.radio_DefaultBassTone.Name = "radio_DefaultBassTone";
            this.radio_DefaultBassTone.Size = new System.Drawing.Size(256, 17);
            this.radio_DefaultBassTone.TabIndex = 102;
            this.radio_DefaultBassTone.Text = "Set Highlighted Tone As New Default Bass Tone";
            this.radio_DefaultBassTone.UseVisualStyleBackColor = true;
            this.radio_DefaultBassTone.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.radio_DefaultBassTone.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_DefaultLeadTone
            // 
            this.radio_DefaultLeadTone.AutoSize = true;
            this.radio_DefaultLeadTone.Checked = true;
            this.radio_DefaultLeadTone.Location = new System.Drawing.Point(398, 179);
            this.radio_DefaultLeadTone.Name = "radio_DefaultLeadTone";
            this.radio_DefaultLeadTone.Size = new System.Drawing.Size(257, 17);
            this.radio_DefaultLeadTone.TabIndex = 101;
            this.radio_DefaultLeadTone.TabStop = true;
            this.radio_DefaultLeadTone.Text = "Set Highlighted Tone As New Default Lead Tone";
            this.radio_DefaultLeadTone.UseVisualStyleBackColor = true;
            this.radio_DefaultLeadTone.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.radio_DefaultLeadTone.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_DefaultRhythmTone
            // 
            this.radio_DefaultRhythmTone.AutoSize = true;
            this.radio_DefaultRhythmTone.Location = new System.Drawing.Point(398, 202);
            this.radio_DefaultRhythmTone.Name = "radio_DefaultRhythmTone";
            this.radio_DefaultRhythmTone.Size = new System.Drawing.Size(269, 17);
            this.radio_DefaultRhythmTone.TabIndex = 100;
            this.radio_DefaultRhythmTone.Text = "Set Highlighted Tone As New Default Rhythm Tone";
            this.radio_DefaultRhythmTone.UseVisualStyleBackColor = true;
            this.radio_DefaultRhythmTone.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.radio_DefaultRhythmTone.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_LoadTones
            // 
            this.button_LoadTones.Location = new System.Drawing.Point(5, 179);
            this.button_LoadTones.Name = "button_LoadTones";
            this.button_LoadTones.Size = new System.Drawing.Size(187, 35);
            this.button_LoadTones.TabIndex = 99;
            this.button_LoadTones.Text = "Load Tones From Profile";
            this.button_LoadTones.UseVisualStyleBackColor = true;
            this.button_LoadTones.Click += new System.EventHandler(this.SetForget_LoadTonesFromProfiles);
            this.button_LoadTones.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_LoadTones.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // listBox_ProfileTones
            // 
            this.listBox_ProfileTones.FormattingEnabled = true;
            this.listBox_ProfileTones.Location = new System.Drawing.Point(201, 179);
            this.listBox_ProfileTones.Name = "listBox_ProfileTones";
            this.listBox_ProfileTones.Size = new System.Drawing.Size(184, 134);
            this.listBox_ProfileTones.TabIndex = 98;
            this.listBox_ProfileTones.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.listBox_ProfileTones.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_AssignNewDefaultTone
            // 
            this.button_AssignNewDefaultTone.Location = new System.Drawing.Point(394, 276);
            this.button_AssignNewDefaultTone.Name = "button_AssignNewDefaultTone";
            this.button_AssignNewDefaultTone.Size = new System.Drawing.Size(187, 35);
            this.button_AssignNewDefaultTone.TabIndex = 96;
            this.button_AssignNewDefaultTone.Text = "Assign Tone As New Default";
            this.button_AssignNewDefaultTone.UseVisualStyleBackColor = true;
            this.button_AssignNewDefaultTone.Click += new System.EventHandler(this.SetForget_SetDefaultTones);
            this.button_AssignNewDefaultTone.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_AssignNewDefaultTone.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_RemoveTemp
            // 
            this.button_RemoveTemp.Location = new System.Drawing.Point(916, 135);
            this.button_RemoveTemp.Name = "button_RemoveTemp";
            this.button_RemoveTemp.Size = new System.Drawing.Size(190, 20);
            this.button_RemoveTemp.TabIndex = 95;
            this.button_RemoveTemp.Text = "Remove Temporary Folders";
            this.button_RemoveTemp.UseVisualStyleBackColor = true;
            this.button_RemoveTemp.Click += new System.EventHandler(this.SetForget_RemoveTempFolders);
            this.button_RemoveTemp.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_RemoveTemp.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_AddExitGame
            // 
            this.button_AddExitGame.Location = new System.Drawing.Point(723, 113);
            this.button_AddExitGame.Name = "button_AddExitGame";
            this.button_AddExitGame.Size = new System.Drawing.Size(187, 20);
            this.button_AddExitGame.TabIndex = 93;
            this.button_AddExitGame.Text = "Add EXIT GAME to Main Menu";
            this.button_AddExitGame.UseVisualStyleBackColor = true;
            this.button_AddExitGame.Click += new System.EventHandler(this.SetForget_AddCustomMenu);
            this.button_AddExitGame.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_AddExitGame.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_AddFastLoad
            // 
            this.button_AddFastLoad.Location = new System.Drawing.Point(723, 64);
            this.button_AddFastLoad.Name = "button_AddFastLoad";
            this.button_AddFastLoad.Size = new System.Drawing.Size(187, 40);
            this.button_AddFastLoad.TabIndex = 92;
            this.button_AddFastLoad.Text = "Enable Fast Load";
            this.button_AddFastLoad.UseVisualStyleBackColor = true;
            this.button_AddFastLoad.Click += new System.EventHandler(this.SetForget_AddFastLoadMod);
            this.button_AddFastLoad.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_AddFastLoad.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_CleanUpUnpackedCache
            // 
            this.button_CleanUpUnpackedCache.Location = new System.Drawing.Point(916, 113);
            this.button_CleanUpUnpackedCache.Name = "button_CleanUpUnpackedCache";
            this.button_CleanUpUnpackedCache.Size = new System.Drawing.Size(190, 20);
            this.button_CleanUpUnpackedCache.TabIndex = 91;
            this.button_CleanUpUnpackedCache.Text = "Clean Up Unpacked Cache";
            this.button_CleanUpUnpackedCache.UseVisualStyleBackColor = true;
            this.button_CleanUpUnpackedCache.Click += new System.EventHandler(this.SetForget_UnpackCacheAgain);
            this.button_CleanUpUnpackedCache.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_CleanUpUnpackedCache.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_RestoreCacheBackup
            // 
            this.button_RestoreCacheBackup.Location = new System.Drawing.Point(916, 64);
            this.button_RestoreCacheBackup.Name = "button_RestoreCacheBackup";
            this.button_RestoreCacheBackup.Size = new System.Drawing.Size(190, 40);
            this.button_RestoreCacheBackup.TabIndex = 90;
            this.button_RestoreCacheBackup.Text = "Restore Cache Backup";
            this.button_RestoreCacheBackup.UseVisualStyleBackColor = true;
            this.button_RestoreCacheBackup.Click += new System.EventHandler(this.SetForget_RestoreDefaults);
            this.button_RestoreCacheBackup.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_RestoreCacheBackup.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_AddCustomTunings
            // 
            this.button_AddCustomTunings.Location = new System.Drawing.Point(723, 14);
            this.button_AddCustomTunings.Name = "button_AddCustomTunings";
            this.button_AddCustomTunings.Size = new System.Drawing.Size(187, 40);
            this.button_AddCustomTunings.TabIndex = 89;
            this.button_AddCustomTunings.Text = "Add Custom Tunings";
            this.button_AddCustomTunings.UseVisualStyleBackColor = true;
            this.button_AddCustomTunings.Click += new System.EventHandler(this.SetForget_AddCustomTunings);
            this.button_AddCustomTunings.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_AddCustomTunings.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_SaveTuningChanges
            // 
            this.button_SaveTuningChanges.Location = new System.Drawing.Point(518, 102);
            this.button_SaveTuningChanges.Name = "button_SaveTuningChanges";
            this.button_SaveTuningChanges.Size = new System.Drawing.Size(85, 22);
            this.button_SaveTuningChanges.TabIndex = 88;
            this.button_SaveTuningChanges.Text = "Save Changes";
            this.button_SaveTuningChanges.UseVisualStyleBackColor = true;
            this.button_SaveTuningChanges.Click += new System.EventHandler(this.SetForget_SaveTuningChanges);
            this.button_SaveTuningChanges.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_SaveTuningChanges.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_CustomStringTunings
            // 
            this.label_CustomStringTunings.AutoSize = true;
            this.label_CustomStringTunings.Location = new System.Drawing.Point(390, 22);
            this.label_CustomStringTunings.Name = "label_CustomStringTunings";
            this.label_CustomStringTunings.Size = new System.Drawing.Size(42, 13);
            this.label_CustomStringTunings.TabIndex = 87;
            this.label_CustomStringTunings.Text = "Strings:";
            // 
            // nUpDown_String0
            // 
            this.nUpDown_String0.BackColor = System.Drawing.Color.Tomato;
            this.nUpDown_String0.ForeColor = System.Drawing.SystemColors.ControlText;
            this.nUpDown_String0.Location = new System.Drawing.Point(448, 20);
            this.nUpDown_String0.Maximum = new decimal(new int[] {
            24,
            0,
            0,
            0});
            this.nUpDown_String0.Minimum = new decimal(new int[] {
            24,
            0,
            0,
            -2147483648});
            this.nUpDown_String0.Name = "nUpDown_String0";
            this.nUpDown_String0.Size = new System.Drawing.Size(39, 20);
            this.nUpDown_String0.TabIndex = 86;
            this.nUpDown_String0.ValueChanged += new System.EventHandler(this.TuningOffsets);
            this.nUpDown_String0.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.nUpDown_String0.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_UIIndex
            // 
            this.label_UIIndex.AutoSize = true;
            this.label_UIIndex.Location = new System.Drawing.Point(197, 74);
            this.label_UIIndex.Name = "label_UIIndex";
            this.label_UIIndex.Size = new System.Drawing.Size(50, 13);
            this.label_UIIndex.TabIndex = 85;
            this.label_UIIndex.Text = "UI Index:";
            this.label_UIIndex.Visible = false;
            // 
            // nUpDown_UIIndex
            // 
            this.nUpDown_UIIndex.Location = new System.Drawing.Point(279, 72);
            this.nUpDown_UIIndex.Maximum = new decimal(new int[] {
            100000,
            0,
            0,
            0});
            this.nUpDown_UIIndex.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
            this.nUpDown_UIIndex.Name = "nUpDown_UIIndex";
            this.nUpDown_UIIndex.Size = new System.Drawing.Size(100, 20);
            this.nUpDown_UIIndex.TabIndex = 84;
            this.nUpDown_UIIndex.Value = new decimal(new int[] {
            37500,
            0,
            0,
            0});
            this.nUpDown_UIIndex.Visible = false;
            // 
            // label_UITuningName
            // 
            this.label_UITuningName.AutoSize = true;
            this.label_UITuningName.Location = new System.Drawing.Point(197, 48);
            this.label_UITuningName.Name = "label_UITuningName";
            this.label_UITuningName.Size = new System.Drawing.Size(52, 13);
            this.label_UITuningName.TabIndex = 83;
            this.label_UITuningName.Text = "UI Name:";
            // 
            // label_InternalTuningName
            // 
            this.label_InternalTuningName.AutoSize = true;
            this.label_InternalTuningName.Location = new System.Drawing.Point(197, 22);
            this.label_InternalTuningName.Name = "label_InternalTuningName";
            this.label_InternalTuningName.Size = new System.Drawing.Size(76, 13);
            this.label_InternalTuningName.TabIndex = 82;
            this.label_InternalTuningName.Text = "Internal Name:";
            // 
            // nUpDown_String1
            // 
            this.nUpDown_String1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            this.nUpDown_String1.Location = new System.Drawing.Point(493, 20);
            this.nUpDown_String1.Maximum = new decimal(new int[] {
            24,
            0,
            0,
            0});
            this.nUpDown_String1.Minimum = new decimal(new int[] {
            24,
            0,
            0,
            -2147483648});
            this.nUpDown_String1.Name = "nUpDown_String1";
            this.nUpDown_String1.Size = new System.Drawing.Size(39, 20);
            this.nUpDown_String1.TabIndex = 81;
            this.nUpDown_String1.ValueChanged += new System.EventHandler(this.TuningOffsets);
            this.nUpDown_String1.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.nUpDown_String1.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // nUpDown_String5
            // 
            this.nUpDown_String5.BackColor = System.Drawing.Color.MediumPurple;
            this.nUpDown_String5.Location = new System.Drawing.Point(673, 20);
            this.nUpDown_String5.Maximum = new decimal(new int[] {
            24,
            0,
            0,
            0});
            this.nUpDown_String5.Minimum = new decimal(new int[] {
            24,
            0,
            0,
            -2147483648});
            this.nUpDown_String5.Name = "nUpDown_String5";
            this.nUpDown_String5.Size = new System.Drawing.Size(39, 20);
            this.nUpDown_String5.TabIndex = 80;
            this.nUpDown_String5.ValueChanged += new System.EventHandler(this.TuningOffsets);
            this.nUpDown_String5.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.nUpDown_String5.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // nUpDown_String4
            // 
            this.nUpDown_String4.BackColor = System.Drawing.Color.GreenYellow;
            this.nUpDown_String4.Location = new System.Drawing.Point(628, 20);
            this.nUpDown_String4.Maximum = new decimal(new int[] {
            24,
            0,
            0,
            0});
            this.nUpDown_String4.Minimum = new decimal(new int[] {
            24,
            0,
            0,
            -2147483648});
            this.nUpDown_String4.Name = "nUpDown_String4";
            this.nUpDown_String4.Size = new System.Drawing.Size(39, 20);
            this.nUpDown_String4.TabIndex = 79;
            this.nUpDown_String4.ValueChanged += new System.EventHandler(this.TuningOffsets);
            this.nUpDown_String4.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.nUpDown_String4.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // nUpDown_String3
            // 
            this.nUpDown_String3.BackColor = System.Drawing.Color.DarkOrange;
            this.nUpDown_String3.Location = new System.Drawing.Point(583, 20);
            this.nUpDown_String3.Maximum = new decimal(new int[] {
            24,
            0,
            0,
            0});
            this.nUpDown_String3.Minimum = new decimal(new int[] {
            24,
            0,
            0,
            -2147483648});
            this.nUpDown_String3.Name = "nUpDown_String3";
            this.nUpDown_String3.Size = new System.Drawing.Size(39, 20);
            this.nUpDown_String3.TabIndex = 78;
            this.nUpDown_String3.ValueChanged += new System.EventHandler(this.TuningOffsets);
            this.nUpDown_String3.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.nUpDown_String3.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // nUpDown_String2
            // 
            this.nUpDown_String2.BackColor = System.Drawing.Color.DeepSkyBlue;
            this.nUpDown_String2.Location = new System.Drawing.Point(538, 20);
            this.nUpDown_String2.Maximum = new decimal(new int[] {
            24,
            0,
            0,
            0});
            this.nUpDown_String2.Minimum = new decimal(new int[] {
            24,
            0,
            0,
            -2147483648});
            this.nUpDown_String2.Name = "nUpDown_String2";
            this.nUpDown_String2.Size = new System.Drawing.Size(39, 20);
            this.nUpDown_String2.TabIndex = 77;
            this.nUpDown_String2.ValueChanged += new System.EventHandler(this.TuningOffsets);
            this.nUpDown_String2.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.nUpDown_String2.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // textBox_UIName
            // 
            this.textBox_UIName.Location = new System.Drawing.Point(279, 45);
            this.textBox_UIName.Name = "textBox_UIName";
            this.textBox_UIName.Size = new System.Drawing.Size(100, 20);
            this.textBox_UIName.TabIndex = 76;
            // 
            // textBox_InternalTuningName
            // 
            this.textBox_InternalTuningName.Location = new System.Drawing.Point(279, 19);
            this.textBox_InternalTuningName.Name = "textBox_InternalTuningName";
            this.textBox_InternalTuningName.Size = new System.Drawing.Size(100, 20);
            this.textBox_InternalTuningName.TabIndex = 75;
            // 
            // button_AddTuning
            // 
            this.button_AddTuning.Location = new System.Drawing.Point(455, 64);
            this.button_AddTuning.Name = "button_AddTuning";
            this.button_AddTuning.Size = new System.Drawing.Size(93, 23);
            this.button_AddTuning.TabIndex = 74;
            this.button_AddTuning.Text = "Add Tuning";
            this.button_AddTuning.UseVisualStyleBackColor = true;
            this.button_AddTuning.Click += new System.EventHandler(this.SetForget_AddTuning);
            this.button_AddTuning.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_AddTuning.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_RemoveTuning
            // 
            this.button_RemoveTuning.Location = new System.Drawing.Point(574, 64);
            this.button_RemoveTuning.Name = "button_RemoveTuning";
            this.button_RemoveTuning.Size = new System.Drawing.Size(93, 23);
            this.button_RemoveTuning.TabIndex = 73;
            this.button_RemoveTuning.Text = "Remove Tuning";
            this.button_RemoveTuning.UseVisualStyleBackColor = true;
            this.button_RemoveTuning.Click += new System.EventHandler(this.SetForget_RemoveTuning);
            this.button_RemoveTuning.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.button_RemoveTuning.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // listBox_Tunings
            // 
            this.listBox_Tunings.FormattingEnabled = true;
            this.listBox_Tunings.Location = new System.Drawing.Point(6, 19);
            this.listBox_Tunings.Name = "listBox_Tunings";
            this.listBox_Tunings.Size = new System.Drawing.Size(185, 134);
            this.listBox_Tunings.TabIndex = 72;
            this.listBox_Tunings.SelectedIndexChanged += new System.EventHandler(this.SetForget_ListTunings);
            this.listBox_Tunings.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.listBox_Tunings.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            this.ToolTip.AutomaticDelay = 0;
            this.ToolTip.ToolTipTitle = "ToolTip";
            // 
            // groupBox_ToggleSkylineWhen
            // 
            this.groupBox_ToggleSkylineWhen.Controls.Add(this.radio_SkylineOffInSong);
            this.groupBox_ToggleSkylineWhen.Controls.Add(this.radio_SkylineAlwaysOff);
            this.groupBox_ToggleSkylineWhen.Location = new System.Drawing.Point(511, 91);
            this.groupBox_ToggleSkylineWhen.Name = "groupBox_ToggleSkylineWhen";
            this.groupBox_ToggleSkylineWhen.Size = new System.Drawing.Size(145, 68);
            this.groupBox_ToggleSkylineWhen.TabIndex = 69;
            this.groupBox_ToggleSkylineWhen.TabStop = false;
            this.groupBox_ToggleSkylineWhen.Text = "Toggle Skyline Off When:";
            this.groupBox_ToggleSkylineWhen.Visible = false;
            this.groupBox_ToggleSkylineWhen.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.groupBox_ToggleSkylineWhen.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_SkylineOffInSong
            // 
            this.radio_SkylineOffInSong.AutoSize = true;
            this.radio_SkylineOffInSong.Location = new System.Drawing.Point(11, 40);
            this.radio_SkylineOffInSong.Name = "radio_SkylineOffInSong";
            this.radio_SkylineOffInSong.Size = new System.Drawing.Size(116, 17);
            this.radio_SkylineOffInSong.TabIndex = 46;
            this.radio_SkylineOffInSong.TabStop = true;
            this.radio_SkylineOffInSong.Text = "Skyline Off In Song";
            this.radio_SkylineOffInSong.UseVisualStyleBackColor = true;
            this.radio_SkylineOffInSong.CheckedChanged += new System.EventHandler(this.Save_ToggleSkylineSong);
            // 
            // radio_SkylineAlwaysOff
            // 
            this.radio_SkylineAlwaysOff.AutoSize = true;
            this.radio_SkylineAlwaysOff.Location = new System.Drawing.Point(11, 19);
            this.radio_SkylineAlwaysOff.Name = "radio_SkylineAlwaysOff";
            this.radio_SkylineAlwaysOff.Size = new System.Drawing.Size(112, 17);
            this.radio_SkylineAlwaysOff.TabIndex = 44;
            this.radio_SkylineAlwaysOff.TabStop = true;
            this.radio_SkylineAlwaysOff.Text = "Skyline Always Off";
            this.radio_SkylineAlwaysOff.UseVisualStyleBackColor = true;
            this.radio_SkylineAlwaysOff.CheckedChanged += new System.EventHandler(this.Save_ToggleSkylineStartup);
            // 
            // groupBox_ToggleLyricsOffWhen
            // 
            this.groupBox_ToggleLyricsOffWhen.Controls.Add(this.radio_LyricsOffHotkey);
            this.groupBox_ToggleLyricsOffWhen.Controls.Add(this.radio_LyricsAlwaysOff);
            this.groupBox_ToggleLyricsOffWhen.Location = new System.Drawing.Point(511, 6);
            this.groupBox_ToggleLyricsOffWhen.Name = "groupBox_ToggleLyricsOffWhen";
            this.groupBox_ToggleLyricsOffWhen.Size = new System.Drawing.Size(149, 68);
            this.groupBox_ToggleLyricsOffWhen.TabIndex = 70;
            this.groupBox_ToggleLyricsOffWhen.TabStop = false;
            this.groupBox_ToggleLyricsOffWhen.Text = "Toggle Lyrics Off When:";
            this.groupBox_ToggleLyricsOffWhen.Visible = false;
            // 
            // radio_LyricsOffHotkey
            // 
            this.radio_LyricsOffHotkey.AutoSize = true;
            this.radio_LyricsOffHotkey.Location = new System.Drawing.Point(11, 42);
            this.radio_LyricsOffHotkey.Name = "radio_LyricsOffHotkey";
            this.radio_LyricsOffHotkey.Size = new System.Drawing.Size(124, 17);
            this.radio_LyricsOffHotkey.TabIndex = 46;
            this.radio_LyricsOffHotkey.TabStop = true;
            this.radio_LyricsOffHotkey.Text = "Lyrics Off Via Hotkey";
            this.radio_LyricsOffHotkey.UseVisualStyleBackColor = true;
            this.radio_LyricsOffHotkey.CheckedChanged += new System.EventHandler(this.Save_ToggleLyricsManual);
            this.radio_LyricsOffHotkey.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.radio_LyricsOffHotkey.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_LyricsAlwaysOff
            // 
            this.radio_LyricsAlwaysOff.AutoSize = true;
            this.radio_LyricsAlwaysOff.Location = new System.Drawing.Point(11, 19);
            this.radio_LyricsAlwaysOff.Name = "radio_LyricsAlwaysOff";
            this.radio_LyricsAlwaysOff.Size = new System.Drawing.Size(105, 17);
            this.radio_LyricsAlwaysOff.TabIndex = 44;
            this.radio_LyricsAlwaysOff.TabStop = true;
            this.radio_LyricsAlwaysOff.Text = "Lyrics Always Off";
            this.radio_LyricsAlwaysOff.UseVisualStyleBackColor = true;
            this.radio_LyricsAlwaysOff.CheckedChanged += new System.EventHandler(this.Save_ToggleLyricsStartup);
            this.radio_LyricsAlwaysOff.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.radio_LyricsAlwaysOff.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_GuitarSpeak
            // 
            this.groupBox_GuitarSpeak.Controls.Add(this.button_GuitarSpeak_DeleteSavedValue);
            this.groupBox_GuitarSpeak.Controls.Add(this.button_GuitarSpeakHelp);
            this.groupBox_GuitarSpeak.Controls.Add(this.label_GuitarSpeakSaved);
            this.groupBox_GuitarSpeak.Controls.Add(this.listBox_GuitarSpeakSaved);
            this.groupBox_GuitarSpeak.Controls.Add(this.checkbox_GuitarSpeakWhileTuning);
            this.groupBox_GuitarSpeak.Controls.Add(this.button_GuitarSpeakSave);
            this.groupBox_GuitarSpeak.Controls.Add(this.label_GuitarSpeakKeypress);
            this.groupBox_GuitarSpeak.Controls.Add(this.label_GuitarSpeakOctave);
            this.groupBox_GuitarSpeak.Controls.Add(this.label_GuitarSpeakNote);
            this.groupBox_GuitarSpeak.Controls.Add(this.listBox_GuitarSpeakKeypress);
            this.groupBox_GuitarSpeak.Controls.Add(this.listBox_GuitarSpeakOctave);
            this.groupBox_GuitarSpeak.Controls.Add(this.listBox_GuitarSpeakNote);
            this.groupBox_GuitarSpeak.Location = new System.Drawing.Point(12, 18);
            this.groupBox_GuitarSpeak.Name = "groupBox_GuitarSpeak";
            this.groupBox_GuitarSpeak.Size = new System.Drawing.Size(432, 206);
            this.groupBox_GuitarSpeak.TabIndex = 71;
            this.groupBox_GuitarSpeak.TabStop = false;
            this.groupBox_GuitarSpeak.Text = "Guitar Speak";
            this.groupBox_GuitarSpeak.Visible = false;
            // 
            // button_GuitarSpeak_DeleteSavedValue
            // 
            this.button_GuitarSpeak_DeleteSavedValue.Location = new System.Drawing.Point(311, 147);
            this.button_GuitarSpeak_DeleteSavedValue.Name = "button_GuitarSpeak_DeleteSavedValue";
            this.button_GuitarSpeak_DeleteSavedValue.Size = new System.Drawing.Size(115, 23);
            this.button_GuitarSpeak_DeleteSavedValue.TabIndex = 13;
            this.button_GuitarSpeak_DeleteSavedValue.Text = "Delete Saved Value";
            this.button_GuitarSpeak_DeleteSavedValue.UseVisualStyleBackColor = true;
            this.button_GuitarSpeak_DeleteSavedValue.Click += new System.EventHandler(this.GuitarSpeak_ClearSavedValue);
            // 
            // button_GuitarSpeakHelp
            // 
            this.button_GuitarSpeakHelp.BackColor = System.Drawing.Color.Azure;
            this.button_GuitarSpeakHelp.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button_GuitarSpeakHelp.Location = new System.Drawing.Point(402, 11);
            this.button_GuitarSpeakHelp.Name = "button_GuitarSpeakHelp";
            this.button_GuitarSpeakHelp.Size = new System.Drawing.Size(24, 23);
            this.button_GuitarSpeakHelp.TabIndex = 12;
            this.button_GuitarSpeakHelp.Text = "?";
            this.button_GuitarSpeakHelp.UseVisualStyleBackColor = true;
            this.button_GuitarSpeakHelp.Click += new System.EventHandler(this.GuitarSpeakHelpButton_Click);
            // 
            // label_GuitarSpeakSaved
            // 
            this.label_GuitarSpeakSaved.AutoSize = true;
            this.label_GuitarSpeakSaved.Location = new System.Drawing.Point(314, 26);
            this.label_GuitarSpeakSaved.Name = "label_GuitarSpeakSaved";
            this.label_GuitarSpeakSaved.Size = new System.Drawing.Size(73, 13);
            this.label_GuitarSpeakSaved.TabIndex = 11;
            this.label_GuitarSpeakSaved.Text = "Saved Values";
            // 
            // listBox_GuitarSpeakSaved
            // 
            this.listBox_GuitarSpeakSaved.FormattingEnabled = true;
            this.listBox_GuitarSpeakSaved.Location = new System.Drawing.Point(282, 48);
            this.listBox_GuitarSpeakSaved.Name = "listBox_GuitarSpeakSaved";
            this.listBox_GuitarSpeakSaved.Size = new System.Drawing.Size(144, 82);
            this.listBox_GuitarSpeakSaved.TabIndex = 10;
            // 
            // checkbox_GuitarSpeakWhileTuning
            // 
            this.checkbox_GuitarSpeakWhileTuning.AutoSize = true;
            this.checkbox_GuitarSpeakWhileTuning.Location = new System.Drawing.Point(111, 151);
            this.checkbox_GuitarSpeakWhileTuning.Name = "checkbox_GuitarSpeakWhileTuning";
            this.checkbox_GuitarSpeakWhileTuning.Size = new System.Drawing.Size(111, 17);
            this.checkbox_GuitarSpeakWhileTuning.TabIndex = 9;
            this.checkbox_GuitarSpeakWhileTuning.Text = "Use While Tuning";
            this.checkbox_GuitarSpeakWhileTuning.UseVisualStyleBackColor = true;
            this.checkbox_GuitarSpeakWhileTuning.CheckedChanged += new System.EventHandler(this.GuitarSpeakWhileTuningBox_CheckedChanged);
            this.checkbox_GuitarSpeakWhileTuning.MouseLeave += new System.EventHandler(this.HideToolTips);
            this.checkbox_GuitarSpeakWhileTuning.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_GuitarSpeakSave
            // 
            this.button_GuitarSpeakSave.Location = new System.Drawing.Point(6, 147);
            this.button_GuitarSpeakSave.Name = "button_GuitarSpeakSave";
            this.button_GuitarSpeakSave.Size = new System.Drawing.Size(95, 23);
            this.button_GuitarSpeakSave.TabIndex = 8;
            this.button_GuitarSpeakSave.Text = "Save Keybind";
            this.button_GuitarSpeakSave.UseVisualStyleBackColor = true;
            this.button_GuitarSpeakSave.Click += new System.EventHandler(this.GuitarSpeakSaveButton_Click);
            // 
            // label_GuitarSpeakKeypress
            // 
            this.label_GuitarSpeakKeypress.AutoSize = true;
            this.label_GuitarSpeakKeypress.Location = new System.Drawing.Point(182, 21);
            this.label_GuitarSpeakKeypress.Name = "label_GuitarSpeakKeypress";
            this.label_GuitarSpeakKeypress.Size = new System.Drawing.Size(50, 13);
            this.label_GuitarSpeakKeypress.TabIndex = 5;
            this.label_GuitarSpeakKeypress.Text = "Keypress";
            // 
            // label_GuitarSpeakOctave
            // 
            this.label_GuitarSpeakOctave.AutoSize = true;
            this.label_GuitarSpeakOctave.Location = new System.Drawing.Point(79, 21);
            this.label_GuitarSpeakOctave.Name = "label_GuitarSpeakOctave";
            this.label_GuitarSpeakOctave.Size = new System.Drawing.Size(42, 13);
            this.label_GuitarSpeakOctave.TabIndex = 4;
            this.label_GuitarSpeakOctave.Text = "Octave";
            // 
            // label_GuitarSpeakNote
            // 
            this.label_GuitarSpeakNote.AutoSize = true;
            this.label_GuitarSpeakNote.Location = new System.Drawing.Point(11, 22);
            this.label_GuitarSpeakNote.Name = "label_GuitarSpeakNote";
            this.label_GuitarSpeakNote.Size = new System.Drawing.Size(30, 13);
            this.label_GuitarSpeakNote.TabIndex = 3;
            this.label_GuitarSpeakNote.Text = "Note";
            // 
            // listBox_GuitarSpeakKeypress
            // 
            this.listBox_GuitarSpeakKeypress.FormattingEnabled = true;
            this.listBox_GuitarSpeakKeypress.Items.AddRange(new object[] {
            "Delete",
            "Space",
            "Enter",
            "Tab",
            "Page Up",
            "Page Down",
            "Up Arrow",
            "Down Arrow",
            "Escape",
            "Open Bracket",
            "Close Bracket",
            "Tilde / Tilda",
            "Forward Slash",
            "Close Guitar Speak"});
            this.listBox_GuitarSpeakKeypress.Location = new System.Drawing.Point(150, 48);
            this.listBox_GuitarSpeakKeypress.Name = "listBox_GuitarSpeakKeypress";
            this.listBox_GuitarSpeakKeypress.Size = new System.Drawing.Size(126, 82);
            this.listBox_GuitarSpeakKeypress.TabIndex = 2;
            // 
            // listBox_GuitarSpeakOctave
            // 
            this.listBox_GuitarSpeakOctave.FormattingEnabled = true;
            this.listBox_GuitarSpeakOctave.Items.AddRange(new object[] {
            "-1",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6"});
            this.listBox_GuitarSpeakOctave.Location = new System.Drawing.Point(78, 47);
            this.listBox_GuitarSpeakOctave.Name = "listBox_GuitarSpeakOctave";
            this.listBox_GuitarSpeakOctave.Size = new System.Drawing.Size(46, 82);
            this.listBox_GuitarSpeakOctave.TabIndex = 1;
            // 
            // listBox_GuitarSpeakNote
            // 
            this.listBox_GuitarSpeakNote.FormattingEnabled = true;
            this.listBox_GuitarSpeakNote.Items.AddRange(new object[] {
            "C",
            "C#",
            "D",
            "Eb",
            "E",
            "F",
            "F#",
            "G",
            "Ab",
            "A",
            "Bb",
            "B"});
            this.listBox_GuitarSpeakNote.Location = new System.Drawing.Point(6, 48);
            this.listBox_GuitarSpeakNote.Name = "listBox_GuitarSpeakNote";
            this.listBox_GuitarSpeakNote.Size = new System.Drawing.Size(46, 82);
            this.listBox_GuitarSpeakNote.TabIndex = 0;
            // 
            // label_Credits
            // 
            this.label_Credits.AutoSize = true;
            this.label_Credits.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_Credits.Location = new System.Drawing.Point(989, 455);
            this.label_Credits.Name = "label_Credits";
            this.label_Credits.Size = new System.Drawing.Size(152, 52);
            this.label_Credits.TabIndex = 72;
            this.label_Credits.Text = "Originally Created By:\r\nFfio - Main GUI Dev\r\nLovroM8 - Set-and-Forget\r\nZagatoZee " +
    "- Co-GUI Dev";
            this.label_Credits.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // groupBox_ToggleHeadstockOffWhen
            // 
            this.groupBox_ToggleHeadstockOffWhen.Controls.Add(this.radio_HeadstockOffInSong);
            this.groupBox_ToggleHeadstockOffWhen.Controls.Add(this.radio_HeadstockAlwaysOff);
            this.groupBox_ToggleHeadstockOffWhen.Location = new System.Drawing.Point(511, 266);
            this.groupBox_ToggleHeadstockOffWhen.Name = "groupBox_ToggleHeadstockOffWhen";
            this.groupBox_ToggleHeadstockOffWhen.Size = new System.Drawing.Size(161, 77);
            this.groupBox_ToggleHeadstockOffWhen.TabIndex = 73;
            this.groupBox_ToggleHeadstockOffWhen.TabStop = false;
            this.groupBox_ToggleHeadstockOffWhen.Text = "Toggle Headstock Off When:";
            this.groupBox_ToggleHeadstockOffWhen.Visible = false;
            // 
            // radio_HeadstockOffInSong
            // 
            this.radio_HeadstockOffInSong.AutoSize = true;
            this.radio_HeadstockOffInSong.Location = new System.Drawing.Point(11, 50);
            this.radio_HeadstockOffInSong.Name = "radio_HeadstockOffInSong";
            this.radio_HeadstockOffInSong.Size = new System.Drawing.Size(134, 17);
            this.radio_HeadstockOffInSong.TabIndex = 1;
            this.radio_HeadstockOffInSong.TabStop = true;
            this.radio_HeadstockOffInSong.Text = "Headstock Off In Song";
            this.radio_HeadstockOffInSong.UseVisualStyleBackColor = true;
            this.radio_HeadstockOffInSong.CheckedChanged += new System.EventHandler(this.Save_HeadstockOffInSongOnlyButton);
            // 
            // radio_HeadstockAlwaysOff
            // 
            this.radio_HeadstockAlwaysOff.AutoSize = true;
            this.radio_HeadstockAlwaysOff.Location = new System.Drawing.Point(11, 27);
            this.radio_HeadstockAlwaysOff.Name = "radio_HeadstockAlwaysOff";
            this.radio_HeadstockAlwaysOff.Size = new System.Drawing.Size(130, 17);
            this.radio_HeadstockAlwaysOff.TabIndex = 0;
            this.radio_HeadstockAlwaysOff.TabStop = true;
            this.radio_HeadstockAlwaysOff.Text = "Headstock Always Off";
            this.radio_HeadstockAlwaysOff.UseVisualStyleBackColor = true;
            this.radio_HeadstockAlwaysOff.CheckedChanged += new System.EventHandler(this.Save_HeadStockAlwaysOffButton);
            // 
            // TabController
            // 
            this.TabController.Controls.Add(this.tab_Songlists);
            this.TabController.Controls.Add(this.tab_Keybindings);
            this.TabController.Controls.Add(this.tab_ModToggles);
            this.TabController.Controls.Add(this.tab_SetAndForget);
            this.TabController.Controls.Add(this.tab_ModSettings);
            this.TabController.Controls.Add(this.tab_RSASIO);
            this.TabController.Controls.Add(this.tab_Rocksmith);
            this.TabController.Controls.Add(this.tab_Twitch);
            this.TabController.Controls.Add(this.tab_GUISettings);
            this.TabController.Location = new System.Drawing.Point(3, 7);
            this.TabController.Margin = new System.Windows.Forms.Padding(0);
            this.TabController.Name = "TabController";
            this.TabController.Padding = new System.Drawing.Point(0, 0);
            this.TabController.SelectedIndex = 0;
            this.TabController.Size = new System.Drawing.Size(1158, 539);
            this.TabController.TabIndex = 100001;
            // 
            // tab_Songlists
            // 
            this.tab_Songlists.BackColor = System.Drawing.Color.Azure;
            this.tab_Songlists.Controls.Add(this.groupBox_Songlist);
            this.tab_Songlists.Location = new System.Drawing.Point(4, 22);
            this.tab_Songlists.Name = "tab_Songlists";
            this.tab_Songlists.Padding = new System.Windows.Forms.Padding(3);
            this.tab_Songlists.Size = new System.Drawing.Size(1150, 513);
            this.tab_Songlists.TabIndex = 0;
            this.tab_Songlists.Text = "Song Lists";
            // 
            // tab_Keybindings
            // 
            this.tab_Keybindings.BackColor = System.Drawing.Color.Azure;
            this.tab_Keybindings.Controls.Add(this.groupBox_Keybindings_AUDIO);
            this.tab_Keybindings.Controls.Add(this.groupBox_Keybindings_MODS);
            this.tab_Keybindings.Location = new System.Drawing.Point(4, 22);
            this.tab_Keybindings.Name = "tab_Keybindings";
            this.tab_Keybindings.Padding = new System.Windows.Forms.Padding(3);
            this.tab_Keybindings.Size = new System.Drawing.Size(1150, 513);
            this.tab_Keybindings.TabIndex = 1;
            this.tab_Keybindings.Text = "Keybindings";
            // 
            // groupBox_Keybindings_AUDIO
            // 
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_ChangeSelectedVolumeKey);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_SFXVolumeKey);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_VoiceOverVolumeKey);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_MicrophoneVolumeKey);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_Player2VolumeKey);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_Player1VolumeKey);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_SongVolumeKey);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_MasterVolumeKey);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.button_ClearSelectedKeybind_AUDIO);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.label_NewAssignmentAUDIO);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.textBox_NewKeyAssignment_AUDIO);
            this.groupBox_Keybindings_AUDIO.Controls.Add(this.listBox_Modlist_AUDIO);
            this.groupBox_Keybindings_AUDIO.Location = new System.Drawing.Point(15, 214);
            this.groupBox_Keybindings_AUDIO.Name = "groupBox_Keybindings_AUDIO";
            this.groupBox_Keybindings_AUDIO.Size = new System.Drawing.Size(701, 222);
            this.groupBox_Keybindings_AUDIO.TabIndex = 53;
            this.groupBox_Keybindings_AUDIO.TabStop = false;
            this.groupBox_Keybindings_AUDIO.Text = "Audio Key Bindings";
            this.groupBox_Keybindings_AUDIO.Visible = false;
            this.groupBox_Keybindings_AUDIO.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_ChangeSelectedVolumeKey
            // 
            this.label_ChangeSelectedVolumeKey.AutoSize = true;
            this.label_ChangeSelectedVolumeKey.Location = new System.Drawing.Point(463, 195);
            this.label_ChangeSelectedVolumeKey.Name = "label_ChangeSelectedVolumeKey";
            this.label_ChangeSelectedVolumeKey.Size = new System.Drawing.Size(132, 13);
            this.label_ChangeSelectedVolumeKey.TabIndex = 61;
            this.label_ChangeSelectedVolumeKey.Text = "Show Volume On Screen: ";
            // 
            // label_SFXVolumeKey
            // 
            this.label_SFXVolumeKey.AutoSize = true;
            this.label_SFXVolumeKey.Location = new System.Drawing.Point(464, 169);
            this.label_SFXVolumeKey.Name = "label_SFXVolumeKey";
            this.label_SFXVolumeKey.Size = new System.Drawing.Size(71, 13);
            this.label_SFXVolumeKey.TabIndex = 60;
            this.label_SFXVolumeKey.Text = "SFX Volume: ";
            // 
            // label_VoiceOverVolumeKey
            // 
            this.label_VoiceOverVolumeKey.AutoSize = true;
            this.label_VoiceOverVolumeKey.Location = new System.Drawing.Point(464, 143);
            this.label_VoiceOverVolumeKey.Name = "label_VoiceOverVolumeKey";
            this.label_VoiceOverVolumeKey.Size = new System.Drawing.Size(104, 13);
            this.label_VoiceOverVolumeKey.TabIndex = 59;
            this.label_VoiceOverVolumeKey.Text = "Voice-Over Volume: ";
            // 
            // label_MicrophoneVolumeKey
            // 
            this.label_MicrophoneVolumeKey.AutoSize = true;
            this.label_MicrophoneVolumeKey.Location = new System.Drawing.Point(464, 115);
            this.label_MicrophoneVolumeKey.Name = "label_MicrophoneVolumeKey";
            this.label_MicrophoneVolumeKey.Size = new System.Drawing.Size(107, 13);
            this.label_MicrophoneVolumeKey.TabIndex = 58;
            this.label_MicrophoneVolumeKey.Text = "Microphone Volume: ";
            // 
            // label_Player2VolumeKey
            // 
            this.label_Player2VolumeKey.AutoSize = true;
            this.label_Player2VolumeKey.Location = new System.Drawing.Point(464, 88);
            this.label_Player2VolumeKey.Name = "label_Player2VolumeKey";
            this.label_Player2VolumeKey.Size = new System.Drawing.Size(86, 13);
            this.label_Player2VolumeKey.TabIndex = 57;
            this.label_Player2VolumeKey.Text = "Player2 Volume: ";
            // 
            // label_Player1VolumeKey
            // 
            this.label_Player1VolumeKey.AutoSize = true;
            this.label_Player1VolumeKey.Location = new System.Drawing.Point(464, 62);
            this.label_Player1VolumeKey.Name = "label_Player1VolumeKey";
            this.label_Player1VolumeKey.Size = new System.Drawing.Size(86, 13);
            this.label_Player1VolumeKey.TabIndex = 56;
            this.label_Player1VolumeKey.Text = "Player1 Volume: ";
            // 
            // label_SongVolumeKey
            // 
            this.label_SongVolumeKey.AutoSize = true;
            this.label_SongVolumeKey.Location = new System.Drawing.Point(464, 39);
            this.label_SongVolumeKey.Name = "label_SongVolumeKey";
            this.label_SongVolumeKey.Size = new System.Drawing.Size(76, 13);
            this.label_SongVolumeKey.TabIndex = 55;
            this.label_SongVolumeKey.Text = "Song Volume: ";
            // 
            // label_MasterVolumeKey
            // 
            this.label_MasterVolumeKey.AutoSize = true;
            this.label_MasterVolumeKey.Location = new System.Drawing.Point(464, 16);
            this.label_MasterVolumeKey.Name = "label_MasterVolumeKey";
            this.label_MasterVolumeKey.Size = new System.Drawing.Size(83, 13);
            this.label_MasterVolumeKey.TabIndex = 54;
            this.label_MasterVolumeKey.Text = "Master Volume: ";
            // 
            // button_ClearSelectedKeybind_AUDIO
            // 
            this.button_ClearSelectedKeybind_AUDIO.Location = new System.Drawing.Point(222, 130);
            this.button_ClearSelectedKeybind_AUDIO.Name = "button_ClearSelectedKeybind_AUDIO";
            this.button_ClearSelectedKeybind_AUDIO.Size = new System.Drawing.Size(188, 23);
            this.button_ClearSelectedKeybind_AUDIO.TabIndex = 29;
            this.button_ClearSelectedKeybind_AUDIO.Text = "Clear Selected Keybind";
            this.button_ClearSelectedKeybind_AUDIO.UseVisualStyleBackColor = true;
            this.button_ClearSelectedKeybind_AUDIO.Click += new System.EventHandler(this.Delete_Keybind_AUDIO);
            // 
            // label_NewAssignmentAUDIO
            // 
            this.label_NewAssignmentAUDIO.AutoSize = true;
            this.label_NewAssignmentAUDIO.Location = new System.Drawing.Point(219, 59);
            this.label_NewAssignmentAUDIO.Name = "label_NewAssignmentAUDIO";
            this.label_NewAssignmentAUDIO.Size = new System.Drawing.Size(196, 13);
            this.label_NewAssignmentAUDIO.TabIndex = 21;
            this.label_NewAssignmentAUDIO.Text = "New Assignment (Press Enter To Save):";
            // 
            // textBox_NewKeyAssignment_AUDIO
            // 
            this.textBox_NewKeyAssignment_AUDIO.Location = new System.Drawing.Point(222, 93);
            this.textBox_NewKeyAssignment_AUDIO.MaxLength = 1;
            this.textBox_NewKeyAssignment_AUDIO.Name = "textBox_NewKeyAssignment_AUDIO";
            this.textBox_NewKeyAssignment_AUDIO.Size = new System.Drawing.Size(188, 20);
            this.textBox_NewKeyAssignment_AUDIO.TabIndex = 20;
            this.textBox_NewKeyAssignment_AUDIO.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.textBox_NewKeyAssignment_AUDIO.KeyDown += new System.Windows.Forms.KeyEventHandler(this.CheckKeyPressesDown);
            this.textBox_NewKeyAssignment_AUDIO.KeyUp += new System.Windows.Forms.KeyEventHandler(this.CheckKeyPressesUp);
            this.textBox_NewKeyAssignment_AUDIO.MouseDown += new System.Windows.Forms.MouseEventHandler(this.CheckMouseInput);
            // 
            // listBox_Modlist_AUDIO
            // 
            this.listBox_Modlist_AUDIO.FormattingEnabled = true;
            this.listBox_Modlist_AUDIO.Location = new System.Drawing.Point(22, 29);
            this.listBox_Modlist_AUDIO.Name = "listBox_Modlist_AUDIO";
            this.listBox_Modlist_AUDIO.Size = new System.Drawing.Size(185, 134);
            this.listBox_Modlist_AUDIO.TabIndex = 5;
            this.listBox_Modlist_AUDIO.SelectedIndexChanged += new System.EventHandler(this.LoadPreviousVolumeAssignment);
            // 
            // tab_ModToggles
            // 
            this.tab_ModToggles.BackColor = System.Drawing.Color.Azure;
            this.tab_ModToggles.Controls.Add(this.groupBox_EnabledMods);
            this.tab_ModToggles.Location = new System.Drawing.Point(4, 22);
            this.tab_ModToggles.Name = "tab_ModToggles";
            this.tab_ModToggles.Padding = new System.Windows.Forms.Padding(3);
            this.tab_ModToggles.Size = new System.Drawing.Size(1150, 513);
            this.tab_ModToggles.TabIndex = 2;
            this.tab_ModToggles.Text = "Enable / Disable Mods";
            // 
            // tab_SetAndForget
            // 
            this.tab_SetAndForget.Controls.Add(this.groupBox_SetAndForget);
            this.tab_SetAndForget.Location = new System.Drawing.Point(4, 22);
            this.tab_SetAndForget.Name = "tab_SetAndForget";
            this.tab_SetAndForget.Padding = new System.Windows.Forms.Padding(3);
            this.tab_SetAndForget.Size = new System.Drawing.Size(1150, 513);
            this.tab_SetAndForget.TabIndex = 3;
            this.tab_SetAndForget.Text = "Set And Forget Mods";
            this.tab_SetAndForget.UseVisualStyleBackColor = true;
            // 
            // tab_ModSettings
            // 
            this.tab_ModSettings.BackColor = System.Drawing.Color.Azure;
            this.tab_ModSettings.Controls.Add(this.TabController_ModSettings);
            this.tab_ModSettings.Controls.Add(this.button_ResetModsToDefault);
            this.tab_ModSettings.Location = new System.Drawing.Point(4, 22);
            this.tab_ModSettings.Name = "tab_ModSettings";
            this.tab_ModSettings.Padding = new System.Windows.Forms.Padding(3);
            this.tab_ModSettings.Size = new System.Drawing.Size(1150, 513);
            this.tab_ModSettings.TabIndex = 4;
            this.tab_ModSettings.Text = "Mod Settings";
            // 
            // TabController_ModSettings
            // 
            this.TabController_ModSettings.Controls.Add(this.tabPage_ModSettings_ER);
            this.TabController_ModSettings.Controls.Add(this.tabPage_ModSettings_Automation);
            this.TabController_ModSettings.Controls.Add(this.tabPage_ModSettings_Misc);
            this.TabController_ModSettings.Location = new System.Drawing.Point(6, 6);
            this.TabController_ModSettings.Name = "TabController_ModSettings";
            this.TabController_ModSettings.SelectedIndex = 0;
            this.TabController_ModSettings.Size = new System.Drawing.Size(1130, 473);
            this.TabController_ModSettings.TabIndex = 100007;
            // 
            // tabPage_ModSettings_ER
            // 
            this.tabPage_ModSettings_ER.BackColor = System.Drawing.Color.Azure;
            this.tabPage_ModSettings_ER.Controls.Add(this.groupBox_ExtendedRangeWhen);
            this.tabPage_ModSettings_ER.Controls.Add(this.groupBox_StringColors);
            this.tabPage_ModSettings_ER.Location = new System.Drawing.Point(4, 22);
            this.tabPage_ModSettings_ER.Name = "tabPage_ModSettings_ER";
            this.tabPage_ModSettings_ER.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage_ModSettings_ER.Size = new System.Drawing.Size(1122, 447);
            this.tabPage_ModSettings_ER.TabIndex = 0;
            this.tabPage_ModSettings_ER.Text = "Extended Range";
            // 
            // tabPage_ModSettings_Automation
            // 
            this.tabPage_ModSettings_Automation.BackColor = System.Drawing.Color.Azure;
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_GuitarSpeak);
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_Backups);
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_ToggleLyricsOffWhen);
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_HowToEnumerate);
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_AutoLoadProfiles);
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_LoftOffWhen);
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_MidiAutoTuneDevice);
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_ToggleHeadstockOffWhen);
            this.tabPage_ModSettings_Automation.Controls.Add(this.groupBox_ToggleSkylineWhen);
            this.tabPage_ModSettings_Automation.Location = new System.Drawing.Point(4, 22);
            this.tabPage_ModSettings_Automation.Name = "tabPage_ModSettings_Automation";
            this.tabPage_ModSettings_Automation.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage_ModSettings_Automation.Size = new System.Drawing.Size(1122, 447);
            this.tabPage_ModSettings_Automation.TabIndex = 1;
            this.tabPage_ModSettings_Automation.Text = "Automation";
            // 
            // groupBox_Backups
            // 
            this.groupBox_Backups.Controls.Add(this.checkBox_UnlimitedBackups);
            this.groupBox_Backups.Controls.Add(this.nUpDown_NumberOfBackups);
            this.groupBox_Backups.Location = new System.Drawing.Point(511, 355);
            this.groupBox_Backups.Name = "groupBox_Backups";
            this.groupBox_Backups.Size = new System.Drawing.Size(128, 86);
            this.groupBox_Backups.TabIndex = 100006;
            this.groupBox_Backups.TabStop = false;
            this.groupBox_Backups.Text = "How Many Backups";
            this.groupBox_Backups.Visible = false;
            this.groupBox_Backups.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_UnlimitedBackups
            // 
            this.checkBox_UnlimitedBackups.AutoSize = true;
            this.checkBox_UnlimitedBackups.Location = new System.Drawing.Point(8, 62);
            this.checkBox_UnlimitedBackups.Name = "checkBox_UnlimitedBackups";
            this.checkBox_UnlimitedBackups.Size = new System.Drawing.Size(114, 17);
            this.checkBox_UnlimitedBackups.TabIndex = 1;
            this.checkBox_UnlimitedBackups.Text = "Unlimited Backups";
            this.checkBox_UnlimitedBackups.UseVisualStyleBackColor = true;
            this.checkBox_UnlimitedBackups.CheckedChanged += new System.EventHandler(this.UnlimitedBackups);
            this.checkBox_UnlimitedBackups.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // nUpDown_NumberOfBackups
            // 
            this.nUpDown_NumberOfBackups.Location = new System.Drawing.Point(11, 23);
            this.nUpDown_NumberOfBackups.Maximum = new decimal(new int[] {
            100000,
            0,
            0,
            0});
            this.nUpDown_NumberOfBackups.Name = "nUpDown_NumberOfBackups";
            this.nUpDown_NumberOfBackups.Size = new System.Drawing.Size(104, 20);
            this.nUpDown_NumberOfBackups.TabIndex = 0;
            this.nUpDown_NumberOfBackups.Value = new decimal(new int[] {
            50,
            0,
            0,
            0});
            this.nUpDown_NumberOfBackups.ValueChanged += new System.EventHandler(this.Save_NumberOfBackups);
            // 
            // groupBox_AutoLoadProfiles
            // 
            this.groupBox_AutoLoadProfiles.Controls.Add(this.button_AutoLoadProfile_ClearSelection);
            this.groupBox_AutoLoadProfiles.Controls.Add(this.listBox_AutoLoadProfiles);
            this.groupBox_AutoLoadProfiles.Location = new System.Drawing.Point(892, 266);
            this.groupBox_AutoLoadProfiles.Name = "groupBox_AutoLoadProfiles";
            this.groupBox_AutoLoadProfiles.Size = new System.Drawing.Size(200, 171);
            this.groupBox_AutoLoadProfiles.TabIndex = 100005;
            this.groupBox_AutoLoadProfiles.TabStop = false;
            this.groupBox_AutoLoadProfiles.Text = "Auto Load Profile";
            this.groupBox_AutoLoadProfiles.Visible = false;
            this.groupBox_AutoLoadProfiles.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_AutoLoadProfile_ClearSelection
            // 
            this.button_AutoLoadProfile_ClearSelection.Location = new System.Drawing.Point(15, 140);
            this.button_AutoLoadProfile_ClearSelection.Name = "button_AutoLoadProfile_ClearSelection";
            this.button_AutoLoadProfile_ClearSelection.Size = new System.Drawing.Size(166, 23);
            this.button_AutoLoadProfile_ClearSelection.TabIndex = 1;
            this.button_AutoLoadProfile_ClearSelection.Text = "Clear Selection";
            this.button_AutoLoadProfile_ClearSelection.UseVisualStyleBackColor = true;
            this.button_AutoLoadProfile_ClearSelection.Click += new System.EventHandler(this.AutoLoadProfile_ClearSelection);
            this.button_AutoLoadProfile_ClearSelection.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // listBox_AutoLoadProfiles
            // 
            this.listBox_AutoLoadProfiles.FormattingEnabled = true;
            this.listBox_AutoLoadProfiles.Location = new System.Drawing.Point(15, 26);
            this.listBox_AutoLoadProfiles.Name = "listBox_AutoLoadProfiles";
            this.listBox_AutoLoadProfiles.Size = new System.Drawing.Size(166, 108);
            this.listBox_AutoLoadProfiles.TabIndex = 0;
            this.listBox_AutoLoadProfiles.SelectedIndexChanged += new System.EventHandler(this.Save_AutoLoadProfile);
            this.listBox_AutoLoadProfiles.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_MidiAutoTuneDevice
            // 
            this.groupBox_MidiAutoTuneDevice.Controls.Add(this.checkBox_WhammyChordsMode);
            this.groupBox_MidiAutoTuneDevice.Controls.Add(this.radio_WhammyORBass);
            this.groupBox_MidiAutoTuneDevice.Controls.Add(this.radio_WhammyDT);
            this.groupBox_MidiAutoTuneDevice.Controls.Add(this.label_MidiWhatTuningPedalDoYouUse);
            this.groupBox_MidiAutoTuneDevice.Controls.Add(this.label_SelectedMidiDevice);
            this.groupBox_MidiAutoTuneDevice.Controls.Add(this.listBox_ListMidiDevices);
            this.groupBox_MidiAutoTuneDevice.Location = new System.Drawing.Point(791, 18);
            this.groupBox_MidiAutoTuneDevice.Name = "groupBox_MidiAutoTuneDevice";
            this.groupBox_MidiAutoTuneDevice.Size = new System.Drawing.Size(315, 209);
            this.groupBox_MidiAutoTuneDevice.TabIndex = 100002;
            this.groupBox_MidiAutoTuneDevice.TabStop = false;
            this.groupBox_MidiAutoTuneDevice.Text = "Midi Device To Tune";
            this.groupBox_MidiAutoTuneDevice.Visible = false;
            this.groupBox_MidiAutoTuneDevice.VisibleChanged += new System.EventHandler(this.LoadMidiDeviceNames);
            this.groupBox_MidiAutoTuneDevice.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_WhammyChordsMode
            // 
            this.checkBox_WhammyChordsMode.AutoSize = true;
            this.checkBox_WhammyChordsMode.Location = new System.Drawing.Point(212, 172);
            this.checkBox_WhammyChordsMode.Name = "checkBox_WhammyChordsMode";
            this.checkBox_WhammyChordsMode.Size = new System.Drawing.Size(89, 17);
            this.checkBox_WhammyChordsMode.TabIndex = 5;
            this.checkBox_WhammyChordsMode.Text = "Chords Mode";
            this.checkBox_WhammyChordsMode.UseVisualStyleBackColor = true;
            this.checkBox_WhammyChordsMode.Visible = false;
            this.checkBox_WhammyChordsMode.CheckedChanged += new System.EventHandler(this.Save_WhammyChordsMode);
            this.checkBox_WhammyChordsMode.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_WhammyORBass
            // 
            this.radio_WhammyORBass.AutoSize = true;
            this.radio_WhammyORBass.Location = new System.Drawing.Point(10, 171);
            this.radio_WhammyORBass.Name = "radio_WhammyORBass";
            this.radio_WhammyORBass.Size = new System.Drawing.Size(196, 17);
            this.radio_WhammyORBass.TabIndex = 4;
            this.radio_WhammyORBass.TabStop = true;
            this.radio_WhammyORBass.Text = "Digitech Whammy or Whammy Bass";
            this.radio_WhammyORBass.UseVisualStyleBackColor = true;
            this.radio_WhammyORBass.CheckedChanged += new System.EventHandler(this.Save_WhammyORBass);
            // 
            // radio_WhammyDT
            // 
            this.radio_WhammyDT.AutoSize = true;
            this.radio_WhammyDT.Location = new System.Drawing.Point(10, 143);
            this.radio_WhammyDT.Name = "radio_WhammyDT";
            this.radio_WhammyDT.Size = new System.Drawing.Size(129, 17);
            this.radio_WhammyDT.TabIndex = 3;
            this.radio_WhammyDT.TabStop = true;
            this.radio_WhammyDT.Text = "Digitech Whammy DT";
            this.radio_WhammyDT.UseVisualStyleBackColor = true;
            this.radio_WhammyDT.CheckedChanged += new System.EventHandler(this.Save_WhammyDT);
            // 
            // label_MidiWhatTuningPedalDoYouUse
            // 
            this.label_MidiWhatTuningPedalDoYouUse.AutoSize = true;
            this.label_MidiWhatTuningPedalDoYouUse.Location = new System.Drawing.Point(7, 128);
            this.label_MidiWhatTuningPedalDoYouUse.Name = "label_MidiWhatTuningPedalDoYouUse";
            this.label_MidiWhatTuningPedalDoYouUse.Size = new System.Drawing.Size(166, 13);
            this.label_MidiWhatTuningPedalDoYouUse.TabIndex = 2;
            this.label_MidiWhatTuningPedalDoYouUse.Text = "What Tuning Pedal Do You Use?";
            // 
            // label_SelectedMidiDevice
            // 
            this.label_SelectedMidiDevice.AutoSize = true;
            this.label_SelectedMidiDevice.Location = new System.Drawing.Point(7, 96);
            this.label_SelectedMidiDevice.Name = "label_SelectedMidiDevice";
            this.label_SelectedMidiDevice.Size = new System.Drawing.Size(69, 13);
            this.label_SelectedMidiDevice.TabIndex = 1;
            this.label_SelectedMidiDevice.Text = "Midi Device: ";
            // 
            // listBox_ListMidiDevices
            // 
            this.listBox_ListMidiDevices.FormattingEnabled = true;
            this.listBox_ListMidiDevices.Location = new System.Drawing.Point(13, 27);
            this.listBox_ListMidiDevices.Name = "listBox_ListMidiDevices";
            this.listBox_ListMidiDevices.Size = new System.Drawing.Size(288, 43);
            this.listBox_ListMidiDevices.TabIndex = 0;
            this.listBox_ListMidiDevices.SelectedIndexChanged += new System.EventHandler(this.Save_AutoTuneDevice);
            // 
            // tabPage_ModSettings_Misc
            // 
            this.tabPage_ModSettings_Misc.BackColor = System.Drawing.Color.Azure;
            this.tabPage_ModSettings_Misc.Controls.Add(this.groupBox_CustomHighway);
            this.tabPage_ModSettings_Misc.Controls.Add(this.groupBox_OnScreenFont);
            this.tabPage_ModSettings_Misc.Controls.Add(this.groupBox_RRSpeed);
            this.tabPage_ModSettings_Misc.Controls.Add(this.groupBox_ControlVolumeIncrement);
            this.tabPage_ModSettings_Misc.Location = new System.Drawing.Point(4, 22);
            this.tabPage_ModSettings_Misc.Name = "tabPage_ModSettings_Misc";
            this.tabPage_ModSettings_Misc.Size = new System.Drawing.Size(1122, 447);
            this.tabPage_ModSettings_Misc.TabIndex = 2;
            this.tabPage_ModSettings_Misc.Text = "Misc";
            // 
            // groupBox_CustomHighway
            // 
            this.groupBox_CustomHighway.Controls.Add(this.button_ResetNotewayColors);
            this.groupBox_CustomHighway.Controls.Add(this.textBox_ShowFretNumber);
            this.groupBox_CustomHighway.Controls.Add(this.textBox_ShowNotewayGutter);
            this.groupBox_CustomHighway.Controls.Add(this.textBox_ShowUnNumberedFrets);
            this.groupBox_CustomHighway.Controls.Add(this.textBox_ShowNumberedFrets);
            this.groupBox_CustomHighway.Controls.Add(this.button_ChangeFretNumber);
            this.groupBox_CustomHighway.Controls.Add(this.button_ChangeNotewayGutter);
            this.groupBox_CustomHighway.Controls.Add(this.button_ChangeUnNumberedFrets);
            this.groupBox_CustomHighway.Controls.Add(this.button_ChangeNumberedFrets);
            this.groupBox_CustomHighway.Location = new System.Drawing.Point(612, 16);
            this.groupBox_CustomHighway.Name = "groupBox_CustomHighway";
            this.groupBox_CustomHighway.Size = new System.Drawing.Size(308, 169);
            this.groupBox_CustomHighway.TabIndex = 100005;
            this.groupBox_CustomHighway.TabStop = false;
            this.groupBox_CustomHighway.Text = "Custom Highway Colors";
            this.groupBox_CustomHighway.Visible = false;
            this.groupBox_CustomHighway.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_ResetNotewayColors
            // 
            this.button_ResetNotewayColors.Location = new System.Drawing.Point(157, 140);
            this.button_ResetNotewayColors.Name = "button_ResetNotewayColors";
            this.button_ResetNotewayColors.Size = new System.Drawing.Size(134, 23);
            this.button_ResetNotewayColors.TabIndex = 8;
            this.button_ResetNotewayColors.Text = "Reset Colors";
            this.button_ResetNotewayColors.UseVisualStyleBackColor = true;
            this.button_ResetNotewayColors.Click += new System.EventHandler(this.ResetNotewayColors);
            // 
            // textBox_ShowFretNumber
            // 
            this.textBox_ShowFretNumber.Enabled = false;
            this.textBox_ShowFretNumber.Location = new System.Drawing.Point(157, 116);
            this.textBox_ShowFretNumber.Name = "textBox_ShowFretNumber";
            this.textBox_ShowFretNumber.ReadOnly = true;
            this.textBox_ShowFretNumber.Size = new System.Drawing.Size(134, 20);
            this.textBox_ShowFretNumber.TabIndex = 7;
            // 
            // textBox_ShowNotewayGutter
            // 
            this.textBox_ShowNotewayGutter.Enabled = false;
            this.textBox_ShowNotewayGutter.Location = new System.Drawing.Point(157, 87);
            this.textBox_ShowNotewayGutter.Name = "textBox_ShowNotewayGutter";
            this.textBox_ShowNotewayGutter.ReadOnly = true;
            this.textBox_ShowNotewayGutter.Size = new System.Drawing.Size(134, 20);
            this.textBox_ShowNotewayGutter.TabIndex = 6;
            // 
            // textBox_ShowUnNumberedFrets
            // 
            this.textBox_ShowUnNumberedFrets.Enabled = false;
            this.textBox_ShowUnNumberedFrets.Location = new System.Drawing.Point(157, 58);
            this.textBox_ShowUnNumberedFrets.Name = "textBox_ShowUnNumberedFrets";
            this.textBox_ShowUnNumberedFrets.ReadOnly = true;
            this.textBox_ShowUnNumberedFrets.Size = new System.Drawing.Size(134, 20);
            this.textBox_ShowUnNumberedFrets.TabIndex = 5;
            // 
            // textBox_ShowNumberedFrets
            // 
            this.textBox_ShowNumberedFrets.Enabled = false;
            this.textBox_ShowNumberedFrets.Location = new System.Drawing.Point(157, 29);
            this.textBox_ShowNumberedFrets.Name = "textBox_ShowNumberedFrets";
            this.textBox_ShowNumberedFrets.ReadOnly = true;
            this.textBox_ShowNumberedFrets.Size = new System.Drawing.Size(134, 20);
            this.textBox_ShowNumberedFrets.TabIndex = 4;
            // 
            // button_ChangeFretNumber
            // 
            this.button_ChangeFretNumber.Location = new System.Drawing.Point(7, 113);
            this.button_ChangeFretNumber.Name = "button_ChangeFretNumber";
            this.button_ChangeFretNumber.Size = new System.Drawing.Size(144, 23);
            this.button_ChangeFretNumber.TabIndex = 3;
            this.button_ChangeFretNumber.Text = "Change Fret Number";
            this.button_ChangeFretNumber.UseVisualStyleBackColor = true;
            this.button_ChangeFretNumber.Click += new System.EventHandler(this.NotewayColors_ChangeNotewayColor);
            this.button_ChangeFretNumber.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_ChangeNotewayGutter
            // 
            this.button_ChangeNotewayGutter.Location = new System.Drawing.Point(6, 84);
            this.button_ChangeNotewayGutter.Name = "button_ChangeNotewayGutter";
            this.button_ChangeNotewayGutter.Size = new System.Drawing.Size(144, 23);
            this.button_ChangeNotewayGutter.TabIndex = 2;
            this.button_ChangeNotewayGutter.Text = "Change Noteway Sides";
            this.button_ChangeNotewayGutter.UseVisualStyleBackColor = true;
            this.button_ChangeNotewayGutter.Click += new System.EventHandler(this.NotewayColors_ChangeNotewayColor);
            this.button_ChangeNotewayGutter.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_ChangeUnNumberedFrets
            // 
            this.button_ChangeUnNumberedFrets.Location = new System.Drawing.Point(6, 55);
            this.button_ChangeUnNumberedFrets.Name = "button_ChangeUnNumberedFrets";
            this.button_ChangeUnNumberedFrets.Size = new System.Drawing.Size(144, 23);
            this.button_ChangeUnNumberedFrets.TabIndex = 1;
            this.button_ChangeUnNumberedFrets.Text = "Change UnNumbered Frets";
            this.button_ChangeUnNumberedFrets.UseVisualStyleBackColor = true;
            this.button_ChangeUnNumberedFrets.Click += new System.EventHandler(this.NotewayColors_ChangeNotewayColor);
            this.button_ChangeUnNumberedFrets.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_ChangeNumberedFrets
            // 
            this.button_ChangeNumberedFrets.Location = new System.Drawing.Point(6, 26);
            this.button_ChangeNumberedFrets.Name = "button_ChangeNumberedFrets";
            this.button_ChangeNumberedFrets.Size = new System.Drawing.Size(145, 23);
            this.button_ChangeNumberedFrets.TabIndex = 0;
            this.button_ChangeNumberedFrets.Text = "Change Numbered Frets";
            this.button_ChangeNumberedFrets.UseVisualStyleBackColor = true;
            this.button_ChangeNumberedFrets.Click += new System.EventHandler(this.NotewayColors_ChangeNotewayColor);
            this.button_ChangeNumberedFrets.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_OnScreenFont
            // 
            this.groupBox_OnScreenFont.Controls.Add(this.label_FontTestNumbers);
            this.groupBox_OnScreenFont.Controls.Add(this.label_FontTestlowercase);
            this.groupBox_OnScreenFont.Controls.Add(this.label_FontTestCAPITALS);
            this.groupBox_OnScreenFont.Controls.Add(this.listBox_AvailableFonts);
            this.groupBox_OnScreenFont.Location = new System.Drawing.Point(222, 16);
            this.groupBox_OnScreenFont.Name = "groupBox_OnScreenFont";
            this.groupBox_OnScreenFont.Size = new System.Drawing.Size(357, 115);
            this.groupBox_OnScreenFont.TabIndex = 100003;
            this.groupBox_OnScreenFont.TabStop = false;
            this.groupBox_OnScreenFont.Text = "On Screen Font";
            this.groupBox_OnScreenFont.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_FontTestNumbers
            // 
            this.label_FontTestNumbers.AutoSize = true;
            this.label_FontTestNumbers.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F);
            this.label_FontTestNumbers.Location = new System.Drawing.Point(203, 92);
            this.label_FontTestNumbers.Name = "label_FontTestNumbers";
            this.label_FontTestNumbers.Size = new System.Drawing.Size(88, 17);
            this.label_FontTestNumbers.TabIndex = 3;
            this.label_FontTestNumbers.Text = "1234567890";
            // 
            // label_FontTestlowercase
            // 
            this.label_FontTestlowercase.AutoSize = true;
            this.label_FontTestlowercase.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F);
            this.label_FontTestlowercase.Location = new System.Drawing.Point(268, 15);
            this.label_FontTestlowercase.Name = "label_FontTestlowercase";
            this.label_FontTestlowercase.Size = new System.Drawing.Size(59, 68);
            this.label_FontTestlowercase.TabIndex = 2;
            this.label_FontTestlowercase.Text = "abcdefg\r\nhijklmn\r\nopqrstu\r\nvwxyz";
            // 
            // label_FontTestCAPITALS
            // 
            this.label_FontTestCAPITALS.AutoSize = true;
            this.label_FontTestCAPITALS.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F);
            this.label_FontTestCAPITALS.Location = new System.Drawing.Point(189, 15);
            this.label_FontTestCAPITALS.Name = "label_FontTestCAPITALS";
            this.label_FontTestCAPITALS.Size = new System.Drawing.Size(77, 68);
            this.label_FontTestCAPITALS.TabIndex = 1;
            this.label_FontTestCAPITALS.Text = "ABCDEFG\r\nHIJKLMN\r\nOPQRSTU\r\nVWXYZ\r\n";
            // 
            // listBox_AvailableFonts
            // 
            this.listBox_AvailableFonts.FormattingEnabled = true;
            this.listBox_AvailableFonts.Location = new System.Drawing.Point(9, 20);
            this.listBox_AvailableFonts.Name = "listBox_AvailableFonts";
            this.listBox_AvailableFonts.Size = new System.Drawing.Size(174, 82);
            this.listBox_AvailableFonts.TabIndex = 0;
            this.listBox_AvailableFonts.SelectedIndexChanged += new System.EventHandler(this.ChangeOnScreenFont);
            // 
            // groupBox_RRSpeed
            // 
            this.groupBox_RRSpeed.Controls.Add(this.nUpDown_RiffRepeaterSpeed);
            this.groupBox_RRSpeed.Location = new System.Drawing.Point(21, 81);
            this.groupBox_RRSpeed.Name = "groupBox_RRSpeed";
            this.groupBox_RRSpeed.Size = new System.Drawing.Size(172, 50);
            this.groupBox_RRSpeed.TabIndex = 100001;
            this.groupBox_RRSpeed.TabStop = false;
            this.groupBox_RRSpeed.Text = "Riff Repeater Speed Increment";
            this.groupBox_RRSpeed.Visible = false;
            // 
            // nUpDown_RiffRepeaterSpeed
            // 
            this.nUpDown_RiffRepeaterSpeed.Location = new System.Drawing.Point(7, 20);
            this.nUpDown_RiffRepeaterSpeed.Maximum = new decimal(new int[] {
            50,
            0,
            0,
            0});
            this.nUpDown_RiffRepeaterSpeed.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nUpDown_RiffRepeaterSpeed.Name = "nUpDown_RiffRepeaterSpeed";
            this.nUpDown_RiffRepeaterSpeed.Size = new System.Drawing.Size(153, 20);
            this.nUpDown_RiffRepeaterSpeed.TabIndex = 200;
            this.nUpDown_RiffRepeaterSpeed.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nUpDown_RiffRepeaterSpeed.ValueChanged += new System.EventHandler(this.Save_RiffRepeaterSpeedInterval);
            // 
            // groupBox_ControlVolumeIncrement
            // 
            this.groupBox_ControlVolumeIncrement.Controls.Add(this.nUpDown_VolumeInterval);
            this.groupBox_ControlVolumeIncrement.Location = new System.Drawing.Point(21, 16);
            this.groupBox_ControlVolumeIncrement.Name = "groupBox_ControlVolumeIncrement";
            this.groupBox_ControlVolumeIncrement.Size = new System.Drawing.Size(172, 50);
            this.groupBox_ControlVolumeIncrement.TabIndex = 100004;
            this.groupBox_ControlVolumeIncrement.TabStop = false;
            this.groupBox_ControlVolumeIncrement.Text = "Control Volume Increment";
            this.groupBox_ControlVolumeIncrement.Visible = false;
            // 
            // nUpDown_VolumeInterval
            // 
            this.nUpDown_VolumeInterval.Location = new System.Drawing.Point(6, 20);
            this.nUpDown_VolumeInterval.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nUpDown_VolumeInterval.Name = "nUpDown_VolumeInterval";
            this.nUpDown_VolumeInterval.Size = new System.Drawing.Size(154, 20);
            this.nUpDown_VolumeInterval.TabIndex = 0;
            this.nUpDown_VolumeInterval.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nUpDown_VolumeInterval.ValueChanged += new System.EventHandler(this.Save_VolumeInterval);
            // 
            // tab_RSASIO
            // 
            this.tab_RSASIO.BackColor = System.Drawing.Color.Azure;
            this.tab_RSASIO.Controls.Add(this.groupBox_ASIO_InputMic);
            this.tab_RSASIO.Controls.Add(this.label_ASIO_Credits);
            this.tab_RSASIO.Controls.Add(this.groupBox_ASIO_Output);
            this.tab_RSASIO.Controls.Add(this.groupBox_ASIO_Input1);
            this.tab_RSASIO.Controls.Add(this.groupBox_ASIO_Input0);
            this.tab_RSASIO.Controls.Add(this.groupBox_ASIO_Config);
            this.tab_RSASIO.Controls.Add(this.groupBox_ASIO_BufferSize);
            this.tab_RSASIO.Location = new System.Drawing.Point(4, 22);
            this.tab_RSASIO.Name = "tab_RSASIO";
            this.tab_RSASIO.Size = new System.Drawing.Size(1150, 513);
            this.tab_RSASIO.TabIndex = 7;
            this.tab_RSASIO.Text = "RS_ASIO Settings";
            // 
            // groupBox_ASIO_InputMic
            // 
            this.groupBox_ASIO_InputMic.Controls.Add(this.button_ASIO_InputMic_ClearSelection);
            this.groupBox_ASIO_InputMic.Controls.Add(this.checkBox_ASIO_InputMic_Disabled);
            this.groupBox_ASIO_InputMic.Controls.Add(this.checkBox_ASIO_InputMic_ControlEndpointVolume);
            this.groupBox_ASIO_InputMic.Controls.Add(this.checkBox_ASIO_InputMic_ControlMasterVolume);
            this.groupBox_ASIO_InputMic.Controls.Add(this.label_ASIO_InputMic_MaxVolume);
            this.groupBox_ASIO_InputMic.Controls.Add(this.nUpDown_ASIO_InputMic_MaxVolume);
            this.groupBox_ASIO_InputMic.Controls.Add(this.nUpDown_ASIO_InputMic_Channel);
            this.groupBox_ASIO_InputMic.Controls.Add(this.label_ASIO_InputMic_Channel);
            this.groupBox_ASIO_InputMic.Controls.Add(this.listBox_AvailableASIODevices_InputMic);
            this.groupBox_ASIO_InputMic.Location = new System.Drawing.Point(490, 260);
            this.groupBox_ASIO_InputMic.Name = "groupBox_ASIO_InputMic";
            this.groupBox_ASIO_InputMic.Size = new System.Drawing.Size(470, 230);
            this.groupBox_ASIO_InputMic.TabIndex = 12;
            this.groupBox_ASIO_InputMic.TabStop = false;
            this.groupBox_ASIO_InputMic.Text = "Input Microphone";
            this.groupBox_ASIO_InputMic.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_ASIO_InputMic_ClearSelection
            // 
            this.button_ASIO_InputMic_ClearSelection.Location = new System.Drawing.Point(22, 192);
            this.button_ASIO_InputMic_ClearSelection.Name = "button_ASIO_InputMic_ClearSelection";
            this.button_ASIO_InputMic_ClearSelection.Size = new System.Drawing.Size(194, 23);
            this.button_ASIO_InputMic_ClearSelection.TabIndex = 14;
            this.button_ASIO_InputMic_ClearSelection.Text = "Clear Selection";
            this.button_ASIO_InputMic_ClearSelection.UseVisualStyleBackColor = true;
            this.button_ASIO_InputMic_ClearSelection.Click += new System.EventHandler(this.ASIO_InputMic_ClearSelection);
            // 
            // checkBox_ASIO_InputMic_Disabled
            // 
            this.checkBox_ASIO_InputMic_Disabled.AutoSize = true;
            this.checkBox_ASIO_InputMic_Disabled.Location = new System.Drawing.Point(253, 207);
            this.checkBox_ASIO_InputMic_Disabled.Name = "checkBox_ASIO_InputMic_Disabled";
            this.checkBox_ASIO_InputMic_Disabled.Size = new System.Drawing.Size(67, 17);
            this.checkBox_ASIO_InputMic_Disabled.TabIndex = 10;
            this.checkBox_ASIO_InputMic_Disabled.Text = "Disabled";
            this.checkBox_ASIO_InputMic_Disabled.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_InputMic_Disabled.CheckedChanged += new System.EventHandler(this.ASIO_InputMic_Disable);
            // 
            // checkBox_ASIO_InputMic_ControlEndpointVolume
            // 
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.AutoSize = true;
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.Location = new System.Drawing.Point(253, 160);
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.Name = "checkBox_ASIO_InputMic_ControlEndpointVolume";
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.Size = new System.Drawing.Size(142, 17);
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.TabIndex = 9;
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.Text = "Control Endpoint Volume";
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.CheckedChanged += new System.EventHandler(this.ASIO_InputMic_EndpointVolume);
            this.checkBox_ASIO_InputMic_ControlEndpointVolume.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ASIO_InputMic_ControlMasterVolume
            // 
            this.checkBox_ASIO_InputMic_ControlMasterVolume.AutoSize = true;
            this.checkBox_ASIO_InputMic_ControlMasterVolume.Location = new System.Drawing.Point(253, 128);
            this.checkBox_ASIO_InputMic_ControlMasterVolume.Name = "checkBox_ASIO_InputMic_ControlMasterVolume";
            this.checkBox_ASIO_InputMic_ControlMasterVolume.Size = new System.Drawing.Size(132, 17);
            this.checkBox_ASIO_InputMic_ControlMasterVolume.TabIndex = 8;
            this.checkBox_ASIO_InputMic_ControlMasterVolume.Text = "Control Master Volume";
            this.checkBox_ASIO_InputMic_ControlMasterVolume.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_InputMic_ControlMasterVolume.CheckedChanged += new System.EventHandler(this.ASIO_InputMic_MasterVolume);
            // 
            // label_ASIO_InputMic_MaxVolume
            // 
            this.label_ASIO_InputMic_MaxVolume.AutoSize = true;
            this.label_ASIO_InputMic_MaxVolume.Location = new System.Drawing.Point(250, 69);
            this.label_ASIO_InputMic_MaxVolume.Name = "label_ASIO_InputMic_MaxVolume";
            this.label_ASIO_InputMic_MaxVolume.Size = new System.Drawing.Size(68, 13);
            this.label_ASIO_InputMic_MaxVolume.TabIndex = 7;
            this.label_ASIO_InputMic_MaxVolume.Text = "Max Volume:";
            this.label_ASIO_InputMic_MaxVolume.Visible = false;
            // 
            // nUpDown_ASIO_InputMic_MaxVolume
            // 
            this.nUpDown_ASIO_InputMic_MaxVolume.Location = new System.Drawing.Point(338, 67);
            this.nUpDown_ASIO_InputMic_MaxVolume.Name = "nUpDown_ASIO_InputMic_MaxVolume";
            this.nUpDown_ASIO_InputMic_MaxVolume.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_InputMic_MaxVolume.TabIndex = 4;
            this.nUpDown_ASIO_InputMic_MaxVolume.Visible = false;
            this.nUpDown_ASIO_InputMic_MaxVolume.ValueChanged += new System.EventHandler(this.ASIO_InputMic_MaxVolume);
            // 
            // nUpDown_ASIO_InputMic_Channel
            // 
            this.nUpDown_ASIO_InputMic_Channel.Location = new System.Drawing.Point(338, 28);
            this.nUpDown_ASIO_InputMic_Channel.Maximum = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.nUpDown_ASIO_InputMic_Channel.Name = "nUpDown_ASIO_InputMic_Channel";
            this.nUpDown_ASIO_InputMic_Channel.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_InputMic_Channel.TabIndex = 3;
            this.nUpDown_ASIO_InputMic_Channel.ValueChanged += new System.EventHandler(this.ASIO_InputMic_Channel);
            // 
            // label_ASIO_InputMic_Channel
            // 
            this.label_ASIO_InputMic_Channel.AutoSize = true;
            this.label_ASIO_InputMic_Channel.Location = new System.Drawing.Point(250, 30);
            this.label_ASIO_InputMic_Channel.Name = "label_ASIO_InputMic_Channel";
            this.label_ASIO_InputMic_Channel.Size = new System.Drawing.Size(49, 13);
            this.label_ASIO_InputMic_Channel.TabIndex = 2;
            this.label_ASIO_InputMic_Channel.Text = "Channel:";
            this.label_ASIO_InputMic_Channel.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // listBox_AvailableASIODevices_InputMic
            // 
            this.listBox_AvailableASIODevices_InputMic.FormattingEnabled = true;
            this.listBox_AvailableASIODevices_InputMic.Location = new System.Drawing.Point(23, 30);
            this.listBox_AvailableASIODevices_InputMic.Name = "listBox_AvailableASIODevices_InputMic";
            this.listBox_AvailableASIODevices_InputMic.Size = new System.Drawing.Size(193, 147);
            this.listBox_AvailableASIODevices_InputMic.TabIndex = 0;
            this.listBox_AvailableASIODevices_InputMic.SelectedIndexChanged += new System.EventHandler(this.ASIO_ListAvailableInputMic);
            this.listBox_AvailableASIODevices_InputMic.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_ASIO_Credits
            // 
            this.label_ASIO_Credits.AutoSize = true;
            this.label_ASIO_Credits.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F);
            this.label_ASIO_Credits.Location = new System.Drawing.Point(972, 466);
            this.label_ASIO_Credits.Name = "label_ASIO_Credits";
            this.label_ASIO_Credits.Size = new System.Drawing.Size(163, 34);
            this.label_ASIO_Credits.TabIndex = 11;
            this.label_ASIO_Credits.Text = "Big Thank You To Mdias\r\nFor Creating RS_ASIO";
            // 
            // groupBox_ASIO_Output
            // 
            this.groupBox_ASIO_Output.Controls.Add(this.button_ASIO_Output_ClearSelection);
            this.groupBox_ASIO_Output.Controls.Add(this.checkBox_ASIO_Output_Disabled);
            this.groupBox_ASIO_Output.Controls.Add(this.checkBox_ASIO_Output_ControlEndpointVolume);
            this.groupBox_ASIO_Output.Controls.Add(this.checkBox_ASIO_Output_ControlMasterVolume);
            this.groupBox_ASIO_Output.Controls.Add(this.label_ASIO_Output_MaxVolume);
            this.groupBox_ASIO_Output.Controls.Add(this.nUpDown_ASIO_Output_MaxVolume);
            this.groupBox_ASIO_Output.Controls.Add(this.nUpDown_ASIO_Output_BaseChannel);
            this.groupBox_ASIO_Output.Controls.Add(this.label_ASIO_Output_BaseChannel);
            this.groupBox_ASIO_Output.Controls.Add(this.listBox_AvailableASIODevices_Output);
            this.groupBox_ASIO_Output.Location = new System.Drawing.Point(490, 24);
            this.groupBox_ASIO_Output.Name = "groupBox_ASIO_Output";
            this.groupBox_ASIO_Output.Size = new System.Drawing.Size(470, 224);
            this.groupBox_ASIO_Output.TabIndex = 10;
            this.groupBox_ASIO_Output.TabStop = false;
            this.groupBox_ASIO_Output.Text = "Output";
            this.groupBox_ASIO_Output.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_ASIO_Output_ClearSelection
            // 
            this.button_ASIO_Output_ClearSelection.Location = new System.Drawing.Point(22, 192);
            this.button_ASIO_Output_ClearSelection.Name = "button_ASIO_Output_ClearSelection";
            this.button_ASIO_Output_ClearSelection.Size = new System.Drawing.Size(194, 23);
            this.button_ASIO_Output_ClearSelection.TabIndex = 13;
            this.button_ASIO_Output_ClearSelection.Text = "Clear Selection";
            this.button_ASIO_Output_ClearSelection.UseVisualStyleBackColor = true;
            this.button_ASIO_Output_ClearSelection.Click += new System.EventHandler(this.ASIO_Output_ClearSelection);
            // 
            // checkBox_ASIO_Output_Disabled
            // 
            this.checkBox_ASIO_Output_Disabled.AutoSize = true;
            this.checkBox_ASIO_Output_Disabled.Location = new System.Drawing.Point(253, 198);
            this.checkBox_ASIO_Output_Disabled.Name = "checkBox_ASIO_Output_Disabled";
            this.checkBox_ASIO_Output_Disabled.Size = new System.Drawing.Size(67, 17);
            this.checkBox_ASIO_Output_Disabled.TabIndex = 12;
            this.checkBox_ASIO_Output_Disabled.Text = "Disabled";
            this.checkBox_ASIO_Output_Disabled.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Output_Disabled.CheckedChanged += new System.EventHandler(this.ASIO_Output_Disable);
            // 
            // checkBox_ASIO_Output_ControlEndpointVolume
            // 
            this.checkBox_ASIO_Output_ControlEndpointVolume.AutoSize = true;
            this.checkBox_ASIO_Output_ControlEndpointVolume.Location = new System.Drawing.Point(253, 160);
            this.checkBox_ASIO_Output_ControlEndpointVolume.Name = "checkBox_ASIO_Output_ControlEndpointVolume";
            this.checkBox_ASIO_Output_ControlEndpointVolume.Size = new System.Drawing.Size(142, 17);
            this.checkBox_ASIO_Output_ControlEndpointVolume.TabIndex = 9;
            this.checkBox_ASIO_Output_ControlEndpointVolume.Text = "Control Endpoint Volume";
            this.checkBox_ASIO_Output_ControlEndpointVolume.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Output_ControlEndpointVolume.CheckedChanged += new System.EventHandler(this.ASIO_Output_EndpointVolume);
            this.checkBox_ASIO_Output_ControlEndpointVolume.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ASIO_Output_ControlMasterVolume
            // 
            this.checkBox_ASIO_Output_ControlMasterVolume.AutoSize = true;
            this.checkBox_ASIO_Output_ControlMasterVolume.Location = new System.Drawing.Point(253, 128);
            this.checkBox_ASIO_Output_ControlMasterVolume.Name = "checkBox_ASIO_Output_ControlMasterVolume";
            this.checkBox_ASIO_Output_ControlMasterVolume.Size = new System.Drawing.Size(132, 17);
            this.checkBox_ASIO_Output_ControlMasterVolume.TabIndex = 8;
            this.checkBox_ASIO_Output_ControlMasterVolume.Text = "Control Master Volume";
            this.checkBox_ASIO_Output_ControlMasterVolume.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Output_ControlMasterVolume.CheckedChanged += new System.EventHandler(this.ASIO_Output_MasterVolume);
            // 
            // label_ASIO_Output_MaxVolume
            // 
            this.label_ASIO_Output_MaxVolume.AutoSize = true;
            this.label_ASIO_Output_MaxVolume.Location = new System.Drawing.Point(250, 69);
            this.label_ASIO_Output_MaxVolume.Name = "label_ASIO_Output_MaxVolume";
            this.label_ASIO_Output_MaxVolume.Size = new System.Drawing.Size(68, 13);
            this.label_ASIO_Output_MaxVolume.TabIndex = 7;
            this.label_ASIO_Output_MaxVolume.Text = "Max Volume:";
            this.label_ASIO_Output_MaxVolume.Visible = false;
            // 
            // nUpDown_ASIO_Output_MaxVolume
            // 
            this.nUpDown_ASIO_Output_MaxVolume.Location = new System.Drawing.Point(338, 67);
            this.nUpDown_ASIO_Output_MaxVolume.Name = "nUpDown_ASIO_Output_MaxVolume";
            this.nUpDown_ASIO_Output_MaxVolume.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_Output_MaxVolume.TabIndex = 4;
            this.nUpDown_ASIO_Output_MaxVolume.Visible = false;
            this.nUpDown_ASIO_Output_MaxVolume.ValueChanged += new System.EventHandler(this.ASIO_Output_MaxVolume);
            // 
            // nUpDown_ASIO_Output_BaseChannel
            // 
            this.nUpDown_ASIO_Output_BaseChannel.Location = new System.Drawing.Point(338, 28);
            this.nUpDown_ASIO_Output_BaseChannel.Maximum = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.nUpDown_ASIO_Output_BaseChannel.Name = "nUpDown_ASIO_Output_BaseChannel";
            this.nUpDown_ASIO_Output_BaseChannel.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_Output_BaseChannel.TabIndex = 3;
            this.nUpDown_ASIO_Output_BaseChannel.ValueChanged += new System.EventHandler(this.ASIO_Output_BaseChannel);
            // 
            // label_ASIO_Output_BaseChannel
            // 
            this.label_ASIO_Output_BaseChannel.AutoSize = true;
            this.label_ASIO_Output_BaseChannel.Location = new System.Drawing.Point(250, 30);
            this.label_ASIO_Output_BaseChannel.Name = "label_ASIO_Output_BaseChannel";
            this.label_ASIO_Output_BaseChannel.Size = new System.Drawing.Size(76, 13);
            this.label_ASIO_Output_BaseChannel.TabIndex = 2;
            this.label_ASIO_Output_BaseChannel.Text = "Base Channel:";
            this.label_ASIO_Output_BaseChannel.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // listBox_AvailableASIODevices_Output
            // 
            this.listBox_AvailableASIODevices_Output.FormattingEnabled = true;
            this.listBox_AvailableASIODevices_Output.Location = new System.Drawing.Point(23, 30);
            this.listBox_AvailableASIODevices_Output.Name = "listBox_AvailableASIODevices_Output";
            this.listBox_AvailableASIODevices_Output.Size = new System.Drawing.Size(193, 147);
            this.listBox_AvailableASIODevices_Output.TabIndex = 0;
            this.listBox_AvailableASIODevices_Output.SelectedIndexChanged += new System.EventHandler(this.ASIO_ListAvailableOutput);
            this.listBox_AvailableASIODevices_Output.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_ASIO_Input1
            // 
            this.groupBox_ASIO_Input1.Controls.Add(this.button_ASIO_Input1_ClearSelection);
            this.groupBox_ASIO_Input1.Controls.Add(this.checkBox_ASIO_Input1_Disabled);
            this.groupBox_ASIO_Input1.Controls.Add(this.checkBox_ASIO_Input1_ControlEndpointVolume);
            this.groupBox_ASIO_Input1.Controls.Add(this.checkBox_ASIO_Input1_ControlMasterVolume);
            this.groupBox_ASIO_Input1.Controls.Add(this.label_ASIO_Input1_MaxVolume);
            this.groupBox_ASIO_Input1.Controls.Add(this.nUpDown_ASIO_Input1_MaxVolume);
            this.groupBox_ASIO_Input1.Controls.Add(this.nUpDown_ASIO_Input1_Channel);
            this.groupBox_ASIO_Input1.Controls.Add(this.label_ASIO_Input1_Channel);
            this.groupBox_ASIO_Input1.Controls.Add(this.listBox_AvailableASIODevices_Input1);
            this.groupBox_ASIO_Input1.Location = new System.Drawing.Point(14, 260);
            this.groupBox_ASIO_Input1.Name = "groupBox_ASIO_Input1";
            this.groupBox_ASIO_Input1.Size = new System.Drawing.Size(470, 230);
            this.groupBox_ASIO_Input1.TabIndex = 4;
            this.groupBox_ASIO_Input1.TabStop = false;
            this.groupBox_ASIO_Input1.Text = "Input 2";
            this.groupBox_ASIO_Input1.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_ASIO_Input1_ClearSelection
            // 
            this.button_ASIO_Input1_ClearSelection.Location = new System.Drawing.Point(22, 192);
            this.button_ASIO_Input1_ClearSelection.Name = "button_ASIO_Input1_ClearSelection";
            this.button_ASIO_Input1_ClearSelection.Size = new System.Drawing.Size(194, 23);
            this.button_ASIO_Input1_ClearSelection.TabIndex = 14;
            this.button_ASIO_Input1_ClearSelection.Text = "Clear Selection";
            this.button_ASIO_Input1_ClearSelection.UseVisualStyleBackColor = true;
            this.button_ASIO_Input1_ClearSelection.Click += new System.EventHandler(this.ASIO_Input1_ClearSelection);
            // 
            // checkBox_ASIO_Input1_Disabled
            // 
            this.checkBox_ASIO_Input1_Disabled.AutoSize = true;
            this.checkBox_ASIO_Input1_Disabled.Location = new System.Drawing.Point(253, 207);
            this.checkBox_ASIO_Input1_Disabled.Name = "checkBox_ASIO_Input1_Disabled";
            this.checkBox_ASIO_Input1_Disabled.Size = new System.Drawing.Size(67, 17);
            this.checkBox_ASIO_Input1_Disabled.TabIndex = 10;
            this.checkBox_ASIO_Input1_Disabled.Text = "Disabled";
            this.checkBox_ASIO_Input1_Disabled.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Input1_Disabled.CheckedChanged += new System.EventHandler(this.ASIO_Input1_Disable);
            // 
            // checkBox_ASIO_Input1_ControlEndpointVolume
            // 
            this.checkBox_ASIO_Input1_ControlEndpointVolume.AutoSize = true;
            this.checkBox_ASIO_Input1_ControlEndpointVolume.Location = new System.Drawing.Point(253, 160);
            this.checkBox_ASIO_Input1_ControlEndpointVolume.Name = "checkBox_ASIO_Input1_ControlEndpointVolume";
            this.checkBox_ASIO_Input1_ControlEndpointVolume.Size = new System.Drawing.Size(142, 17);
            this.checkBox_ASIO_Input1_ControlEndpointVolume.TabIndex = 9;
            this.checkBox_ASIO_Input1_ControlEndpointVolume.Text = "Control Endpoint Volume";
            this.checkBox_ASIO_Input1_ControlEndpointVolume.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Input1_ControlEndpointVolume.CheckedChanged += new System.EventHandler(this.ASIO_Input1_EndpointVolume);
            this.checkBox_ASIO_Input1_ControlEndpointVolume.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ASIO_Input1_ControlMasterVolume
            // 
            this.checkBox_ASIO_Input1_ControlMasterVolume.AutoSize = true;
            this.checkBox_ASIO_Input1_ControlMasterVolume.Location = new System.Drawing.Point(253, 128);
            this.checkBox_ASIO_Input1_ControlMasterVolume.Name = "checkBox_ASIO_Input1_ControlMasterVolume";
            this.checkBox_ASIO_Input1_ControlMasterVolume.Size = new System.Drawing.Size(132, 17);
            this.checkBox_ASIO_Input1_ControlMasterVolume.TabIndex = 8;
            this.checkBox_ASIO_Input1_ControlMasterVolume.Text = "Control Master Volume";
            this.checkBox_ASIO_Input1_ControlMasterVolume.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Input1_ControlMasterVolume.CheckedChanged += new System.EventHandler(this.ASIO_Input1_MasterVolume);
            // 
            // label_ASIO_Input1_MaxVolume
            // 
            this.label_ASIO_Input1_MaxVolume.AutoSize = true;
            this.label_ASIO_Input1_MaxVolume.Location = new System.Drawing.Point(250, 69);
            this.label_ASIO_Input1_MaxVolume.Name = "label_ASIO_Input1_MaxVolume";
            this.label_ASIO_Input1_MaxVolume.Size = new System.Drawing.Size(68, 13);
            this.label_ASIO_Input1_MaxVolume.TabIndex = 7;
            this.label_ASIO_Input1_MaxVolume.Text = "Max Volume:";
            this.label_ASIO_Input1_MaxVolume.Visible = false;
            // 
            // nUpDown_ASIO_Input1_MaxVolume
            // 
            this.nUpDown_ASIO_Input1_MaxVolume.Location = new System.Drawing.Point(338, 67);
            this.nUpDown_ASIO_Input1_MaxVolume.Name = "nUpDown_ASIO_Input1_MaxVolume";
            this.nUpDown_ASIO_Input1_MaxVolume.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_Input1_MaxVolume.TabIndex = 4;
            this.nUpDown_ASIO_Input1_MaxVolume.Visible = false;
            this.nUpDown_ASIO_Input1_MaxVolume.ValueChanged += new System.EventHandler(this.ASIO_Input1_MaxVolume);
            // 
            // nUpDown_ASIO_Input1_Channel
            // 
            this.nUpDown_ASIO_Input1_Channel.Location = new System.Drawing.Point(338, 28);
            this.nUpDown_ASIO_Input1_Channel.Maximum = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.nUpDown_ASIO_Input1_Channel.Name = "nUpDown_ASIO_Input1_Channel";
            this.nUpDown_ASIO_Input1_Channel.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_Input1_Channel.TabIndex = 3;
            this.nUpDown_ASIO_Input1_Channel.ValueChanged += new System.EventHandler(this.ASIO_Input1_Channel);
            // 
            // label_ASIO_Input1_Channel
            // 
            this.label_ASIO_Input1_Channel.AutoSize = true;
            this.label_ASIO_Input1_Channel.Location = new System.Drawing.Point(250, 30);
            this.label_ASIO_Input1_Channel.Name = "label_ASIO_Input1_Channel";
            this.label_ASIO_Input1_Channel.Size = new System.Drawing.Size(49, 13);
            this.label_ASIO_Input1_Channel.TabIndex = 2;
            this.label_ASIO_Input1_Channel.Text = "Channel:";
            this.label_ASIO_Input1_Channel.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // listBox_AvailableASIODevices_Input1
            // 
            this.listBox_AvailableASIODevices_Input1.FormattingEnabled = true;
            this.listBox_AvailableASIODevices_Input1.Location = new System.Drawing.Point(23, 30);
            this.listBox_AvailableASIODevices_Input1.Name = "listBox_AvailableASIODevices_Input1";
            this.listBox_AvailableASIODevices_Input1.Size = new System.Drawing.Size(193, 147);
            this.listBox_AvailableASIODevices_Input1.TabIndex = 0;
            this.listBox_AvailableASIODevices_Input1.SelectedIndexChanged += new System.EventHandler(this.ASIO_ListAvailableInput1);
            this.listBox_AvailableASIODevices_Input1.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_ASIO_Input0
            // 
            this.groupBox_ASIO_Input0.Controls.Add(this.button_ASIO_Input0_ClearSelection);
            this.groupBox_ASIO_Input0.Controls.Add(this.checkBox_ASIO_Input0_Disabled);
            this.groupBox_ASIO_Input0.Controls.Add(this.checkBox_ASIO_Input0_ControlEndpointVolume);
            this.groupBox_ASIO_Input0.Controls.Add(this.checkBox_ASIO_Input0_ControlMasterVolume);
            this.groupBox_ASIO_Input0.Controls.Add(this.label_ASIO_Input0_MaxVolume);
            this.groupBox_ASIO_Input0.Controls.Add(this.nUpDown_ASIO_Input0_MaxVolume);
            this.groupBox_ASIO_Input0.Controls.Add(this.nUpDown_ASIO_Input0_Channel);
            this.groupBox_ASIO_Input0.Controls.Add(this.label_ASIO_Input0_Channel);
            this.groupBox_ASIO_Input0.Controls.Add(this.listBox_AvailableASIODevices_Input0);
            this.groupBox_ASIO_Input0.Location = new System.Drawing.Point(14, 24);
            this.groupBox_ASIO_Input0.Name = "groupBox_ASIO_Input0";
            this.groupBox_ASIO_Input0.Size = new System.Drawing.Size(470, 224);
            this.groupBox_ASIO_Input0.TabIndex = 3;
            this.groupBox_ASIO_Input0.TabStop = false;
            this.groupBox_ASIO_Input0.Text = "Input 1";
            this.groupBox_ASIO_Input0.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_ASIO_Input0_ClearSelection
            // 
            this.button_ASIO_Input0_ClearSelection.Location = new System.Drawing.Point(22, 192);
            this.button_ASIO_Input0_ClearSelection.Name = "button_ASIO_Input0_ClearSelection";
            this.button_ASIO_Input0_ClearSelection.Size = new System.Drawing.Size(194, 23);
            this.button_ASIO_Input0_ClearSelection.TabIndex = 15;
            this.button_ASIO_Input0_ClearSelection.Text = "Clear Selection";
            this.button_ASIO_Input0_ClearSelection.UseVisualStyleBackColor = true;
            this.button_ASIO_Input0_ClearSelection.Click += new System.EventHandler(this.ASIO_Input0_ClearSelection);
            // 
            // checkBox_ASIO_Input0_Disabled
            // 
            this.checkBox_ASIO_Input0_Disabled.AutoSize = true;
            this.checkBox_ASIO_Input0_Disabled.Location = new System.Drawing.Point(253, 192);
            this.checkBox_ASIO_Input0_Disabled.Name = "checkBox_ASIO_Input0_Disabled";
            this.checkBox_ASIO_Input0_Disabled.Size = new System.Drawing.Size(67, 17);
            this.checkBox_ASIO_Input0_Disabled.TabIndex = 11;
            this.checkBox_ASIO_Input0_Disabled.Text = "Disabled";
            this.checkBox_ASIO_Input0_Disabled.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Input0_Disabled.CheckedChanged += new System.EventHandler(this.ASIO_Input0_Disable);
            // 
            // checkBox_ASIO_Input0_ControlEndpointVolume
            // 
            this.checkBox_ASIO_Input0_ControlEndpointVolume.AutoSize = true;
            this.checkBox_ASIO_Input0_ControlEndpointVolume.Location = new System.Drawing.Point(253, 142);
            this.checkBox_ASIO_Input0_ControlEndpointVolume.Name = "checkBox_ASIO_Input0_ControlEndpointVolume";
            this.checkBox_ASIO_Input0_ControlEndpointVolume.Size = new System.Drawing.Size(142, 17);
            this.checkBox_ASIO_Input0_ControlEndpointVolume.TabIndex = 11;
            this.checkBox_ASIO_Input0_ControlEndpointVolume.Text = "Control Endpoint Volume";
            this.checkBox_ASIO_Input0_ControlEndpointVolume.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Input0_ControlEndpointVolume.CheckedChanged += new System.EventHandler(this.ASIO_Input0_EndpointVolume);
            this.checkBox_ASIO_Input0_ControlEndpointVolume.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ASIO_Input0_ControlMasterVolume
            // 
            this.checkBox_ASIO_Input0_ControlMasterVolume.AutoSize = true;
            this.checkBox_ASIO_Input0_ControlMasterVolume.Location = new System.Drawing.Point(253, 110);
            this.checkBox_ASIO_Input0_ControlMasterVolume.Name = "checkBox_ASIO_Input0_ControlMasterVolume";
            this.checkBox_ASIO_Input0_ControlMasterVolume.Size = new System.Drawing.Size(132, 17);
            this.checkBox_ASIO_Input0_ControlMasterVolume.TabIndex = 10;
            this.checkBox_ASIO_Input0_ControlMasterVolume.Text = "Control Master Volume";
            this.checkBox_ASIO_Input0_ControlMasterVolume.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_Input0_ControlMasterVolume.CheckedChanged += new System.EventHandler(this.ASIO_Input0_MasterVolume);
            // 
            // label_ASIO_Input0_MaxVolume
            // 
            this.label_ASIO_Input0_MaxVolume.AutoSize = true;
            this.label_ASIO_Input0_MaxVolume.Location = new System.Drawing.Point(250, 65);
            this.label_ASIO_Input0_MaxVolume.Name = "label_ASIO_Input0_MaxVolume";
            this.label_ASIO_Input0_MaxVolume.Size = new System.Drawing.Size(68, 13);
            this.label_ASIO_Input0_MaxVolume.TabIndex = 6;
            this.label_ASIO_Input0_MaxVolume.Text = "Max Volume:";
            this.label_ASIO_Input0_MaxVolume.Visible = false;
            // 
            // nUpDown_ASIO_Input0_MaxVolume
            // 
            this.nUpDown_ASIO_Input0_MaxVolume.Location = new System.Drawing.Point(338, 63);
            this.nUpDown_ASIO_Input0_MaxVolume.Name = "nUpDown_ASIO_Input0_MaxVolume";
            this.nUpDown_ASIO_Input0_MaxVolume.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_Input0_MaxVolume.TabIndex = 5;
            this.nUpDown_ASIO_Input0_MaxVolume.Visible = false;
            this.nUpDown_ASIO_Input0_MaxVolume.ValueChanged += new System.EventHandler(this.ASIO_Input0_MaxVolume);
            // 
            // nUpDown_ASIO_Input0_Channel
            // 
            this.nUpDown_ASIO_Input0_Channel.Location = new System.Drawing.Point(338, 28);
            this.nUpDown_ASIO_Input0_Channel.Maximum = new decimal(new int[] {
            64,
            0,
            0,
            0});
            this.nUpDown_ASIO_Input0_Channel.Name = "nUpDown_ASIO_Input0_Channel";
            this.nUpDown_ASIO_Input0_Channel.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_Input0_Channel.TabIndex = 2;
            this.nUpDown_ASIO_Input0_Channel.ValueChanged += new System.EventHandler(this.ASIO_Input0_Channel);
            // 
            // label_ASIO_Input0_Channel
            // 
            this.label_ASIO_Input0_Channel.AutoSize = true;
            this.label_ASIO_Input0_Channel.Location = new System.Drawing.Point(250, 30);
            this.label_ASIO_Input0_Channel.Name = "label_ASIO_Input0_Channel";
            this.label_ASIO_Input0_Channel.Size = new System.Drawing.Size(49, 13);
            this.label_ASIO_Input0_Channel.TabIndex = 1;
            this.label_ASIO_Input0_Channel.Text = "Channel:";
            this.label_ASIO_Input0_Channel.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // listBox_AvailableASIODevices_Input0
            // 
            this.listBox_AvailableASIODevices_Input0.FormattingEnabled = true;
            this.listBox_AvailableASIODevices_Input0.Location = new System.Drawing.Point(23, 30);
            this.listBox_AvailableASIODevices_Input0.Name = "listBox_AvailableASIODevices_Input0";
            this.listBox_AvailableASIODevices_Input0.Size = new System.Drawing.Size(193, 147);
            this.listBox_AvailableASIODevices_Input0.TabIndex = 0;
            this.listBox_AvailableASIODevices_Input0.SelectedIndexChanged += new System.EventHandler(this.ASIO_ListAvailableInput0);
            this.listBox_AvailableASIODevices_Input0.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_ASIO_Config
            // 
            this.groupBox_ASIO_Config.Controls.Add(this.checkBox_ASIO_ASIO);
            this.groupBox_ASIO_Config.Controls.Add(this.checkBox_ASIO_WASAPI_Input);
            this.groupBox_ASIO_Config.Controls.Add(this.checkBox_ASIO_WASAPI_Output);
            this.groupBox_ASIO_Config.Location = new System.Drawing.Point(966, 216);
            this.groupBox_ASIO_Config.Name = "groupBox_ASIO_Config";
            this.groupBox_ASIO_Config.Size = new System.Drawing.Size(161, 111);
            this.groupBox_ASIO_Config.TabIndex = 2;
            this.groupBox_ASIO_Config.TabStop = false;
            this.groupBox_ASIO_Config.Text = "Config";
            this.groupBox_ASIO_Config.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ASIO_ASIO
            // 
            this.checkBox_ASIO_ASIO.AutoSize = true;
            this.checkBox_ASIO_ASIO.Location = new System.Drawing.Point(6, 80);
            this.checkBox_ASIO_ASIO.Name = "checkBox_ASIO_ASIO";
            this.checkBox_ASIO_ASIO.Size = new System.Drawing.Size(73, 17);
            this.checkBox_ASIO_ASIO.TabIndex = 2;
            this.checkBox_ASIO_ASIO.Text = "Use ASIO";
            this.checkBox_ASIO_ASIO.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_ASIO.CheckedChanged += new System.EventHandler(this.ASIO_ASIO);
            this.checkBox_ASIO_ASIO.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ASIO_WASAPI_Input
            // 
            this.checkBox_ASIO_WASAPI_Input.AutoSize = true;
            this.checkBox_ASIO_WASAPI_Input.Location = new System.Drawing.Point(6, 50);
            this.checkBox_ASIO_WASAPI_Input.Name = "checkBox_ASIO_WASAPI_Input";
            this.checkBox_ASIO_WASAPI_Input.Size = new System.Drawing.Size(117, 17);
            this.checkBox_ASIO_WASAPI_Input.TabIndex = 1;
            this.checkBox_ASIO_WASAPI_Input.Text = "Use WASAPI Input";
            this.checkBox_ASIO_WASAPI_Input.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_WASAPI_Input.CheckedChanged += new System.EventHandler(this.ASIO_WASAPI_Input);
            this.checkBox_ASIO_WASAPI_Input.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_ASIO_WASAPI_Output
            // 
            this.checkBox_ASIO_WASAPI_Output.AutoSize = true;
            this.checkBox_ASIO_WASAPI_Output.Location = new System.Drawing.Point(6, 19);
            this.checkBox_ASIO_WASAPI_Output.Name = "checkBox_ASIO_WASAPI_Output";
            this.checkBox_ASIO_WASAPI_Output.Size = new System.Drawing.Size(125, 17);
            this.checkBox_ASIO_WASAPI_Output.TabIndex = 0;
            this.checkBox_ASIO_WASAPI_Output.Text = "Use WASAPI Output";
            this.checkBox_ASIO_WASAPI_Output.UseVisualStyleBackColor = true;
            this.checkBox_ASIO_WASAPI_Output.CheckedChanged += new System.EventHandler(this.ASIO_WASAPI_Output);
            this.checkBox_ASIO_WASAPI_Output.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_ASIO_BufferSize
            // 
            this.groupBox_ASIO_BufferSize.Controls.Add(this.label_ASIO_CustomBufferSize);
            this.groupBox_ASIO_BufferSize.Controls.Add(this.nUpDown_ASIO_CustomBufferSize);
            this.groupBox_ASIO_BufferSize.Controls.Add(this.radio_ASIO_BufferSize_Custom);
            this.groupBox_ASIO_BufferSize.Controls.Add(this.radio_ASIO_BufferSize_Host);
            this.groupBox_ASIO_BufferSize.Controls.Add(this.radio_ASIO_BufferSize_Driver);
            this.groupBox_ASIO_BufferSize.Location = new System.Drawing.Point(966, 24);
            this.groupBox_ASIO_BufferSize.Name = "groupBox_ASIO_BufferSize";
            this.groupBox_ASIO_BufferSize.Size = new System.Drawing.Size(161, 177);
            this.groupBox_ASIO_BufferSize.TabIndex = 1;
            this.groupBox_ASIO_BufferSize.TabStop = false;
            this.groupBox_ASIO_BufferSize.Text = "Buffer Size";
            this.groupBox_ASIO_BufferSize.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_ASIO_CustomBufferSize
            // 
            this.label_ASIO_CustomBufferSize.AutoSize = true;
            this.label_ASIO_CustomBufferSize.Location = new System.Drawing.Point(28, 110);
            this.label_ASIO_CustomBufferSize.Name = "label_ASIO_CustomBufferSize";
            this.label_ASIO_CustomBufferSize.Size = new System.Drawing.Size(96, 13);
            this.label_ASIO_CustomBufferSize.TabIndex = 4;
            this.label_ASIO_CustomBufferSize.Text = "Custom Buffer Size";
            this.label_ASIO_CustomBufferSize.Visible = false;
            this.label_ASIO_CustomBufferSize.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // nUpDown_ASIO_CustomBufferSize
            // 
            this.nUpDown_ASIO_CustomBufferSize.Increment = new decimal(new int[] {
            16,
            0,
            0,
            0});
            this.nUpDown_ASIO_CustomBufferSize.Location = new System.Drawing.Point(20, 129);
            this.nUpDown_ASIO_CustomBufferSize.Maximum = new decimal(new int[] {
            496,
            0,
            0,
            0});
            this.nUpDown_ASIO_CustomBufferSize.Minimum = new decimal(new int[] {
            16,
            0,
            0,
            0});
            this.nUpDown_ASIO_CustomBufferSize.Name = "nUpDown_ASIO_CustomBufferSize";
            this.nUpDown_ASIO_CustomBufferSize.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_ASIO_CustomBufferSize.TabIndex = 3;
            this.nUpDown_ASIO_CustomBufferSize.Value = new decimal(new int[] {
            16,
            0,
            0,
            0});
            this.nUpDown_ASIO_CustomBufferSize.Visible = false;
            this.nUpDown_ASIO_CustomBufferSize.ValueChanged += new System.EventHandler(this.ASIO_CustomBufferSize);
            // 
            // radio_ASIO_BufferSize_Custom
            // 
            this.radio_ASIO_BufferSize_Custom.AutoSize = true;
            this.radio_ASIO_BufferSize_Custom.Location = new System.Drawing.Point(22, 80);
            this.radio_ASIO_BufferSize_Custom.Name = "radio_ASIO_BufferSize_Custom";
            this.radio_ASIO_BufferSize_Custom.Size = new System.Drawing.Size(60, 17);
            this.radio_ASIO_BufferSize_Custom.TabIndex = 2;
            this.radio_ASIO_BufferSize_Custom.TabStop = true;
            this.radio_ASIO_BufferSize_Custom.Text = "Custom";
            this.radio_ASIO_BufferSize_Custom.UseVisualStyleBackColor = true;
            this.radio_ASIO_BufferSize_Custom.CheckedChanged += new System.EventHandler(this.ASIO_BufferSize_Custom);
            this.radio_ASIO_BufferSize_Custom.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_ASIO_BufferSize_Host
            // 
            this.radio_ASIO_BufferSize_Host.AutoSize = true;
            this.radio_ASIO_BufferSize_Host.Location = new System.Drawing.Point(22, 57);
            this.radio_ASIO_BufferSize_Host.Name = "radio_ASIO_BufferSize_Host";
            this.radio_ASIO_BufferSize_Host.Size = new System.Drawing.Size(47, 17);
            this.radio_ASIO_BufferSize_Host.TabIndex = 1;
            this.radio_ASIO_BufferSize_Host.TabStop = true;
            this.radio_ASIO_BufferSize_Host.Text = "Host";
            this.radio_ASIO_BufferSize_Host.UseVisualStyleBackColor = true;
            this.radio_ASIO_BufferSize_Host.CheckedChanged += new System.EventHandler(this.ASIO_BufferSize_Host);
            this.radio_ASIO_BufferSize_Host.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_ASIO_BufferSize_Driver
            // 
            this.radio_ASIO_BufferSize_Driver.AutoSize = true;
            this.radio_ASIO_BufferSize_Driver.Location = new System.Drawing.Point(22, 34);
            this.radio_ASIO_BufferSize_Driver.Name = "radio_ASIO_BufferSize_Driver";
            this.radio_ASIO_BufferSize_Driver.Size = new System.Drawing.Size(53, 17);
            this.radio_ASIO_BufferSize_Driver.TabIndex = 0;
            this.radio_ASIO_BufferSize_Driver.TabStop = true;
            this.radio_ASIO_BufferSize_Driver.Text = "Driver";
            this.radio_ASIO_BufferSize_Driver.UseVisualStyleBackColor = true;
            this.radio_ASIO_BufferSize_Driver.CheckedChanged += new System.EventHandler(this.ASIO_BufferSize_Driver);
            this.radio_ASIO_BufferSize_Driver.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // tab_Rocksmith
            // 
            this.tab_Rocksmith.BackColor = System.Drawing.Color.Azure;
            this.tab_Rocksmith.Controls.Add(this.label_Rocksmith_Thanks);
            this.tab_Rocksmith.Controls.Add(this.groupBox_Rocksmith_AudioSettings);
            this.tab_Rocksmith.Controls.Add(this.groupBox_Rocksmith_VisualSettings);
            this.tab_Rocksmith.Location = new System.Drawing.Point(4, 22);
            this.tab_Rocksmith.Name = "tab_Rocksmith";
            this.tab_Rocksmith.Size = new System.Drawing.Size(1150, 513);
            this.tab_Rocksmith.TabIndex = 8;
            this.tab_Rocksmith.Text = "Rocksmith Settings";
            // 
            // label_Rocksmith_Thanks
            // 
            this.label_Rocksmith_Thanks.AutoSize = true;
            this.label_Rocksmith_Thanks.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F);
            this.label_Rocksmith_Thanks.Location = new System.Drawing.Point(564, 468);
            this.label_Rocksmith_Thanks.Name = "label_Rocksmith_Thanks";
            this.label_Rocksmith_Thanks.Size = new System.Drawing.Size(574, 34);
            this.label_Rocksmith_Thanks.TabIndex = 4;
            this.label_Rocksmith_Thanks.Text = "Thank you Ubisoft San Fransisco for creating Rocksmith 1, and Rocksmith 2014.\r\nMo" +
    "st of the tooltips from this section are from their documentation provided with " +
    "the game.";
            // 
            // groupBox_Rocksmith_AudioSettings
            // 
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.checkBox_Rocksmith_LowLatencyMode);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.checkBox_Rocksmith_RTCOnly);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.checkBox_Rocksmith_DumpAudioLog);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.checkBox_Rocksmith_ForceDirextXSink);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.checkBox_Rocksmith_ForceWDM);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.checkBox_Rocksmith_ExclusiveMode);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.checkBox_Rocksmith_EnableMicrophone);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.checkBox_Rocksmith_Override_MaxOutputBufferSize);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.label_Rocksmith_MaxOutputBuffer);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.label_Rocksmith_LatencyBuffer);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.nUpDown_Rocksmith_MaxOutputBuffer);
            this.groupBox_Rocksmith_AudioSettings.Controls.Add(this.nUpDown_Rocksmith_LatencyBuffer);
            this.groupBox_Rocksmith_AudioSettings.Location = new System.Drawing.Point(524, 26);
            this.groupBox_Rocksmith_AudioSettings.Name = "groupBox_Rocksmith_AudioSettings";
            this.groupBox_Rocksmith_AudioSettings.Size = new System.Drawing.Size(304, 224);
            this.groupBox_Rocksmith_AudioSettings.TabIndex = 3;
            this.groupBox_Rocksmith_AudioSettings.TabStop = false;
            this.groupBox_Rocksmith_AudioSettings.Text = "Audio Settings";
            // 
            // checkBox_Rocksmith_LowLatencyMode
            // 
            this.checkBox_Rocksmith_LowLatencyMode.AutoSize = true;
            this.checkBox_Rocksmith_LowLatencyMode.Location = new System.Drawing.Point(156, 189);
            this.checkBox_Rocksmith_LowLatencyMode.Name = "checkBox_Rocksmith_LowLatencyMode";
            this.checkBox_Rocksmith_LowLatencyMode.Size = new System.Drawing.Size(117, 17);
            this.checkBox_Rocksmith_LowLatencyMode.TabIndex = 15;
            this.checkBox_Rocksmith_LowLatencyMode.Text = "Low Latency Mode";
            this.checkBox_Rocksmith_LowLatencyMode.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_LowLatencyMode.CheckedChanged += new System.EventHandler(this.Rocksmith_LowLatencyMode);
            this.checkBox_Rocksmith_LowLatencyMode.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_RTCOnly
            // 
            this.checkBox_Rocksmith_RTCOnly.AutoSize = true;
            this.checkBox_Rocksmith_RTCOnly.Location = new System.Drawing.Point(156, 166);
            this.checkBox_Rocksmith_RTCOnly.Name = "checkBox_Rocksmith_RTCOnly";
            this.checkBox_Rocksmith_RTCOnly.Size = new System.Drawing.Size(130, 17);
            this.checkBox_Rocksmith_RTCOnly.TabIndex = 14;
            this.checkBox_Rocksmith_RTCOnly.Text = "Real Tone Cable Only";
            this.checkBox_Rocksmith_RTCOnly.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_RTCOnly.CheckedChanged += new System.EventHandler(this.Rocksmith_RTCOnly);
            this.checkBox_Rocksmith_RTCOnly.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_DumpAudioLog
            // 
            this.checkBox_Rocksmith_DumpAudioLog.AutoSize = true;
            this.checkBox_Rocksmith_DumpAudioLog.Location = new System.Drawing.Point(156, 143);
            this.checkBox_Rocksmith_DumpAudioLog.Name = "checkBox_Rocksmith_DumpAudioLog";
            this.checkBox_Rocksmith_DumpAudioLog.Size = new System.Drawing.Size(105, 17);
            this.checkBox_Rocksmith_DumpAudioLog.TabIndex = 13;
            this.checkBox_Rocksmith_DumpAudioLog.Text = "Dump Audio Log";
            this.checkBox_Rocksmith_DumpAudioLog.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_DumpAudioLog.CheckedChanged += new System.EventHandler(this.Rocksmith_DumpAudioLog);
            this.checkBox_Rocksmith_DumpAudioLog.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_ForceDirextXSink
            // 
            this.checkBox_Rocksmith_ForceDirextXSink.AutoSize = true;
            this.checkBox_Rocksmith_ForceDirextXSink.Location = new System.Drawing.Point(10, 189);
            this.checkBox_Rocksmith_ForceDirextXSink.Name = "checkBox_Rocksmith_ForceDirextXSink";
            this.checkBox_Rocksmith_ForceDirextXSink.Size = new System.Drawing.Size(112, 17);
            this.checkBox_Rocksmith_ForceDirextXSink.TabIndex = 12;
            this.checkBox_Rocksmith_ForceDirextXSink.Text = "Force DirectXSink";
            this.checkBox_Rocksmith_ForceDirextXSink.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_ForceDirextXSink.CheckedChanged += new System.EventHandler(this.Rocksmith_ForceDirextXSink);
            this.checkBox_Rocksmith_ForceDirextXSink.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_ForceWDM
            // 
            this.checkBox_Rocksmith_ForceWDM.AutoSize = true;
            this.checkBox_Rocksmith_ForceWDM.Location = new System.Drawing.Point(9, 166);
            this.checkBox_Rocksmith_ForceWDM.Name = "checkBox_Rocksmith_ForceWDM";
            this.checkBox_Rocksmith_ForceWDM.Size = new System.Drawing.Size(84, 17);
            this.checkBox_Rocksmith_ForceWDM.TabIndex = 11;
            this.checkBox_Rocksmith_ForceWDM.Text = "Force WDM";
            this.checkBox_Rocksmith_ForceWDM.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_ForceWDM.CheckedChanged += new System.EventHandler(this.Rocksmith_ForceWDM);
            this.checkBox_Rocksmith_ForceWDM.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_ExclusiveMode
            // 
            this.checkBox_Rocksmith_ExclusiveMode.AutoSize = true;
            this.checkBox_Rocksmith_ExclusiveMode.Location = new System.Drawing.Point(10, 143);
            this.checkBox_Rocksmith_ExclusiveMode.Name = "checkBox_Rocksmith_ExclusiveMode";
            this.checkBox_Rocksmith_ExclusiveMode.Size = new System.Drawing.Size(101, 17);
            this.checkBox_Rocksmith_ExclusiveMode.TabIndex = 10;
            this.checkBox_Rocksmith_ExclusiveMode.Text = "Exclusive Mode";
            this.checkBox_Rocksmith_ExclusiveMode.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_ExclusiveMode.CheckedChanged += new System.EventHandler(this.Rocksmith_ExclusiveMode);
            this.checkBox_Rocksmith_ExclusiveMode.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_EnableMicrophone
            // 
            this.checkBox_Rocksmith_EnableMicrophone.AutoSize = true;
            this.checkBox_Rocksmith_EnableMicrophone.Location = new System.Drawing.Point(10, 120);
            this.checkBox_Rocksmith_EnableMicrophone.Name = "checkBox_Rocksmith_EnableMicrophone";
            this.checkBox_Rocksmith_EnableMicrophone.Size = new System.Drawing.Size(118, 17);
            this.checkBox_Rocksmith_EnableMicrophone.TabIndex = 9;
            this.checkBox_Rocksmith_EnableMicrophone.Text = "Enable Microphone";
            this.checkBox_Rocksmith_EnableMicrophone.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_EnableMicrophone.CheckedChanged += new System.EventHandler(this.Rocksmith_EnableMicrophone);
            this.checkBox_Rocksmith_EnableMicrophone.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_Override_MaxOutputBufferSize
            // 
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize.AutoSize = true;
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize.Location = new System.Drawing.Point(156, 120);
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize.Name = "checkBox_Rocksmith_Override_MaxOutputBufferSize";
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize.Size = new System.Drawing.Size(137, 17);
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize.TabIndex = 8;
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize.Text = "Automate Output Buffer";
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_Override_MaxOutputBufferSize.CheckedChanged += new System.EventHandler(this.Rocksmith_AutomateMaxBufferSize);
            // 
            // label_Rocksmith_MaxOutputBuffer
            // 
            this.label_Rocksmith_MaxOutputBuffer.AutoSize = true;
            this.label_Rocksmith_MaxOutputBuffer.Location = new System.Drawing.Point(6, 57);
            this.label_Rocksmith_MaxOutputBuffer.Name = "label_Rocksmith_MaxOutputBuffer";
            this.label_Rocksmith_MaxOutputBuffer.Size = new System.Drawing.Size(99, 13);
            this.label_Rocksmith_MaxOutputBuffer.TabIndex = 7;
            this.label_Rocksmith_MaxOutputBuffer.Text = "Max Output Buffer: ";
            this.label_Rocksmith_MaxOutputBuffer.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_Rocksmith_LatencyBuffer
            // 
            this.label_Rocksmith_LatencyBuffer.AutoSize = true;
            this.label_Rocksmith_LatencyBuffer.Location = new System.Drawing.Point(7, 28);
            this.label_Rocksmith_LatencyBuffer.Name = "label_Rocksmith_LatencyBuffer";
            this.label_Rocksmith_LatencyBuffer.Size = new System.Drawing.Size(79, 13);
            this.label_Rocksmith_LatencyBuffer.TabIndex = 6;
            this.label_Rocksmith_LatencyBuffer.Text = "Latency Buffer:";
            this.label_Rocksmith_LatencyBuffer.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // nUpDown_Rocksmith_MaxOutputBuffer
            // 
            this.nUpDown_Rocksmith_MaxOutputBuffer.Increment = new decimal(new int[] {
            32,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_MaxOutputBuffer.Location = new System.Drawing.Point(111, 55);
            this.nUpDown_Rocksmith_MaxOutputBuffer.Maximum = new decimal(new int[] {
            1000000,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_MaxOutputBuffer.Name = "nUpDown_Rocksmith_MaxOutputBuffer";
            this.nUpDown_Rocksmith_MaxOutputBuffer.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_Rocksmith_MaxOutputBuffer.TabIndex = 5;
            this.nUpDown_Rocksmith_MaxOutputBuffer.Value = new decimal(new int[] {
            32,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_MaxOutputBuffer.ValueChanged += new System.EventHandler(this.Rocksmith_MaxBufferSize);
            // 
            // nUpDown_Rocksmith_LatencyBuffer
            // 
            this.nUpDown_Rocksmith_LatencyBuffer.Location = new System.Drawing.Point(111, 26);
            this.nUpDown_Rocksmith_LatencyBuffer.Maximum = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_LatencyBuffer.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_LatencyBuffer.Name = "nUpDown_Rocksmith_LatencyBuffer";
            this.nUpDown_Rocksmith_LatencyBuffer.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_Rocksmith_LatencyBuffer.TabIndex = 4;
            this.nUpDown_Rocksmith_LatencyBuffer.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_LatencyBuffer.ValueChanged += new System.EventHandler(this.Rocksmith_LatencyBuffer);
            // 
            // groupBox_Rocksmith_VisualSettings
            // 
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_EnableRenderRes);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.groupBox_Rocksmith_Network);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_MSAASamples);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_PostEffects);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_PerPixelLighting);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_DepthOfField);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_HighResScope);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_Shadows);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_GamepadUI);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.checkBox_Rocksmith_DisableBrowser);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.groupBox_Rocksmith_Fullscreen);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.groupBox_Rocksmith_VisualQuality);
            this.groupBox_Rocksmith_VisualSettings.Controls.Add(this.groupBox_Rocksmith_Resolutions);
            this.groupBox_Rocksmith_VisualSettings.Location = new System.Drawing.Point(35, 26);
            this.groupBox_Rocksmith_VisualSettings.Name = "groupBox_Rocksmith_VisualSettings";
            this.groupBox_Rocksmith_VisualSettings.Size = new System.Drawing.Size(483, 294);
            this.groupBox_Rocksmith_VisualSettings.TabIndex = 2;
            this.groupBox_Rocksmith_VisualSettings.TabStop = false;
            this.groupBox_Rocksmith_VisualSettings.Text = "Visual Settings";
            // 
            // checkBox_Rocksmith_EnableRenderRes
            // 
            this.checkBox_Rocksmith_EnableRenderRes.AutoSize = true;
            this.checkBox_Rocksmith_EnableRenderRes.Location = new System.Drawing.Point(304, 204);
            this.checkBox_Rocksmith_EnableRenderRes.Name = "checkBox_Rocksmith_EnableRenderRes";
            this.checkBox_Rocksmith_EnableRenderRes.Size = new System.Drawing.Size(160, 17);
            this.checkBox_Rocksmith_EnableRenderRes.TabIndex = 11;
            this.checkBox_Rocksmith_EnableRenderRes.Text = "Seperate Render Resolution";
            this.checkBox_Rocksmith_EnableRenderRes.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_EnableRenderRes.CheckedChanged += new System.EventHandler(this.Rocksmith_EnableRenderRes);
            // 
            // groupBox_Rocksmith_Network
            // 
            this.groupBox_Rocksmith_Network.Controls.Add(this.checkBox_Rocksmith_UseProxy);
            this.groupBox_Rocksmith_Network.Location = new System.Drawing.Point(304, 229);
            this.groupBox_Rocksmith_Network.Name = "groupBox_Rocksmith_Network";
            this.groupBox_Rocksmith_Network.Size = new System.Drawing.Size(105, 59);
            this.groupBox_Rocksmith_Network.TabIndex = 4;
            this.groupBox_Rocksmith_Network.TabStop = false;
            this.groupBox_Rocksmith_Network.Text = "Network Settings";
            // 
            // checkBox_Rocksmith_UseProxy
            // 
            this.checkBox_Rocksmith_UseProxy.AutoSize = true;
            this.checkBox_Rocksmith_UseProxy.Location = new System.Drawing.Point(16, 32);
            this.checkBox_Rocksmith_UseProxy.Name = "checkBox_Rocksmith_UseProxy";
            this.checkBox_Rocksmith_UseProxy.Size = new System.Drawing.Size(74, 17);
            this.checkBox_Rocksmith_UseProxy.TabIndex = 0;
            this.checkBox_Rocksmith_UseProxy.Text = "Use Proxy";
            this.checkBox_Rocksmith_UseProxy.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_UseProxy.CheckedChanged += new System.EventHandler(this.Rocksmith_UseProxy);
            this.checkBox_Rocksmith_UseProxy.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_MSAASamples
            // 
            this.checkBox_Rocksmith_MSAASamples.AutoSize = true;
            this.checkBox_Rocksmith_MSAASamples.Location = new System.Drawing.Point(304, 157);
            this.checkBox_Rocksmith_MSAASamples.Name = "checkBox_Rocksmith_MSAASamples";
            this.checkBox_Rocksmith_MSAASamples.Size = new System.Drawing.Size(83, 17);
            this.checkBox_Rocksmith_MSAASamples.TabIndex = 10;
            this.checkBox_Rocksmith_MSAASamples.Text = "Anti-Aliasing";
            this.checkBox_Rocksmith_MSAASamples.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_MSAASamples.CheckedChanged += new System.EventHandler(this.Rocksmith_MSAA);
            this.checkBox_Rocksmith_MSAASamples.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_PostEffects
            // 
            this.checkBox_Rocksmith_PostEffects.AutoSize = true;
            this.checkBox_Rocksmith_PostEffects.Location = new System.Drawing.Point(304, 42);
            this.checkBox_Rocksmith_PostEffects.Name = "checkBox_Rocksmith_PostEffects";
            this.checkBox_Rocksmith_PostEffects.Size = new System.Drawing.Size(138, 17);
            this.checkBox_Rocksmith_PostEffects.TabIndex = 5;
            this.checkBox_Rocksmith_PostEffects.Text = "Post Processing Effects";
            this.checkBox_Rocksmith_PostEffects.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_PostEffects.CheckedChanged += new System.EventHandler(this.Rocksmith_PostEffects);
            this.checkBox_Rocksmith_PostEffects.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_PerPixelLighting
            // 
            this.checkBox_Rocksmith_PerPixelLighting.AutoSize = true;
            this.checkBox_Rocksmith_PerPixelLighting.Location = new System.Drawing.Point(304, 134);
            this.checkBox_Rocksmith_PerPixelLighting.Name = "checkBox_Rocksmith_PerPixelLighting";
            this.checkBox_Rocksmith_PerPixelLighting.Size = new System.Drawing.Size(125, 17);
            this.checkBox_Rocksmith_PerPixelLighting.TabIndex = 9;
            this.checkBox_Rocksmith_PerPixelLighting.Text = "Pixel Perfect Lighting";
            this.checkBox_Rocksmith_PerPixelLighting.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_PerPixelLighting.CheckedChanged += new System.EventHandler(this.Rocksmith_PerPixelLighting);
            this.checkBox_Rocksmith_PerPixelLighting.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_DepthOfField
            // 
            this.checkBox_Rocksmith_DepthOfField.AutoSize = true;
            this.checkBox_Rocksmith_DepthOfField.Location = new System.Drawing.Point(304, 111);
            this.checkBox_Rocksmith_DepthOfField.Name = "checkBox_Rocksmith_DepthOfField";
            this.checkBox_Rocksmith_DepthOfField.Size = new System.Drawing.Size(92, 17);
            this.checkBox_Rocksmith_DepthOfField.TabIndex = 8;
            this.checkBox_Rocksmith_DepthOfField.Text = "Depth of Field";
            this.checkBox_Rocksmith_DepthOfField.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_DepthOfField.CheckedChanged += new System.EventHandler(this.Rocksmith_DepthOfField);
            this.checkBox_Rocksmith_DepthOfField.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_HighResScope
            // 
            this.checkBox_Rocksmith_HighResScope.AutoSize = true;
            this.checkBox_Rocksmith_HighResScope.Location = new System.Drawing.Point(304, 88);
            this.checkBox_Rocksmith_HighResScope.Name = "checkBox_Rocksmith_HighResScope";
            this.checkBox_Rocksmith_HighResScope.Size = new System.Drawing.Size(178, 17);
            this.checkBox_Rocksmith_HighResScope.TabIndex = 7;
            this.checkBox_Rocksmith_HighResScope.Text = "High Resolution Audio Visualizer";
            this.checkBox_Rocksmith_HighResScope.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_HighResScope.CheckedChanged += new System.EventHandler(this.Rocksmith_HighResScope);
            this.checkBox_Rocksmith_HighResScope.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_Shadows
            // 
            this.checkBox_Rocksmith_Shadows.AutoSize = true;
            this.checkBox_Rocksmith_Shadows.Location = new System.Drawing.Point(304, 65);
            this.checkBox_Rocksmith_Shadows.Name = "checkBox_Rocksmith_Shadows";
            this.checkBox_Rocksmith_Shadows.Size = new System.Drawing.Size(114, 17);
            this.checkBox_Rocksmith_Shadows.TabIndex = 6;
            this.checkBox_Rocksmith_Shadows.Text = "Realtime Shadows";
            this.checkBox_Rocksmith_Shadows.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_Shadows.CheckedChanged += new System.EventHandler(this.Rocksmith_Shadows);
            this.checkBox_Rocksmith_Shadows.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_GamepadUI
            // 
            this.checkBox_Rocksmith_GamepadUI.AutoSize = true;
            this.checkBox_Rocksmith_GamepadUI.Location = new System.Drawing.Point(304, 19);
            this.checkBox_Rocksmith_GamepadUI.Name = "checkBox_Rocksmith_GamepadUI";
            this.checkBox_Rocksmith_GamepadUI.Size = new System.Drawing.Size(116, 17);
            this.checkBox_Rocksmith_GamepadUI.TabIndex = 4;
            this.checkBox_Rocksmith_GamepadUI.Text = "Show Gamepad UI";
            this.checkBox_Rocksmith_GamepadUI.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_GamepadUI.CheckedChanged += new System.EventHandler(this.Rocksmith_GamepadUI);
            this.checkBox_Rocksmith_GamepadUI.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // checkBox_Rocksmith_DisableBrowser
            // 
            this.checkBox_Rocksmith_DisableBrowser.AutoSize = true;
            this.checkBox_Rocksmith_DisableBrowser.Location = new System.Drawing.Point(304, 180);
            this.checkBox_Rocksmith_DisableBrowser.Name = "checkBox_Rocksmith_DisableBrowser";
            this.checkBox_Rocksmith_DisableBrowser.Size = new System.Drawing.Size(102, 17);
            this.checkBox_Rocksmith_DisableBrowser.TabIndex = 0;
            this.checkBox_Rocksmith_DisableBrowser.Text = "Disable Browser";
            this.checkBox_Rocksmith_DisableBrowser.UseVisualStyleBackColor = true;
            this.checkBox_Rocksmith_DisableBrowser.CheckedChanged += new System.EventHandler(this.Rocksmith_DisableBrowser);
            this.checkBox_Rocksmith_DisableBrowser.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // groupBox_Rocksmith_Fullscreen
            // 
            this.groupBox_Rocksmith_Fullscreen.Controls.Add(this.radio_Rocksmith_ExclusiveFullScreen);
            this.groupBox_Rocksmith_Fullscreen.Controls.Add(this.radio_Rocksmith_NonExclusiveFullScreen);
            this.groupBox_Rocksmith_Fullscreen.Controls.Add(this.radio_Rocksmith_Windowed);
            this.groupBox_Rocksmith_Fullscreen.Location = new System.Drawing.Point(128, 186);
            this.groupBox_Rocksmith_Fullscreen.Name = "groupBox_Rocksmith_Fullscreen";
            this.groupBox_Rocksmith_Fullscreen.Size = new System.Drawing.Size(161, 100);
            this.groupBox_Rocksmith_Fullscreen.TabIndex = 3;
            this.groupBox_Rocksmith_Fullscreen.TabStop = false;
            this.groupBox_Rocksmith_Fullscreen.Text = "Fullscreen";
            this.groupBox_Rocksmith_Fullscreen.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_Rocksmith_ExclusiveFullScreen
            // 
            this.radio_Rocksmith_ExclusiveFullScreen.AutoSize = true;
            this.radio_Rocksmith_ExclusiveFullScreen.Location = new System.Drawing.Point(6, 73);
            this.radio_Rocksmith_ExclusiveFullScreen.Name = "radio_Rocksmith_ExclusiveFullScreen";
            this.radio_Rocksmith_ExclusiveFullScreen.Size = new System.Drawing.Size(121, 17);
            this.radio_Rocksmith_ExclusiveFullScreen.TabIndex = 2;
            this.radio_Rocksmith_ExclusiveFullScreen.TabStop = true;
            this.radio_Rocksmith_ExclusiveFullScreen.Text = "Exclusive Fullscreen";
            this.radio_Rocksmith_ExclusiveFullScreen.UseVisualStyleBackColor = true;
            this.radio_Rocksmith_ExclusiveFullScreen.CheckedChanged += new System.EventHandler(this.Rocksmith_ExclusiveFullScreen);
            // 
            // radio_Rocksmith_NonExclusiveFullScreen
            // 
            this.radio_Rocksmith_NonExclusiveFullScreen.AutoSize = true;
            this.radio_Rocksmith_NonExclusiveFullScreen.Location = new System.Drawing.Point(6, 47);
            this.radio_Rocksmith_NonExclusiveFullScreen.Name = "radio_Rocksmith_NonExclusiveFullScreen";
            this.radio_Rocksmith_NonExclusiveFullScreen.Size = new System.Drawing.Size(144, 17);
            this.radio_Rocksmith_NonExclusiveFullScreen.TabIndex = 1;
            this.radio_Rocksmith_NonExclusiveFullScreen.TabStop = true;
            this.radio_Rocksmith_NonExclusiveFullScreen.Text = "Non-Exclusive Fullscreen";
            this.radio_Rocksmith_NonExclusiveFullScreen.UseVisualStyleBackColor = true;
            this.radio_Rocksmith_NonExclusiveFullScreen.CheckedChanged += new System.EventHandler(this.Rocksmith_NonExclusiveFullScreen);
            this.radio_Rocksmith_NonExclusiveFullScreen.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_Rocksmith_Windowed
            // 
            this.radio_Rocksmith_Windowed.AutoSize = true;
            this.radio_Rocksmith_Windowed.Location = new System.Drawing.Point(6, 21);
            this.radio_Rocksmith_Windowed.Name = "radio_Rocksmith_Windowed";
            this.radio_Rocksmith_Windowed.Size = new System.Drawing.Size(76, 17);
            this.radio_Rocksmith_Windowed.TabIndex = 0;
            this.radio_Rocksmith_Windowed.TabStop = true;
            this.radio_Rocksmith_Windowed.Text = "Windowed";
            this.radio_Rocksmith_Windowed.UseVisualStyleBackColor = true;
            this.radio_Rocksmith_Windowed.CheckedChanged += new System.EventHandler(this.Rocksmith_Windowed);
            // 
            // groupBox_Rocksmith_VisualQuality
            // 
            this.groupBox_Rocksmith_VisualQuality.Controls.Add(this.radio_Rocksmith_HighQuality);
            this.groupBox_Rocksmith_VisualQuality.Controls.Add(this.radio_Rocksmith_MediumQuality);
            this.groupBox_Rocksmith_VisualQuality.Controls.Add(this.radio_Rocksmith_LowQuality);
            this.groupBox_Rocksmith_VisualQuality.Location = new System.Drawing.Point(6, 186);
            this.groupBox_Rocksmith_VisualQuality.Name = "groupBox_Rocksmith_VisualQuality";
            this.groupBox_Rocksmith_VisualQuality.Size = new System.Drawing.Size(116, 100);
            this.groupBox_Rocksmith_VisualQuality.TabIndex = 3;
            this.groupBox_Rocksmith_VisualQuality.TabStop = false;
            this.groupBox_Rocksmith_VisualQuality.Text = "Visual Quality";
            this.groupBox_Rocksmith_VisualQuality.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // radio_Rocksmith_HighQuality
            // 
            this.radio_Rocksmith_HighQuality.AutoSize = true;
            this.radio_Rocksmith_HighQuality.Location = new System.Drawing.Point(6, 69);
            this.radio_Rocksmith_HighQuality.Name = "radio_Rocksmith_HighQuality";
            this.radio_Rocksmith_HighQuality.Size = new System.Drawing.Size(82, 17);
            this.radio_Rocksmith_HighQuality.TabIndex = 2;
            this.radio_Rocksmith_HighQuality.TabStop = true;
            this.radio_Rocksmith_HighQuality.Text = "High Quality";
            this.radio_Rocksmith_HighQuality.UseVisualStyleBackColor = true;
            this.radio_Rocksmith_HighQuality.CheckedChanged += new System.EventHandler(this.Rocksmith_HighQuality);
            // 
            // radio_Rocksmith_MediumQuality
            // 
            this.radio_Rocksmith_MediumQuality.AutoSize = true;
            this.radio_Rocksmith_MediumQuality.Location = new System.Drawing.Point(6, 45);
            this.radio_Rocksmith_MediumQuality.Name = "radio_Rocksmith_MediumQuality";
            this.radio_Rocksmith_MediumQuality.Size = new System.Drawing.Size(97, 17);
            this.radio_Rocksmith_MediumQuality.TabIndex = 1;
            this.radio_Rocksmith_MediumQuality.TabStop = true;
            this.radio_Rocksmith_MediumQuality.Text = "Medium Quality";
            this.radio_Rocksmith_MediumQuality.UseVisualStyleBackColor = true;
            this.radio_Rocksmith_MediumQuality.CheckedChanged += new System.EventHandler(this.Rocksmith_MediumQuality);
            // 
            // radio_Rocksmith_LowQuality
            // 
            this.radio_Rocksmith_LowQuality.AutoSize = true;
            this.radio_Rocksmith_LowQuality.Location = new System.Drawing.Point(6, 22);
            this.radio_Rocksmith_LowQuality.Name = "radio_Rocksmith_LowQuality";
            this.radio_Rocksmith_LowQuality.Size = new System.Drawing.Size(80, 17);
            this.radio_Rocksmith_LowQuality.TabIndex = 0;
            this.radio_Rocksmith_LowQuality.TabStop = true;
            this.radio_Rocksmith_LowQuality.Text = "Low Quality";
            this.radio_Rocksmith_LowQuality.UseVisualStyleBackColor = true;
            this.radio_Rocksmith_LowQuality.CheckedChanged += new System.EventHandler(this.Rocksmith_LowQuality);
            // 
            // groupBox_Rocksmith_Resolutions
            // 
            this.groupBox_Rocksmith_Resolutions.Controls.Add(this.label_Rocksmith_RenderHeight);
            this.groupBox_Rocksmith_Resolutions.Controls.Add(this.label_Rocksmith_RenderWidth);
            this.groupBox_Rocksmith_Resolutions.Controls.Add(this.label_Rocksmith_ScreenHeight);
            this.groupBox_Rocksmith_Resolutions.Controls.Add(this.label_Rocksmith_ScreenWidth);
            this.groupBox_Rocksmith_Resolutions.Controls.Add(this.nUpDown_Rocksmith_RenderHeight);
            this.groupBox_Rocksmith_Resolutions.Controls.Add(this.nUpDown_Rocksmith_RenderWidth);
            this.groupBox_Rocksmith_Resolutions.Controls.Add(this.nUpDown_Rocksmith_ScreenHeight);
            this.groupBox_Rocksmith_Resolutions.Controls.Add(this.nUpDown_Rocksmith_ScreenWidth);
            this.groupBox_Rocksmith_Resolutions.Location = new System.Drawing.Point(6, 19);
            this.groupBox_Rocksmith_Resolutions.Name = "groupBox_Rocksmith_Resolutions";
            this.groupBox_Rocksmith_Resolutions.Size = new System.Drawing.Size(236, 161);
            this.groupBox_Rocksmith_Resolutions.TabIndex = 1;
            this.groupBox_Rocksmith_Resolutions.TabStop = false;
            this.groupBox_Rocksmith_Resolutions.Text = "Resolutions";
            // 
            // label_Rocksmith_RenderHeight
            // 
            this.label_Rocksmith_RenderHeight.AutoSize = true;
            this.label_Rocksmith_RenderHeight.Location = new System.Drawing.Point(6, 132);
            this.label_Rocksmith_RenderHeight.Name = "label_Rocksmith_RenderHeight";
            this.label_Rocksmith_RenderHeight.Size = new System.Drawing.Size(79, 13);
            this.label_Rocksmith_RenderHeight.TabIndex = 7;
            this.label_Rocksmith_RenderHeight.Text = "Render Height:";
            this.label_Rocksmith_RenderHeight.Visible = false;
            this.label_Rocksmith_RenderHeight.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_Rocksmith_RenderWidth
            // 
            this.label_Rocksmith_RenderWidth.AutoSize = true;
            this.label_Rocksmith_RenderWidth.Location = new System.Drawing.Point(6, 106);
            this.label_Rocksmith_RenderWidth.Name = "label_Rocksmith_RenderWidth";
            this.label_Rocksmith_RenderWidth.Size = new System.Drawing.Size(76, 13);
            this.label_Rocksmith_RenderWidth.TabIndex = 6;
            this.label_Rocksmith_RenderWidth.Text = "Render Width:";
            this.label_Rocksmith_RenderWidth.Visible = false;
            this.label_Rocksmith_RenderWidth.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_Rocksmith_ScreenHeight
            // 
            this.label_Rocksmith_ScreenHeight.AutoSize = true;
            this.label_Rocksmith_ScreenHeight.Location = new System.Drawing.Point(6, 59);
            this.label_Rocksmith_ScreenHeight.Name = "label_Rocksmith_ScreenHeight";
            this.label_Rocksmith_ScreenHeight.Size = new System.Drawing.Size(78, 13);
            this.label_Rocksmith_ScreenHeight.TabIndex = 5;
            this.label_Rocksmith_ScreenHeight.Text = "Screen Height:";
            this.label_Rocksmith_ScreenHeight.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_Rocksmith_ScreenWidth
            // 
            this.label_Rocksmith_ScreenWidth.AutoSize = true;
            this.label_Rocksmith_ScreenWidth.Location = new System.Drawing.Point(6, 33);
            this.label_Rocksmith_ScreenWidth.Name = "label_Rocksmith_ScreenWidth";
            this.label_Rocksmith_ScreenWidth.Size = new System.Drawing.Size(75, 13);
            this.label_Rocksmith_ScreenWidth.TabIndex = 4;
            this.label_Rocksmith_ScreenWidth.Text = "Screen Width:";
            this.label_Rocksmith_ScreenWidth.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // nUpDown_Rocksmith_RenderHeight
            // 
            this.nUpDown_Rocksmith_RenderHeight.Location = new System.Drawing.Point(103, 130);
            this.nUpDown_Rocksmith_RenderHeight.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_RenderHeight.Name = "nUpDown_Rocksmith_RenderHeight";
            this.nUpDown_Rocksmith_RenderHeight.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_Rocksmith_RenderHeight.TabIndex = 3;
            this.nUpDown_Rocksmith_RenderHeight.Visible = false;
            this.nUpDown_Rocksmith_RenderHeight.ValueChanged += new System.EventHandler(this.Rocksmith_RenderHeight);
            // 
            // nUpDown_Rocksmith_RenderWidth
            // 
            this.nUpDown_Rocksmith_RenderWidth.Location = new System.Drawing.Point(103, 104);
            this.nUpDown_Rocksmith_RenderWidth.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_RenderWidth.Name = "nUpDown_Rocksmith_RenderWidth";
            this.nUpDown_Rocksmith_RenderWidth.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_Rocksmith_RenderWidth.TabIndex = 2;
            this.nUpDown_Rocksmith_RenderWidth.Visible = false;
            this.nUpDown_Rocksmith_RenderWidth.ValueChanged += new System.EventHandler(this.Rocksmith_RenderWidth);
            // 
            // nUpDown_Rocksmith_ScreenHeight
            // 
            this.nUpDown_Rocksmith_ScreenHeight.Location = new System.Drawing.Point(103, 57);
            this.nUpDown_Rocksmith_ScreenHeight.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_ScreenHeight.Name = "nUpDown_Rocksmith_ScreenHeight";
            this.nUpDown_Rocksmith_ScreenHeight.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_Rocksmith_ScreenHeight.TabIndex = 1;
            this.nUpDown_Rocksmith_ScreenHeight.ValueChanged += new System.EventHandler(this.Rocksmith_ScreenHeight);
            // 
            // nUpDown_Rocksmith_ScreenWidth
            // 
            this.nUpDown_Rocksmith_ScreenWidth.Location = new System.Drawing.Point(103, 31);
            this.nUpDown_Rocksmith_ScreenWidth.Maximum = new decimal(new int[] {
            10000,
            0,
            0,
            0});
            this.nUpDown_Rocksmith_ScreenWidth.Name = "nUpDown_Rocksmith_ScreenWidth";
            this.nUpDown_Rocksmith_ScreenWidth.Size = new System.Drawing.Size(120, 20);
            this.nUpDown_Rocksmith_ScreenWidth.TabIndex = 0;
            this.nUpDown_Rocksmith_ScreenWidth.ValueChanged += new System.EventHandler(this.Rocksmith_ScreenWidth);
            // 
            // tab_Twitch
            // 
            this.tab_Twitch.BackColor = System.Drawing.Color.Azure;
            this.tab_Twitch.Controls.Add(this.checkBox_RevealTwitchAuthToken);
            this.tab_Twitch.Controls.Add(this.button_SaveLogToFile);
            this.tab_Twitch.Controls.Add(this.checkBox_TwitchForceReauth);
            this.tab_Twitch.Controls.Add(this.button_TestTwitchReward);
            this.tab_Twitch.Controls.Add(this.button_SolidNoteColorRandom);
            this.tab_Twitch.Controls.Add(this.textBox_SolidNoteColorPicker);
            this.tab_Twitch.Controls.Add(this.button_SolidNoteColorPicker);
            this.tab_Twitch.Controls.Add(this.button_RemoveReward);
            this.tab_Twitch.Controls.Add(this.button_AddSelectedReward);
            this.tab_Twitch.Controls.Add(this.dgv_EnabledRewards);
            this.tab_Twitch.Controls.Add(this.label_TwitchLog);
            this.tab_Twitch.Controls.Add(this.textBox_TwitchLog);
            this.tab_Twitch.Controls.Add(this.label_IsListeningToEvents);
            this.tab_Twitch.Controls.Add(this.dgv_DefaultRewards);
            this.tab_Twitch.Controls.Add(this.label_HorizontalLineTwitch);
            this.tab_Twitch.Controls.Add(this.label_TwitchAccessTokenVal);
            this.tab_Twitch.Controls.Add(this.label_TwitchChannelIDVal);
            this.tab_Twitch.Controls.Add(this.label_TwitchUsernameVal);
            this.tab_Twitch.Controls.Add(this.label_TwitchAccessToken);
            this.tab_Twitch.Controls.Add(this.label_TwitchChannelID);
            this.tab_Twitch.Controls.Add(this.label_TwitchUsername);
            this.tab_Twitch.Controls.Add(this.label_AuthorizedAs);
            this.tab_Twitch.Controls.Add(this.label_TwitchAuthorized);
            this.tab_Twitch.Controls.Add(this.button_TwitchReAuthorize);
            this.tab_Twitch.Location = new System.Drawing.Point(4, 22);
            this.tab_Twitch.Name = "tab_Twitch";
            this.tab_Twitch.Padding = new System.Windows.Forms.Padding(3);
            this.tab_Twitch.Size = new System.Drawing.Size(1150, 513);
            this.tab_Twitch.TabIndex = 6;
            this.tab_Twitch.Text = "Twitch Bot (Beta)";
            // 
            // checkBox_RevealTwitchAuthToken
            // 
            this.checkBox_RevealTwitchAuthToken.AutoSize = true;
            this.checkBox_RevealTwitchAuthToken.Location = new System.Drawing.Point(548, 87);
            this.checkBox_RevealTwitchAuthToken.Name = "checkBox_RevealTwitchAuthToken";
            this.checkBox_RevealTwitchAuthToken.Size = new System.Drawing.Size(60, 17);
            this.checkBox_RevealTwitchAuthToken.TabIndex = 126;
            this.checkBox_RevealTwitchAuthToken.Text = "Reveal";
            this.checkBox_RevealTwitchAuthToken.UseVisualStyleBackColor = true;
            this.checkBox_RevealTwitchAuthToken.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // button_SaveLogToFile
            // 
            this.button_SaveLogToFile.Location = new System.Drawing.Point(950, 361);
            this.button_SaveLogToFile.Name = "button_SaveLogToFile";
            this.button_SaveLogToFile.Size = new System.Drawing.Size(95, 23);
            this.button_SaveLogToFile.TabIndex = 125;
            this.button_SaveLogToFile.Text = "Save log to file";
            this.button_SaveLogToFile.UseVisualStyleBackColor = true;
            this.button_SaveLogToFile.Visible = false;
            this.button_SaveLogToFile.Click += new System.EventHandler(this.Twitch_SaveLogToFile);
            // 
            // checkBox_TwitchForceReauth
            // 
            this.checkBox_TwitchForceReauth.AutoSize = true;
            this.checkBox_TwitchForceReauth.Location = new System.Drawing.Point(820, 12);
            this.checkBox_TwitchForceReauth.Name = "checkBox_TwitchForceReauth";
            this.checkBox_TwitchForceReauth.Size = new System.Drawing.Size(86, 17);
            this.checkBox_TwitchForceReauth.TabIndex = 124;
            this.checkBox_TwitchForceReauth.Text = "Force reauth";
            this.checkBox_TwitchForceReauth.UseVisualStyleBackColor = true;
            this.checkBox_TwitchForceReauth.CheckedChanged += new System.EventHandler(this.Twitch_ForceReauth_CheckedChanged);
            // 
            // button_TestTwitchReward
            // 
            this.button_TestTwitchReward.Location = new System.Drawing.Point(781, 288);
            this.button_TestTwitchReward.Name = "button_TestTwitchReward";
            this.button_TestTwitchReward.Size = new System.Drawing.Size(83, 47);
            this.button_TestTwitchReward.TabIndex = 123;
            this.button_TestTwitchReward.Text = "Test reward";
            this.button_TestTwitchReward.UseVisualStyleBackColor = true;
            this.button_TestTwitchReward.Visible = false;
            this.button_TestTwitchReward.Click += new System.EventHandler(this.Twitch_TestReward);
            // 
            // button_SolidNoteColorRandom
            // 
            this.button_SolidNoteColorRandom.Location = new System.Drawing.Point(926, 87);
            this.button_SolidNoteColorRandom.Name = "button_SolidNoteColorRandom";
            this.button_SolidNoteColorRandom.Size = new System.Drawing.Size(84, 23);
            this.button_SolidNoteColorRandom.TabIndex = 122;
            this.button_SolidNoteColorRandom.Text = "Random Color";
            this.button_SolidNoteColorRandom.UseVisualStyleBackColor = true;
            this.button_SolidNoteColorRandom.Visible = false;
            this.button_SolidNoteColorRandom.Click += new System.EventHandler(this.Twitch_SolidNoteColorRandom_Click);
            // 
            // textBox_SolidNoteColorPicker
            // 
            this.textBox_SolidNoteColorPicker.Enabled = false;
            this.textBox_SolidNoteColorPicker.Location = new System.Drawing.Point(820, 89);
            this.textBox_SolidNoteColorPicker.Name = "textBox_SolidNoteColorPicker";
            this.textBox_SolidNoteColorPicker.Size = new System.Drawing.Size(100, 20);
            this.textBox_SolidNoteColorPicker.TabIndex = 121;
            this.textBox_SolidNoteColorPicker.Visible = false;
            // 
            // button_SolidNoteColorPicker
            // 
            this.button_SolidNoteColorPicker.Location = new System.Drawing.Point(676, 87);
            this.button_SolidNoteColorPicker.Name = "button_SolidNoteColorPicker";
            this.button_SolidNoteColorPicker.Size = new System.Drawing.Size(137, 23);
            this.button_SolidNoteColorPicker.TabIndex = 120;
            this.button_SolidNoteColorPicker.Text = "Change Solid Note Color";
            this.button_SolidNoteColorPicker.UseVisualStyleBackColor = true;
            this.button_SolidNoteColorPicker.Visible = false;
            this.button_SolidNoteColorPicker.Click += new System.EventHandler(this.Twitch_SolidNoteColorPicker_Click);
            // 
            // button_RemoveReward
            // 
            this.button_RemoveReward.Location = new System.Drawing.Point(781, 236);
            this.button_RemoveReward.Name = "button_RemoveReward";
            this.button_RemoveReward.Size = new System.Drawing.Size(83, 46);
            this.button_RemoveReward.TabIndex = 119;
            this.button_RemoveReward.Text = "Remove reward";
            this.button_RemoveReward.UseVisualStyleBackColor = true;
            this.button_RemoveReward.Visible = false;
            this.button_RemoveReward.Click += new System.EventHandler(this.Twitch_RemoveReward_Click);
            // 
            // button_AddSelectedReward
            // 
            this.button_AddSelectedReward.Location = new System.Drawing.Point(781, 184);
            this.button_AddSelectedReward.Name = "button_AddSelectedReward";
            this.button_AddSelectedReward.Size = new System.Drawing.Size(83, 46);
            this.button_AddSelectedReward.TabIndex = 118;
            this.button_AddSelectedReward.Text = "Add reward";
            this.button_AddSelectedReward.UseVisualStyleBackColor = true;
            this.button_AddSelectedReward.Visible = false;
            this.button_AddSelectedReward.Click += new System.EventHandler(this.Twitch_AddSelectedReward_Click);
            // 
            // dgv_EnabledRewards
            // 
            this.dgv_EnabledRewards.AllowUserToAddRows = false;
            this.dgv_EnabledRewards.AllowUserToDeleteRows = false;
            this.dgv_EnabledRewards.AllowUserToResizeRows = false;
            this.dgv_EnabledRewards.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgv_EnabledRewards.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.colEnabledRewardsEnabled,
            this.colEnabledRewardsName,
            this.colEnabledRewardsLength,
            this.colEnabledRewardsAmount,
            this.colEnabledRewardsType,
            this.colEnabledRewardsID});
            this.dgv_EnabledRewards.EditMode = System.Windows.Forms.DataGridViewEditMode.EditOnKeystroke;
            this.dgv_EnabledRewards.Location = new System.Drawing.Point(384, 134);
            this.dgv_EnabledRewards.MultiSelect = false;
            this.dgv_EnabledRewards.Name = "dgv_EnabledRewards";
            this.dgv_EnabledRewards.RowHeadersVisible = false;
            this.dgv_EnabledRewards.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.dgv_EnabledRewards.Size = new System.Drawing.Size(390, 246);
            this.dgv_EnabledRewards.TabIndex = 116;
            this.dgv_EnabledRewards.Visible = false;
            this.dgv_EnabledRewards.CellEndEdit += new System.Windows.Forms.DataGridViewCellEventHandler(this.Twitch_EnabledRewards_CellEndEdit);
            this.dgv_EnabledRewards.CurrentCellDirtyStateChanged += new System.EventHandler(this.Twitch_EnabledRewards_CurrentCellDirtyStateChanged);
            this.dgv_EnabledRewards.SelectionChanged += new System.EventHandler(this.Twitch_EnabledRewards_SelectionChanged);
            // 
            // colEnabledRewardsEnabled
            // 
            this.colEnabledRewardsEnabled.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colEnabledRewardsEnabled.FillWeight = 15F;
            this.colEnabledRewardsEnabled.HeaderText = "Enabled";
            this.colEnabledRewardsEnabled.Name = "colEnabledRewardsEnabled";
            this.colEnabledRewardsEnabled.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.colEnabledRewardsEnabled.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.Automatic;
            // 
            // colEnabledRewardsName
            // 
            this.colEnabledRewardsName.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colEnabledRewardsName.FillWeight = 27F;
            this.colEnabledRewardsName.HeaderText = "Name";
            this.colEnabledRewardsName.Name = "colEnabledRewardsName";
            this.colEnabledRewardsName.ReadOnly = true;
            // 
            // colEnabledRewardsLength
            // 
            this.colEnabledRewardsLength.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colEnabledRewardsLength.FillWeight = 20F;
            this.colEnabledRewardsLength.HeaderText = "Length (s)";
            this.colEnabledRewardsLength.Name = "colEnabledRewardsLength";
            // 
            // colEnabledRewardsAmount
            // 
            this.colEnabledRewardsAmount.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colEnabledRewardsAmount.FillWeight = 13F;
            this.colEnabledRewardsAmount.HeaderText = "Amount";
            this.colEnabledRewardsAmount.Name = "colEnabledRewardsAmount";
            // 
            // colEnabledRewardsType
            // 
            this.colEnabledRewardsType.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colEnabledRewardsType.FillWeight = 25F;
            this.colEnabledRewardsType.HeaderText = "Reward Type";
            this.colEnabledRewardsType.Name = "colEnabledRewardsType";
            this.colEnabledRewardsType.ReadOnly = true;
            this.colEnabledRewardsType.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            // 
            // colEnabledRewardsID
            // 
            this.colEnabledRewardsID.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colEnabledRewardsID.FillWeight = 1F;
            this.colEnabledRewardsID.HeaderText = "ID";
            this.colEnabledRewardsID.Name = "colEnabledRewardsID";
            this.colEnabledRewardsID.ReadOnly = true;
            this.colEnabledRewardsID.Visible = false;
            // 
            // label_TwitchLog
            // 
            this.label_TwitchLog.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.label_TwitchLog.Location = new System.Drawing.Point(870, 116);
            this.label_TwitchLog.Name = "label_TwitchLog";
            this.label_TwitchLog.Size = new System.Drawing.Size(256, 15);
            this.label_TwitchLog.TabIndex = 115;
            this.label_TwitchLog.Text = "Log";
            this.label_TwitchLog.Visible = false;
            // 
            // textBox_TwitchLog
            // 
            this.textBox_TwitchLog.Location = new System.Drawing.Point(869, 134);
            this.textBox_TwitchLog.Multiline = true;
            this.textBox_TwitchLog.Name = "textBox_TwitchLog";
            this.textBox_TwitchLog.Size = new System.Drawing.Size(256, 221);
            this.textBox_TwitchLog.TabIndex = 114;
            this.textBox_TwitchLog.Visible = false;
            this.textBox_TwitchLog.TextChanged += new System.EventHandler(this.Twitch_scrollLog);
            // 
            // label_IsListeningToEvents
            // 
            this.label_IsListeningToEvents.AutoSize = true;
            this.label_IsListeningToEvents.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.label_IsListeningToEvents.Location = new System.Drawing.Point(735, 46);
            this.label_IsListeningToEvents.Name = "label_IsListeningToEvents";
            this.label_IsListeningToEvents.Size = new System.Drawing.Size(205, 18);
            this.label_IsListeningToEvents.TabIndex = 113;
            this.label_IsListeningToEvents.Text = "Not listening for Twitch events";
            // 
            // dgv_DefaultRewards
            // 
            this.dgv_DefaultRewards.AllowUserToAddRows = false;
            this.dgv_DefaultRewards.AllowUserToDeleteRows = false;
            this.dgv_DefaultRewards.AllowUserToResizeRows = false;
            this.dgv_DefaultRewards.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgv_DefaultRewards.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.colDefaultRewardsName,
            this.colDefaultRewardsDescription});
            this.dgv_DefaultRewards.EditMode = System.Windows.Forms.DataGridViewEditMode.EditProgrammatically;
            this.dgv_DefaultRewards.Location = new System.Drawing.Point(6, 134);
            this.dgv_DefaultRewards.MultiSelect = false;
            this.dgv_DefaultRewards.Name = "dgv_DefaultRewards";
            this.dgv_DefaultRewards.ReadOnly = true;
            this.dgv_DefaultRewards.RowHeadersVisible = false;
            this.dgv_DefaultRewards.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.dgv_DefaultRewards.Size = new System.Drawing.Size(372, 246);
            this.dgv_DefaultRewards.TabIndex = 112;
            this.dgv_DefaultRewards.Visible = false;
            // 
            // colDefaultRewardsName
            // 
            this.colDefaultRewardsName.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colDefaultRewardsName.FillWeight = 25F;
            this.colDefaultRewardsName.HeaderText = "Name";
            this.colDefaultRewardsName.Name = "colDefaultRewardsName";
            this.colDefaultRewardsName.ReadOnly = true;
            // 
            // colDefaultRewardsDescription
            // 
            this.colDefaultRewardsDescription.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.colDefaultRewardsDescription.FillWeight = 75F;
            this.colDefaultRewardsDescription.HeaderText = "Description";
            this.colDefaultRewardsDescription.Name = "colDefaultRewardsDescription";
            this.colDefaultRewardsDescription.ReadOnly = true;
            // 
            // label_HorizontalLineTwitch
            // 
            this.label_HorizontalLineTwitch.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.label_HorizontalLineTwitch.Location = new System.Drawing.Point(6, 116);
            this.label_HorizontalLineTwitch.Name = "label_HorizontalLineTwitch";
            this.label_HorizontalLineTwitch.Size = new System.Drawing.Size(858, 15);
            this.label_HorizontalLineTwitch.TabIndex = 111;
            this.label_HorizontalLineTwitch.Text = "Rewards                ";
            this.label_HorizontalLineTwitch.Visible = false;
            // 
            // label_TwitchAccessTokenVal
            // 
            this.label_TwitchAccessTokenVal.AutoSize = true;
            this.label_TwitchAccessTokenVal.Location = new System.Drawing.Point(362, 87);
            this.label_TwitchAccessTokenVal.Name = "label_TwitchAccessTokenVal";
            this.label_TwitchAccessTokenVal.Size = new System.Drawing.Size(0, 13);
            this.label_TwitchAccessTokenVal.TabIndex = 8;
            this.label_TwitchAccessTokenVal.TextChanged += new System.EventHandler(this.Twitch_NewAccessToken);
            this.label_TwitchAccessTokenVal.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Twitch_CopyOnClick);
            this.label_TwitchAccessTokenVal.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_TwitchChannelIDVal
            // 
            this.label_TwitchChannelIDVal.AutoSize = true;
            this.label_TwitchChannelIDVal.Location = new System.Drawing.Point(362, 62);
            this.label_TwitchChannelIDVal.Name = "label_TwitchChannelIDVal";
            this.label_TwitchChannelIDVal.Size = new System.Drawing.Size(0, 13);
            this.label_TwitchChannelIDVal.TabIndex = 7;
            this.label_TwitchChannelIDVal.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Twitch_CopyOnClick);
            this.label_TwitchChannelIDVal.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_TwitchUsernameVal
            // 
            this.label_TwitchUsernameVal.AutoSize = true;
            this.label_TwitchUsernameVal.Location = new System.Drawing.Point(362, 37);
            this.label_TwitchUsernameVal.Name = "label_TwitchUsernameVal";
            this.label_TwitchUsernameVal.Size = new System.Drawing.Size(0, 13);
            this.label_TwitchUsernameVal.TabIndex = 6;
            this.label_TwitchUsernameVal.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Twitch_CopyOnClick);
            this.label_TwitchUsernameVal.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // label_TwitchAccessToken
            // 
            this.label_TwitchAccessToken.AutoSize = true;
            this.label_TwitchAccessToken.Location = new System.Drawing.Point(281, 87);
            this.label_TwitchAccessToken.Name = "label_TwitchAccessToken";
            this.label_TwitchAccessToken.Size = new System.Drawing.Size(62, 13);
            this.label_TwitchAccessToken.TabIndex = 5;
            this.label_TwitchAccessToken.Text = "Auth token:";
            // 
            // label_TwitchChannelID
            // 
            this.label_TwitchChannelID.AutoSize = true;
            this.label_TwitchChannelID.Location = new System.Drawing.Point(281, 62);
            this.label_TwitchChannelID.Name = "label_TwitchChannelID";
            this.label_TwitchChannelID.Size = new System.Drawing.Size(63, 13);
            this.label_TwitchChannelID.TabIndex = 4;
            this.label_TwitchChannelID.Text = "Channel ID:";
            // 
            // label_TwitchUsername
            // 
            this.label_TwitchUsername.AutoSize = true;
            this.label_TwitchUsername.Location = new System.Drawing.Point(281, 37);
            this.label_TwitchUsername.Name = "label_TwitchUsername";
            this.label_TwitchUsername.Size = new System.Drawing.Size(58, 13);
            this.label_TwitchUsername.TabIndex = 3;
            this.label_TwitchUsername.Text = "Username:";
            // 
            // label_AuthorizedAs
            // 
            this.label_AuthorizedAs.AutoSize = true;
            this.label_AuthorizedAs.Location = new System.Drawing.Point(426, 25);
            this.label_AuthorizedAs.Name = "label_AuthorizedAs";
            this.label_AuthorizedAs.Size = new System.Drawing.Size(0, 13);
            this.label_AuthorizedAs.TabIndex = 2;
            // 
            // label_TwitchAuthorized
            // 
            this.label_TwitchAuthorized.AutoSize = true;
            this.label_TwitchAuthorized.Location = new System.Drawing.Point(270, 12);
            this.label_TwitchAuthorized.Name = "label_TwitchAuthorized";
            this.label_TwitchAuthorized.Size = new System.Drawing.Size(156, 13);
            this.label_TwitchAuthorized.TabIndex = 1;
            this.label_TwitchAuthorized.Text = "You are currently authorized as:";
            // 
            // button_TwitchReAuthorize
            // 
            this.button_TwitchReAuthorize.Location = new System.Drawing.Point(33, 12);
            this.button_TwitchReAuthorize.Name = "button_TwitchReAuthorize";
            this.button_TwitchReAuthorize.Size = new System.Drawing.Size(219, 88);
            this.button_TwitchReAuthorize.TabIndex = 0;
            this.button_TwitchReAuthorize.Text = "(Re)authorize\r\n\r\nFor streamers only!";
            this.button_TwitchReAuthorize.UseVisualStyleBackColor = true;
            this.button_TwitchReAuthorize.Click += new System.EventHandler(this.Twitch_ReAuthorize_Click);
            // 
            // tab_GUISettings
            // 
            this.tab_GUISettings.BackColor = System.Drawing.Color.Azure;
            this.tab_GUISettings.Controls.Add(this.groupBox_ChangeTheme);
            this.tab_GUISettings.Controls.Add(this.checkBox_ChangeTheme);
            this.tab_GUISettings.Controls.Add(this.label_Credits);
            this.tab_GUISettings.Location = new System.Drawing.Point(4, 22);
            this.tab_GUISettings.Margin = new System.Windows.Forms.Padding(0);
            this.tab_GUISettings.Name = "tab_GUISettings";
            this.tab_GUISettings.Size = new System.Drawing.Size(1150, 513);
            this.tab_GUISettings.TabIndex = 5;
            this.tab_GUISettings.Text = "GUI Settings";
            // 
            // groupBox_ChangeTheme
            // 
            this.groupBox_ChangeTheme.Controls.Add(this.button_ApplyThemeColors);
            this.groupBox_ChangeTheme.Controls.Add(this.textBox_ChangeTextColor);
            this.groupBox_ChangeTheme.Controls.Add(this.textBox_ChangeBackgroundColor);
            this.groupBox_ChangeTheme.Controls.Add(this.button_ChangeTextColor);
            this.groupBox_ChangeTheme.Controls.Add(this.button_ChangeBackgroundColor);
            this.groupBox_ChangeTheme.Location = new System.Drawing.Point(284, 18);
            this.groupBox_ChangeTheme.Name = "groupBox_ChangeTheme";
            this.groupBox_ChangeTheme.Size = new System.Drawing.Size(271, 155);
            this.groupBox_ChangeTheme.TabIndex = 74;
            this.groupBox_ChangeTheme.TabStop = false;
            this.groupBox_ChangeTheme.Text = "Change Theme Colors";
            this.groupBox_ChangeTheme.Visible = false;
            // 
            // button_ApplyThemeColors
            // 
            this.button_ApplyThemeColors.Location = new System.Drawing.Point(60, 126);
            this.button_ApplyThemeColors.Name = "button_ApplyThemeColors";
            this.button_ApplyThemeColors.Size = new System.Drawing.Size(133, 23);
            this.button_ApplyThemeColors.TabIndex = 4;
            this.button_ApplyThemeColors.Text = "Apply Theme Colors";
            this.button_ApplyThemeColors.UseVisualStyleBackColor = true;
            this.button_ApplyThemeColors.Click += new System.EventHandler(this.Apply_ThemeColors);
            // 
            // textBox_ChangeTextColor
            // 
            this.textBox_ChangeTextColor.Enabled = false;
            this.textBox_ChangeTextColor.Location = new System.Drawing.Point(172, 79);
            this.textBox_ChangeTextColor.Name = "textBox_ChangeTextColor";
            this.textBox_ChangeTextColor.ReadOnly = true;
            this.textBox_ChangeTextColor.Size = new System.Drawing.Size(74, 20);
            this.textBox_ChangeTextColor.TabIndex = 3;
            // 
            // textBox_ChangeBackgroundColor
            // 
            this.textBox_ChangeBackgroundColor.Enabled = false;
            this.textBox_ChangeBackgroundColor.Location = new System.Drawing.Point(172, 37);
            this.textBox_ChangeBackgroundColor.Name = "textBox_ChangeBackgroundColor";
            this.textBox_ChangeBackgroundColor.ReadOnly = true;
            this.textBox_ChangeBackgroundColor.Size = new System.Drawing.Size(74, 20);
            this.textBox_ChangeBackgroundColor.TabIndex = 2;
            // 
            // button_ChangeTextColor
            // 
            this.button_ChangeTextColor.Location = new System.Drawing.Point(6, 79);
            this.button_ChangeTextColor.Name = "button_ChangeTextColor";
            this.button_ChangeTextColor.Size = new System.Drawing.Size(145, 23);
            this.button_ChangeTextColor.TabIndex = 1;
            this.button_ChangeTextColor.Text = "Change Text Color";
            this.button_ChangeTextColor.UseVisualStyleBackColor = true;
            this.button_ChangeTextColor.Click += new System.EventHandler(this.CustomTheme_ChangeTextColor);
            // 
            // button_ChangeBackgroundColor
            // 
            this.button_ChangeBackgroundColor.Location = new System.Drawing.Point(6, 33);
            this.button_ChangeBackgroundColor.Name = "button_ChangeBackgroundColor";
            this.button_ChangeBackgroundColor.Size = new System.Drawing.Size(145, 23);
            this.button_ChangeBackgroundColor.TabIndex = 0;
            this.button_ChangeBackgroundColor.Text = "Change Background Color";
            this.button_ChangeBackgroundColor.UseVisualStyleBackColor = true;
            this.button_ChangeBackgroundColor.Click += new System.EventHandler(this.CustomTheme_ChangeBackgroundColor);
            // 
            // checkBox_ChangeTheme
            // 
            this.checkBox_ChangeTheme.AutoSize = true;
            this.checkBox_ChangeTheme.Location = new System.Drawing.Point(16, 15);
            this.checkBox_ChangeTheme.Name = "checkBox_ChangeTheme";
            this.checkBox_ChangeTheme.Size = new System.Drawing.Size(99, 17);
            this.checkBox_ChangeTheme.TabIndex = 73;
            this.checkBox_ChangeTheme.Text = "Change Theme";
            this.checkBox_ChangeTheme.UseVisualStyleBackColor = true;
            this.checkBox_ChangeTheme.CheckedChanged += new System.EventHandler(this.CustomTheme_ChangeTheme);
            this.checkBox_ChangeTheme.MouseHover += new System.EventHandler(this.RunToolTips);
            // 
            // timerValidateTwitch
            // 
            this.timerValidateTwitch.Interval = 150000;
            this.timerValidateTwitch.Tick += new System.EventHandler(this.Twitch_timerValidate_Tick);
            // 
            // label_SettingsSaved
            // 
            this.label_SettingsSaved.AutoSize = true;
            this.label_SettingsSaved.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_SettingsSaved.Location = new System.Drawing.Point(4, 549);
            this.label_SettingsSaved.Name = "label_SettingsSaved";
            this.label_SettingsSaved.Size = new System.Drawing.Size(120, 18);
            this.label_SettingsSaved.TabIndex = 52;
            this.label_SettingsSaved.Text = "Settings Saved";
            this.label_SettingsSaved.Visible = false;
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Azure;
            this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.ClientSize = new System.Drawing.Size(1170, 576);
            this.Controls.Add(this.label_SettingsSaved);
            this.Controls.Add(this.TabController);
            this.Cursor = System.Windows.Forms.Cursors.Default;
            this.ForeColor = System.Drawing.Color.Black;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "MainForm";
            this.Text = "Rocksmith 2014 DLL Modifier";
            this.groupBox_HowToEnumerate.ResumeLayout(false);
            this.groupBox_HowToEnumerate.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ForceEnumerationXMS)).EndInit();
            this.groupBox_LoftOffWhen.ResumeLayout(false);
            this.groupBox_LoftOffWhen.PerformLayout();
            this.groupBox_ExtendedRangeWhen.ResumeLayout(false);
            this.groupBox_ExtendedRangeWhen.PerformLayout();
            this.groupBox_EnabledMods.ResumeLayout(false);
            this.groupBox_EnabledMods.PerformLayout();
            this.groupBox_Songlist.ResumeLayout(false);
            this.groupBox_Songlist.PerformLayout();
            this.groupBox_Keybindings_MODS.ResumeLayout(false);
            this.groupBox_Keybindings_MODS.PerformLayout();
            this.groupBox_StringColors.ResumeLayout(false);
            this.groupBox_StringColors.PerformLayout();
            this.groupBox_SetAndForget.ResumeLayout(false);
            this.groupBox_SetAndForget.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String0)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_UIIndex)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String5)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String4)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String3)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_String2)).EndInit();
            this.groupBox_ToggleSkylineWhen.ResumeLayout(false);
            this.groupBox_ToggleSkylineWhen.PerformLayout();
            this.groupBox_ToggleLyricsOffWhen.ResumeLayout(false);
            this.groupBox_ToggleLyricsOffWhen.PerformLayout();
            this.groupBox_GuitarSpeak.ResumeLayout(false);
            this.groupBox_GuitarSpeak.PerformLayout();
            this.groupBox_ToggleHeadstockOffWhen.ResumeLayout(false);
            this.groupBox_ToggleHeadstockOffWhen.PerformLayout();
            this.TabController.ResumeLayout(false);
            this.tab_Songlists.ResumeLayout(false);
            this.tab_Keybindings.ResumeLayout(false);
            this.groupBox_Keybindings_AUDIO.ResumeLayout(false);
            this.groupBox_Keybindings_AUDIO.PerformLayout();
            this.tab_ModToggles.ResumeLayout(false);
            this.tab_SetAndForget.ResumeLayout(false);
            this.tab_ModSettings.ResumeLayout(false);
            this.TabController_ModSettings.ResumeLayout(false);
            this.tabPage_ModSettings_ER.ResumeLayout(false);
            this.tabPage_ModSettings_Automation.ResumeLayout(false);
            this.groupBox_Backups.ResumeLayout(false);
            this.groupBox_Backups.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_NumberOfBackups)).EndInit();
            this.groupBox_AutoLoadProfiles.ResumeLayout(false);
            this.groupBox_MidiAutoTuneDevice.ResumeLayout(false);
            this.groupBox_MidiAutoTuneDevice.PerformLayout();
            this.tabPage_ModSettings_Misc.ResumeLayout(false);
            this.groupBox_CustomHighway.ResumeLayout(false);
            this.groupBox_CustomHighway.PerformLayout();
            this.groupBox_OnScreenFont.ResumeLayout(false);
            this.groupBox_OnScreenFont.PerformLayout();
            this.groupBox_RRSpeed.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_RiffRepeaterSpeed)).EndInit();
            this.groupBox_ControlVolumeIncrement.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_VolumeInterval)).EndInit();
            this.tab_RSASIO.ResumeLayout(false);
            this.tab_RSASIO.PerformLayout();
            this.groupBox_ASIO_InputMic.ResumeLayout(false);
            this.groupBox_ASIO_InputMic.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_InputMic_MaxVolume)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_InputMic_Channel)).EndInit();
            this.groupBox_ASIO_Output.ResumeLayout(false);
            this.groupBox_ASIO_Output.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Output_MaxVolume)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Output_BaseChannel)).EndInit();
            this.groupBox_ASIO_Input1.ResumeLayout(false);
            this.groupBox_ASIO_Input1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Input1_MaxVolume)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Input1_Channel)).EndInit();
            this.groupBox_ASIO_Input0.ResumeLayout(false);
            this.groupBox_ASIO_Input0.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Input0_MaxVolume)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_Input0_Channel)).EndInit();
            this.groupBox_ASIO_Config.ResumeLayout(false);
            this.groupBox_ASIO_Config.PerformLayout();
            this.groupBox_ASIO_BufferSize.ResumeLayout(false);
            this.groupBox_ASIO_BufferSize.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_ASIO_CustomBufferSize)).EndInit();
            this.tab_Rocksmith.ResumeLayout(false);
            this.tab_Rocksmith.PerformLayout();
            this.groupBox_Rocksmith_AudioSettings.ResumeLayout(false);
            this.groupBox_Rocksmith_AudioSettings.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_MaxOutputBuffer)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_LatencyBuffer)).EndInit();
            this.groupBox_Rocksmith_VisualSettings.ResumeLayout(false);
            this.groupBox_Rocksmith_VisualSettings.PerformLayout();
            this.groupBox_Rocksmith_Network.ResumeLayout(false);
            this.groupBox_Rocksmith_Network.PerformLayout();
            this.groupBox_Rocksmith_Fullscreen.ResumeLayout(false);
            this.groupBox_Rocksmith_Fullscreen.PerformLayout();
            this.groupBox_Rocksmith_VisualQuality.ResumeLayout(false);
            this.groupBox_Rocksmith_VisualQuality.PerformLayout();
            this.groupBox_Rocksmith_Resolutions.ResumeLayout(false);
            this.groupBox_Rocksmith_Resolutions.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_RenderHeight)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_RenderWidth)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_ScreenHeight)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nUpDown_Rocksmith_ScreenWidth)).EndInit();
            this.tab_Twitch.ResumeLayout(false);
            this.tab_Twitch.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgv_EnabledRewards)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.dgv_DefaultRewards)).EndInit();
            this.tab_GUISettings.ResumeLayout(false);
            this.tab_GUISettings.PerformLayout();
            this.groupBox_ChangeTheme.ResumeLayout(false);
            this.groupBox_ChangeTheme.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        

        private System.Windows.Forms.ListBox listBox_Songlist;
        private System.Windows.Forms.ListBox listBox_Modlist_MODS;
        private System.Windows.Forms.CheckBox checkBox_ToggleLoft;
        private System.Windows.Forms.CheckBox checkBox_SongTimer;
        private System.Windows.Forms.Label label_ToggleLoftKey;
        private System.Windows.Forms.Label label_SongTimerKey;
        private System.Windows.Forms.Label label_ReEnumerationKey;
        private System.Windows.Forms.TextBox textBox_NewKeyAssignment_MODS;
        private System.Windows.Forms.Label label_NewKeyAssignment_MODS;
        private System.Windows.Forms.Label label_ChangeSonglistName;
        private System.Windows.Forms.TextBox textBox_NewSonglistName;
        private System.Windows.Forms.CheckBox checkBox_ExtendedRange;
        private System.Windows.Forms.Label label_RainbowStringsKey;
        private System.Windows.Forms.ListBox listBox_ExtendedRangeTunings;
        private System.Windows.Forms.CheckBox checkBox_ForceEnumeration;
        private System.Windows.Forms.CheckBox checkBox_RemoveHeadstock;
        private System.Windows.Forms.CheckBox checkBox_RemoveSkyline;
        private System.Windows.Forms.CheckBox checkBox_GreenScreen;
        private System.Windows.Forms.CheckBox checkBox_AutoLoadProfile;
        private System.Windows.Forms.CheckBox checkBox_Fretless;
        private System.Windows.Forms.CheckBox checkBox_RemoveInlays;
        private System.Windows.Forms.RadioButton radio_LoftAlwaysOff;
        private System.Windows.Forms.RadioButton radio_LoftOffHotkey;
        private System.Windows.Forms.RadioButton radio_LoftOffInSong;
        private System.Windows.Forms.RadioButton radio_ForceEnumerationManual;
        private System.Windows.Forms.RadioButton radio_ForceEnumerationAutomatic;
        private System.Windows.Forms.GroupBox groupBox_HowToEnumerate;
        private System.Windows.Forms.GroupBox groupBox_LoftOffWhen;
        private System.Windows.Forms.GroupBox groupBox_ExtendedRangeWhen;
        private System.Windows.Forms.GroupBox groupBox_EnabledMods;
        private System.Windows.Forms.CheckBox checkBox_RemoveLaneMarkers;
        private System.Windows.Forms.GroupBox groupBox_Songlist;
        private System.Windows.Forms.GroupBox groupBox_Keybindings_MODS;
        private System.Windows.Forms.Button button_ResetModsToDefault;
        private System.Windows.Forms.Button button_String0ColorButton;
        private System.Windows.Forms.TextBox textBox_String0Color;
        private System.Windows.Forms.TextBox textBox_String1Color;
        private System.Windows.Forms.Button button_String1ColorButton;
        private System.Windows.Forms.TextBox textBox_String2Color;
        private System.Windows.Forms.Button button_String2ColorButton;
        private System.Windows.Forms.TextBox textBox_String3Color;
        private System.Windows.Forms.Button button_String3ColorButton;
        private System.Windows.Forms.TextBox textBox_String4Color;
        private System.Windows.Forms.Button button_String4ColorButton;
        private System.Windows.Forms.TextBox textBox_String5Color;
        private System.Windows.Forms.Button button_String5ColorButton;
        private System.Windows.Forms.GroupBox groupBox_StringColors;
        private System.Windows.Forms.RadioButton radio_colorBlindERColors;
        private System.Windows.Forms.RadioButton radio_DefaultStringColors;
        private System.Windows.Forms.NumericUpDown nUpDown_ForceEnumerationXMS;
        private System.Windows.Forms.Label label_ForceEnumerationXMS;
        private System.Windows.Forms.CheckBox checkBox_CustomColors;
        private System.Windows.Forms.GroupBox groupBox_SetAndForget;
        private System.Windows.Forms.Button button_AddFastLoad;
        private System.Windows.Forms.Button button_RestoreCacheBackup;
        private System.Windows.Forms.Button button_AddCustomTunings;
        private System.Windows.Forms.Button button_SaveTuningChanges;
        private System.Windows.Forms.Label label_CustomStringTunings;
        private System.Windows.Forms.NumericUpDown nUpDown_String0;
        private System.Windows.Forms.Label label_UIIndex;
        private System.Windows.Forms.NumericUpDown nUpDown_UIIndex;
        private System.Windows.Forms.Label label_UITuningName;
        private System.Windows.Forms.Label label_InternalTuningName;
        private System.Windows.Forms.NumericUpDown nUpDown_String1;
        private System.Windows.Forms.NumericUpDown nUpDown_String5;
        private System.Windows.Forms.NumericUpDown nUpDown_String4;
        private System.Windows.Forms.NumericUpDown nUpDown_String3;
        private System.Windows.Forms.NumericUpDown nUpDown_String2;
        private System.Windows.Forms.TextBox textBox_UIName;
        private System.Windows.Forms.TextBox textBox_InternalTuningName;
        private System.Windows.Forms.Button button_AddTuning;
        private System.Windows.Forms.Button button_RemoveTuning;
        private System.Windows.Forms.ListBox listBox_Tunings;
        private System.Windows.Forms.Button button_AddExitGame;
        private System.Windows.Forms.Button button_ClearSelectedKeybind_MODS;
        private System.Windows.Forms.CheckBox checkBox_RemoveLyrics;
        private System.Windows.Forms.ToolTip ToolTip;
        private System.Windows.Forms.Button button_RemoveTemp;
        private System.Windows.Forms.Button button_AssignNewDefaultTone;
        private System.Windows.Forms.Button button_LoadTones;
        private System.Windows.Forms.ListBox listBox_ProfileTones;
        private System.Windows.Forms.Label label_ChangeTonesHeader;
        private System.Windows.Forms.RadioButton radio_DefaultBassTone;
        private System.Windows.Forms.RadioButton radio_DefaultLeadTone;
        private System.Windows.Forms.RadioButton radio_DefaultRhythmTone;
        private System.Windows.Forms.GroupBox groupBox_ToggleSkylineWhen;
        private System.Windows.Forms.RadioButton radio_SkylineOffInSong;
        private System.Windows.Forms.RadioButton radio_SkylineAlwaysOff;
        private System.Windows.Forms.GroupBox groupBox_ToggleLyricsOffWhen;
        private System.Windows.Forms.RadioButton radio_LyricsOffHotkey;
        private System.Windows.Forms.RadioButton radio_LyricsAlwaysOff;
        private System.Windows.Forms.Label label_RemoveLyricsKey;
        private System.Windows.Forms.GroupBox groupBox_GuitarSpeak;
        private System.Windows.Forms.ListBox listBox_GuitarSpeakKeypress;
        private System.Windows.Forms.ListBox listBox_GuitarSpeakOctave;
        private System.Windows.Forms.ListBox listBox_GuitarSpeakNote;
        private System.Windows.Forms.Label label_GuitarSpeakKeypress;
        private System.Windows.Forms.Label label_GuitarSpeakOctave;
        private System.Windows.Forms.Label label_GuitarSpeakNote;
        private System.Windows.Forms.Button button_GuitarSpeakSave;
        private System.Windows.Forms.CheckBox checkBox_GuitarSpeak;
        private System.Windows.Forms.Label label_CustomTuningLowEStringLetter;
        private System.Windows.Forms.Label label_CustomTuningHighEStringLetter;
        private System.Windows.Forms.Label label_CustomTuningBStringLetter;
        private System.Windows.Forms.Label label_CustomTuningGStringLetter;
        private System.Windows.Forms.Label label_CustomTuningDStringLetter;
        private System.Windows.Forms.Label label_CustomTuningAStringLetter;
        private System.Windows.Forms.Label label_SonglistWarning;
        private System.Windows.Forms.Label label_Credits;
        private System.Windows.Forms.CheckBox checkBox_RainbowStrings;
        private System.Windows.Forms.CheckBox checkBox_ControlVolume;
        private System.Windows.Forms.CheckBox checkbox_GuitarSpeakWhileTuning;
        private System.Windows.Forms.Label label_GuitarSpeakSaved;
        private System.Windows.Forms.ListBox listBox_GuitarSpeakSaved;
        private System.Windows.Forms.Button button_GuitarSpeakHelp;
        private System.Windows.Forms.GroupBox groupBox_ToggleHeadstockOffWhen;
        private System.Windows.Forms.RadioButton radio_HeadstockOffInSong;
        private System.Windows.Forms.RadioButton radio_HeadstockAlwaysOff;
        private System.Windows.Forms.Button button_ImportExistingSettings;
        private System.Windows.Forms.TabPage tab_Songlists;
        private System.Windows.Forms.TabPage tab_Keybindings;
        private System.Windows.Forms.TabPage tab_ModToggles;
        private System.Windows.Forms.TabPage tab_SetAndForget;
        private System.Windows.Forms.TabPage tab_ModSettings;
        private System.Windows.Forms.TabPage tab_GUISettings;
        public System.Windows.Forms.TabControl TabController;
        private System.Windows.Forms.CheckBox checkBox_ChangeTheme;
        private System.Windows.Forms.Label label_HorizontalRuleSetAndForget;
        private System.Windows.Forms.Button button_CleanUpUnpackedCache;
        private System.Windows.Forms.TabPage tab_Twitch;
        private System.Windows.Forms.Button button_TwitchReAuthorize;
        private System.Windows.Forms.Label label_TwitchAuthorized;
        private System.Windows.Forms.GroupBox groupBox_ChangeTheme;
        private System.Windows.Forms.TextBox textBox_ChangeTextColor;
        private System.Windows.Forms.TextBox textBox_ChangeBackgroundColor;
        private System.Windows.Forms.Button button_ChangeTextColor;
        private System.Windows.Forms.Button button_ChangeBackgroundColor;
        private System.Windows.Forms.Button button_ApplyThemeColors;
        private System.Windows.Forms.Label label_TwitchUsername;
        private System.Windows.Forms.Label label_TwitchAccessToken;
        private System.Windows.Forms.Label label_TwitchChannelID;
        private System.Windows.Forms.Label label_TwitchAccessTokenVal;
        private System.Windows.Forms.Label label_TwitchChannelIDVal;
        private System.Windows.Forms.Label label_TwitchUsernameVal;
        private System.Windows.Forms.Label label_AuthorizedAs;
        private System.Windows.Forms.Label label_HorizontalLineTwitch;
        private System.Windows.Forms.DataGridView dgv_DefaultRewards;
        private System.Windows.Forms.Label label_IsListeningToEvents;
        private System.Windows.Forms.Label label_TwitchLog;
        private System.Windows.Forms.TextBox textBox_TwitchLog;
        private System.Windows.Forms.DataGridView dgv_EnabledRewards;
        private System.Windows.Forms.Button button_AddSelectedReward;
        private System.Windows.Forms.DataGridViewTextBoxColumn colDefaultRewardsName;
        private System.Windows.Forms.DataGridViewTextBoxColumn colDefaultRewardsDescription;
        private System.Windows.Forms.Button button_RemoveReward;
        private System.Windows.Forms.TextBox textBox_SolidNoteColorPicker;
        private System.Windows.Forms.Button button_SolidNoteColorPicker;
        private System.Windows.Forms.Button button_SolidNoteColorRandom;
        private System.Windows.Forms.Button button_TestTwitchReward;
        private System.Windows.Forms.Timer timerValidateTwitch;
        private System.Windows.Forms.CheckBox checkBox_TwitchForceReauth;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colEnabledRewardsEnabled;
        private System.Windows.Forms.DataGridViewTextBoxColumn colEnabledRewardsName;
        private System.Windows.Forms.DataGridViewTextBoxColumn colEnabledRewardsLength;
        private System.Windows.Forms.DataGridViewTextBoxColumn colEnabledRewardsAmount;
        private System.Windows.Forms.DataGridViewTextBoxColumn colEnabledRewardsType;
        private System.Windows.Forms.DataGridViewTextBoxColumn colEnabledRewardsID;
        private System.Windows.Forms.CheckBox checkBox_ScreenShotScores;
        private System.Windows.Forms.Button button_SaveLogToFile;
        private System.Windows.Forms.CheckBox checkBox_RiffRepeaterSpeedAboveOneHundred;
        private System.Windows.Forms.GroupBox groupBox_RRSpeed;
        private System.Windows.Forms.NumericUpDown nUpDown_RiffRepeaterSpeed;
        private System.Windows.Forms.Label label_RRSpeedKey;
        private System.Windows.Forms.Button button_AddDCInput;
        private System.Windows.Forms.CheckBox checkBox_useMidiAutoTuning;
        private System.Windows.Forms.GroupBox groupBox_MidiAutoTuneDevice;
        private System.Windows.Forms.ListBox listBox_ListMidiDevices;
        private System.Windows.Forms.Label label_SelectedMidiDevice;
        private System.Windows.Forms.RadioButton radio_WhammyORBass;
        private System.Windows.Forms.RadioButton radio_WhammyDT;
        private System.Windows.Forms.Label label_MidiWhatTuningPedalDoYouUse;
        private System.Windows.Forms.CheckBox checkBox_WhammyChordsMode;
        private System.Windows.Forms.Label label_SettingsSaved;
        private System.Windows.Forms.CheckBox checkBox_ExtendedRangeDrop;
        private System.Windows.Forms.CheckBox checkBox_ShowCurrentNote;
        private System.Windows.Forms.CheckBox checkBox_RevealTwitchAuthToken;
        private System.Windows.Forms.GroupBox groupBox_OnScreenFont;
        private System.Windows.Forms.ListBox listBox_AvailableFonts;
        private System.Windows.Forms.Label label_FontTestNumbers;
        private System.Windows.Forms.Label label_FontTestlowercase;
        private System.Windows.Forms.Label label_FontTestCAPITALS;
        private System.Windows.Forms.GroupBox groupBox_Keybindings_AUDIO;
        private System.Windows.Forms.Button button_ClearSelectedKeybind_AUDIO;
        private System.Windows.Forms.Label label_NewAssignmentAUDIO;
        private System.Windows.Forms.TextBox textBox_NewKeyAssignment_AUDIO;
        private System.Windows.Forms.ListBox listBox_Modlist_AUDIO;
        private System.Windows.Forms.Label label_ChangeSelectedVolumeKey;
        private System.Windows.Forms.Label label_SFXVolumeKey;
        private System.Windows.Forms.Label label_VoiceOverVolumeKey;
        private System.Windows.Forms.Label label_MicrophoneVolumeKey;
        private System.Windows.Forms.Label label_Player2VolumeKey;
        private System.Windows.Forms.Label label_Player1VolumeKey;
        private System.Windows.Forms.Label label_SongVolumeKey;
        private System.Windows.Forms.Label label_MasterVolumeKey;
        private System.Windows.Forms.GroupBox groupBox_ControlVolumeIncrement;
        private System.Windows.Forms.NumericUpDown nUpDown_VolumeInterval;
        private System.Windows.Forms.TabPage tab_RSASIO;
        private System.Windows.Forms.ListBox listBox_AvailableASIODevices_Input0;
        private System.Windows.Forms.GroupBox groupBox_ASIO_BufferSize;
        private System.Windows.Forms.RadioButton radio_ASIO_BufferSize_Custom;
        private System.Windows.Forms.RadioButton radio_ASIO_BufferSize_Host;
        private System.Windows.Forms.RadioButton radio_ASIO_BufferSize_Driver;
        private System.Windows.Forms.Label label_ASIO_CustomBufferSize;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_CustomBufferSize;
        private System.Windows.Forms.GroupBox groupBox_ASIO_Config;
        private System.Windows.Forms.CheckBox checkBox_ASIO_ASIO;
        private System.Windows.Forms.CheckBox checkBox_ASIO_WASAPI_Input;
        private System.Windows.Forms.CheckBox checkBox_ASIO_WASAPI_Output;
        private System.Windows.Forms.GroupBox groupBox_ASIO_Input0;
        private System.Windows.Forms.GroupBox groupBox_ASIO_Input1;
        private System.Windows.Forms.ListBox listBox_AvailableASIODevices_Input1;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_Input1_MaxVolume;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_Input1_Channel;
        private System.Windows.Forms.Label label_ASIO_Input1_Channel;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_Input0_Channel;
        private System.Windows.Forms.Label label_ASIO_Input0_Channel;
        private System.Windows.Forms.Label label_ASIO_Input1_MaxVolume;
        private System.Windows.Forms.Label label_ASIO_Input0_MaxVolume;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_Input0_MaxVolume;
        private System.Windows.Forms.GroupBox groupBox_ASIO_Output;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Output_ControlEndpointVolume;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Output_ControlMasterVolume;
        private System.Windows.Forms.Label label_ASIO_Output_MaxVolume;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_Output_MaxVolume;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_Output_BaseChannel;
        private System.Windows.Forms.Label label_ASIO_Output_BaseChannel;
        private System.Windows.Forms.ListBox listBox_AvailableASIODevices_Output;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Input1_ControlEndpointVolume;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Input1_ControlMasterVolume;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Input0_ControlEndpointVolume;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Input0_ControlMasterVolume;
        private System.Windows.Forms.Label label_ASIO_Credits;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Input1_Disabled;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Input0_Disabled;
        private System.Windows.Forms.CheckBox checkBox_ASIO_Output_Disabled;
        private System.Windows.Forms.Button button_ASIO_Output_ClearSelection;
        private System.Windows.Forms.Button button_ASIO_Input1_ClearSelection;
        private System.Windows.Forms.Button button_ASIO_Input0_ClearSelection;
        private System.Windows.Forms.TabPage tab_Rocksmith;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_DisableBrowser;
        private System.Windows.Forms.GroupBox groupBox_Rocksmith_Resolutions;
        private System.Windows.Forms.Label label_Rocksmith_RenderHeight;
        private System.Windows.Forms.Label label_Rocksmith_RenderWidth;
        private System.Windows.Forms.Label label_Rocksmith_ScreenHeight;
        private System.Windows.Forms.Label label_Rocksmith_ScreenWidth;
        private System.Windows.Forms.NumericUpDown nUpDown_Rocksmith_RenderHeight;
        private System.Windows.Forms.NumericUpDown nUpDown_Rocksmith_RenderWidth;
        private System.Windows.Forms.NumericUpDown nUpDown_Rocksmith_ScreenHeight;
        private System.Windows.Forms.NumericUpDown nUpDown_Rocksmith_ScreenWidth;
        private System.Windows.Forms.GroupBox groupBox_Rocksmith_VisualQuality;
        private System.Windows.Forms.RadioButton radio_Rocksmith_HighQuality;
        private System.Windows.Forms.RadioButton radio_Rocksmith_MediumQuality;
        private System.Windows.Forms.RadioButton radio_Rocksmith_LowQuality;
        private System.Windows.Forms.GroupBox groupBox_Rocksmith_VisualSettings;
        private System.Windows.Forms.GroupBox groupBox_Rocksmith_Fullscreen;
        private System.Windows.Forms.RadioButton radio_Rocksmith_ExclusiveFullScreen;
        private System.Windows.Forms.RadioButton radio_Rocksmith_NonExclusiveFullScreen;
        private System.Windows.Forms.RadioButton radio_Rocksmith_Windowed;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_GamepadUI;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_Shadows;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_PostEffects;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_DepthOfField;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_HighResScope;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_MSAASamples;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_PerPixelLighting;
        private System.Windows.Forms.GroupBox groupBox_Rocksmith_AudioSettings;
        private System.Windows.Forms.GroupBox groupBox_Rocksmith_Network;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_UseProxy;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_Override_MaxOutputBufferSize;
        private System.Windows.Forms.Label label_Rocksmith_MaxOutputBuffer;
        private System.Windows.Forms.Label label_Rocksmith_LatencyBuffer;
        private System.Windows.Forms.NumericUpDown nUpDown_Rocksmith_MaxOutputBuffer;
        private System.Windows.Forms.NumericUpDown nUpDown_Rocksmith_LatencyBuffer;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_EnableMicrophone;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_ForceDirextXSink;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_ForceWDM;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_ExclusiveMode;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_RTCOnly;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_DumpAudioLog;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_LowLatencyMode;
        private System.Windows.Forms.CheckBox checkBox_Rocksmith_EnableRenderRes;
        private System.Windows.Forms.Label label_Rocksmith_Thanks;
        private System.Windows.Forms.GroupBox groupBox_ASIO_InputMic;
        private System.Windows.Forms.Button button_ASIO_InputMic_ClearSelection;
        private System.Windows.Forms.CheckBox checkBox_ASIO_InputMic_Disabled;
        private System.Windows.Forms.CheckBox checkBox_ASIO_InputMic_ControlEndpointVolume;
        private System.Windows.Forms.CheckBox checkBox_ASIO_InputMic_ControlMasterVolume;
        private System.Windows.Forms.Label label_ASIO_InputMic_MaxVolume;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_InputMic_MaxVolume;
        private System.Windows.Forms.NumericUpDown nUpDown_ASIO_InputMic_Channel;
        private System.Windows.Forms.Label label_ASIO_InputMic_Channel;
        private System.Windows.Forms.ListBox listBox_AvailableASIODevices_InputMic;
        private System.Windows.Forms.Label label_SetAndForgetGuitarArcade;
        private System.Windows.Forms.Label label_HorizontalRulerGuitarArcade;
        private System.Windows.Forms.RadioButton radio_ScaleWarriorsTone;
        private System.Windows.Forms.RadioButton radio_TempleOfBendsTone;
        private System.Windows.Forms.Button button_AssignNewGuitarArcadeTone;
        private System.Windows.Forms.RadioButton radio_HurtlinHurdlesTone;
        private System.Windows.Forms.RadioButton radio_NinjaSlideNTone;
        private System.Windows.Forms.RadioButton radio_ScaleRacerTone;
        private System.Windows.Forms.RadioButton radio_StringsSkipSaloonTone;
        private System.Windows.Forms.RadioButton radio_HarmonicHeistTone;
        private System.Windows.Forms.RadioButton radio_RainbowLaserTone;
        private System.Windows.Forms.RadioButton radio_DucksReduxTone;
        private System.Windows.Forms.RadioButton radio_GoneWailinTone;
        private System.Windows.Forms.GroupBox groupBox_AutoLoadProfiles;
        private System.Windows.Forms.ListBox listBox_AutoLoadProfiles;
        private System.Windows.Forms.Button button_GuitarSpeak_DeleteSavedValue;
        private System.Windows.Forms.Button button_AutoLoadProfile_ClearSelection;
        private System.Windows.Forms.CheckBox checkBox_BackupProfile;
        private System.Windows.Forms.GroupBox groupBox_Backups;
        private System.Windows.Forms.CheckBox checkBox_UnlimitedBackups;
        private System.Windows.Forms.NumericUpDown nUpDown_NumberOfBackups;
        private System.Windows.Forms.CheckBox checkBox_RainbowNotes;
        private System.Windows.Forms.Label label_RainbowNotesKey;
        private System.Windows.Forms.TabControl TabController_ModSettings;
        private System.Windows.Forms.TabPage tabPage_ModSettings_ER;
        private System.Windows.Forms.TabPage tabPage_ModSettings_Automation;
        private System.Windows.Forms.TabPage tabPage_ModSettings_Misc;
        private System.Windows.Forms.CheckBox checkBox_CustomHighway;
        private System.Windows.Forms.GroupBox groupBox_CustomHighway;
        private System.Windows.Forms.TextBox textBox_ShowFretNumber;
        private System.Windows.Forms.TextBox textBox_ShowNotewayGutter;
        private System.Windows.Forms.TextBox textBox_ShowUnNumberedFrets;
        private System.Windows.Forms.TextBox textBox_ShowNumberedFrets;
        private System.Windows.Forms.Button button_ChangeFretNumber;
        private System.Windows.Forms.Button button_ChangeNotewayGutter;
        private System.Windows.Forms.Button button_ChangeUnNumberedFrets;
        private System.Windows.Forms.Button button_ChangeNumberedFrets;
        private System.Windows.Forms.Button button_ResetNotewayColors;
    }
}

