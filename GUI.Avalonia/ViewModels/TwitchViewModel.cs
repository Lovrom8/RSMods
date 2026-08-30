using System.Collections.ObjectModel;
using System.Diagnostics;
using System.ComponentModel;
using Avalonia.Threading;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Twitch;

namespace RSMods.ViewModels;

internal sealed partial class TwitchViewModel : ObservableObject, IDisposable
{
    private readonly TwitchService _service;
    private readonly IDialogService _dialogs;
    private readonly TwitchRewardRepository _rewardRepository;
    private CancellationTokenSource? _authorizationCancellation;
    private bool _initialized;
    private bool _disposed;

    public ObservableCollection<TwitchLogEntry> LogEntries { get; } = [];
    public IReadOnlyList<TwitchReward> CatalogEffects { get; } = TwitchRewardCatalog.CreateDefaults();
    public ObservableCollection<TwitchRewardRuleViewModel> RewardRules { get; } = [];
    public IReadOnlyList<string> TriggerChoices { get; } = ["Subscription", "Bits", "Channel Points"];

    [ObservableProperty]
    private TwitchLogEntry? _selectedLogEntry;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AddRuleCommand))]
    private TwitchReward? _selectedCatalogEffect;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(RemoveRuleCommand))]
    [NotifyCanExecuteChangedFor(nameof(TestRuleCommand))]
    private TwitchRewardRuleViewModel? _selectedRule;

    [ObservableProperty]
    private string _selectedTriggerChoice = "Subscription";

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(SaveRulesCommand))]
    [NotifyCanExecuteChangedFor(nameof(RevertRulesCommand))]
    private bool _rulesDirty;

    [ObservableProperty]
    private string _rulesStatus = string.Empty;

    [ObservableProperty]
    private string _authenticationStatus = "Not authorized";

    [ObservableProperty]
    private string _eventSubStatus = "Stopped";

    [ObservableProperty]
    private string _rocksmithStatus = "Waiting for Rocksmith";

    [ObservableProperty]
    private string _statusDetail = string.Empty;

    [ObservableProperty]
    private string _username = string.Empty;

    [ObservableProperty]
    private string _channelId = string.Empty;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(HasIdentity))]
    private bool _isAuthorized;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(AuthorizeButtonText))]
    [NotifyCanExecuteChangedFor(nameof(ForgetAuthorizationCommand))]
    private bool _hasStoredAuthorization;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AuthorizeCommand))]
    [NotifyCanExecuteChangedFor(nameof(CancelAuthorizationCommand))]
    [NotifyCanExecuteChangedFor(nameof(ForgetAuthorizationCommand))]
    [NotifyCanExecuteChangedFor(nameof(StartListeningCommand))]
    [NotifyCanExecuteChangedFor(nameof(StopListeningCommand))]
    [NotifyCanExecuteChangedFor(nameof(AddRuleCommand))]
    [NotifyCanExecuteChangedFor(nameof(RemoveRuleCommand))]
    [NotifyCanExecuteChangedFor(nameof(SaveRulesCommand))]
    [NotifyCanExecuteChangedFor(nameof(RevertRulesCommand))]
    [NotifyCanExecuteChangedFor(nameof(TestRuleCommand))]
    private bool _isBusy;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AuthorizeCommand))]
    [NotifyCanExecuteChangedFor(nameof(CancelAuthorizationCommand))]
    [NotifyCanExecuteChangedFor(nameof(ForgetAuthorizationCommand))]
    private bool _isAuthorizing;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(OpenVerificationPageCommand))]
    private string _userCode = string.Empty;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(OpenVerificationPageCommand))]
    private string _verificationUri = string.Empty;

    public bool HasIdentity => IsAuthorized && !string.IsNullOrWhiteSpace(ChannelId);
    public string AuthorizeButtonText => HasStoredAuthorization ? "Reauthorize" : "Authorize";

    public TwitchViewModel(
        TwitchService service,
        IDialogService dialogs,
        TwitchRewardRepository rewardRepository)
    {
        _service = service;
        _dialogs = dialogs;
        _rewardRepository = rewardRepository;
        _service.StateChanged += OnStateChanged;
        _service.LogReceived += OnLogReceived;

        foreach (TwitchLogEntry entry in _service.GetLogSnapshot())
            LogEntries.Add(entry);
        SelectedLogEntry = LogEntries.LastOrDefault();
        ApplyCurrentState();
    }

    public async Task InitializeAsync()
    {
        if (_initialized)
            return;
        _initialized = true;

        try
        {
            LoadRules();
            await _service.StartAsync();
            ApplyCurrentState();
        }
        catch (Exception ex)
        {
            StatusDetail = "The Twitch runtime could not be started.";
            await _dialogs.ShowErrorAsync(ex.Message, "Twitch startup");
        }
    }

    private bool CanAddRule => !IsBusy && SelectedCatalogEffect != null;

    [RelayCommand(CanExecute = nameof(CanAddRule))]
    private async Task AddRuleAsync()
    {
        if (SelectedCatalogEffect == null)
            return;

        int nextId;
        try
        {
            nextId = RewardRules.Count == 0 ? 0 : checked(RewardRules.Max(rule => rule.LocalId) + 1);
        }
        catch (OverflowException)
        {
            await _dialogs.ShowErrorAsync("No more Twitch reward IDs are available.", "Add Twitch reward");
            return;
        }

        TwitchTriggerKind triggerKind = SelectedTriggerChoice switch
        {
            "Bits" => TwitchTriggerKind.Bits,
            "Channel Points" => TwitchTriggerKind.ChannelPoints,
            _ => TwitchTriggerKind.Subscription
        };
        var rule = new TwitchRewardRuleViewModel(SelectedCatalogEffect, triggerKind, nextId);
        AttachRule(rule);
        RewardRules.Add(rule);
        SelectedRule = rule;
        RulesDirty = true;
        RulesStatus = string.Empty;
    }

    private bool CanRemoveRule => !IsBusy && SelectedRule != null;

    [RelayCommand(CanExecute = nameof(CanRemoveRule))]
    private void RemoveRule()
    {
        if (SelectedRule == null)
            return;
        SelectedRule.PropertyChanged -= OnRuleChanged;
        RewardRules.Remove(SelectedRule);
        SelectedRule = null;
        RulesDirty = true;
        RulesStatus = string.Empty;
    }

    private bool CanSaveRules => !IsBusy && RulesDirty;

    [RelayCommand(CanExecute = nameof(CanSaveRules))]
    private async Task SaveRulesAsync()
    {
        var rewards = new List<TwitchReward>(RewardRules.Count);
        foreach (TwitchRewardRuleViewModel rule in RewardRules)
        {
            if (!rule.TryBuildReward(out TwitchReward? reward) || reward == null)
            {
                SelectedRule = rule;
                RulesStatus = rule.ValidationMessage;
                return;
            }
            rewards.Add(reward);
        }

        IsBusy = true;
        try
        {
            await _rewardRepository.SaveAsync(rewards);
            _service.SetRewards(rewards);
            RulesDirty = false;
            RulesStatus = "Reward rules saved.";
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(ex.Message, "Save Twitch rewards");
        }
        finally
        {
            IsBusy = false;
        }
    }

    private bool CanRevertRules => !IsBusy && RulesDirty;

    [RelayCommand(CanExecute = nameof(CanRevertRules))]
    private void RevertRules()
    {
        LoadRules();
        RulesStatus = "Reverted to the last saved reward rules.";
    }

    private bool CanTestRule => !IsBusy && SelectedRule != null;

    [RelayCommand(CanExecute = nameof(CanTestRule))]
    private void TestRule()
    {
        if (SelectedRule == null || !SelectedRule.TryBuildReward(out TwitchReward? reward) || reward == null)
        {
            RulesStatus = SelectedRule?.ValidationMessage ?? "Select a reward rule to test.";
            return;
        }
        if (!_service.IsRocksmithConnected)
        {
            RulesStatus = "Rocksmith is not connected to the effect bridge.";
            return;
        }

        RulesStatus = _service.TryQueueReward(reward, "Avalonia test")
            ? $"Queued test: {reward.Name}."
            : "The reward could not be queued.";
    }

    private void LoadRules()
    {
        foreach (TwitchRewardRuleViewModel existing in RewardRules)
            existing.PropertyChanged -= OnRuleChanged;
        RewardRules.Clear();

        foreach (TwitchReward reward in _rewardRepository.Load())
        {
            var rule = new TwitchRewardRuleViewModel(reward);
            AttachRule(rule);
            RewardRules.Add(rule);
        }

        SelectedRule = RewardRules.FirstOrDefault();
        RulesDirty = false;
        RulesStatus = string.Empty;
    }

    private void AttachRule(TwitchRewardRuleViewModel rule) => rule.PropertyChanged += OnRuleChanged;

    private void OnRuleChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName is nameof(TwitchRewardRuleViewModel.ValidationMessage) or
            nameof(TwitchRewardRuleViewModel.IsValid) or nameof(TwitchRewardRuleViewModel.SwatchHex))
            return;
        RulesDirty = true;
        RulesStatus = string.Empty;
    }

    private bool CanAuthorize => !IsBusy && !IsAuthorizing;

    [RelayCommand(CanExecute = nameof(CanAuthorize))]
    private async Task AuthorizeAsync()
    {
        _authorizationCancellation?.Cancel();
        _authorizationCancellation?.Dispose();
        _authorizationCancellation = new CancellationTokenSource();
        CancellationToken cancellationToken = _authorizationCancellation.Token;

        IsBusy = true;
        IsAuthorizing = true;
        UserCode = string.Empty;
        VerificationUri = string.Empty;
        try
        {
            TwitchDeviceAuthorization authorization = await _service.BeginAuthorizationAsync(cancellationToken);
            UserCode = authorization.UserCode;
            VerificationUri = authorization.VerificationUri.AbsoluteUri;
            await OpenVerificationPageAsync();
            await _service.CompleteAuthorizationAsync(authorization, cancellationToken);
            UserCode = string.Empty;
            VerificationUri = string.Empty;
        }
        catch (OperationCanceledException)
        {
            StatusDetail = "Authorization cancelled.";
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(ex.Message, "Twitch authorization");
        }
        finally
        {
            IsAuthorizing = false;
            IsBusy = false;
            ApplyCurrentState();
        }
    }

    private bool CanCancelAuthorization => IsAuthorizing;

    [RelayCommand(CanExecute = nameof(CanCancelAuthorization))]
    private void CancelAuthorization() => _authorizationCancellation?.Cancel();

    private bool CanOpenVerificationPage => !string.IsNullOrWhiteSpace(VerificationUri);

    [RelayCommand(CanExecute = nameof(CanOpenVerificationPage))]
    private async Task OpenVerificationPageAsync()
    {
        if (!Uri.TryCreate(VerificationUri, UriKind.Absolute, out Uri? uri))
            return;

        try
        {
            Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true });
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(
                $"Could not open the browser. Open this address manually:\n{uri}\n\n{ex.Message}",
                "Open Twitch authorization");
        }
    }

    private bool CanForgetAuthorization => !IsBusy && !IsAuthorizing && HasStoredAuthorization;

    [RelayCommand(CanExecute = nameof(CanForgetAuthorization))]
    private async Task ForgetAuthorizationAsync()
    {
        if (!await _dialogs.ShowConfirmAsync(
                "Forget the stored Twitch authorization and stop listening for events?",
                "Forget Twitch authorization"))
            return;

        IsBusy = true;
        try
        {
            await _service.ForgetAuthorizationAsync();
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(ex.Message, "Twitch");
        }
        finally
        {
            IsBusy = false;
            ApplyCurrentState();
        }
    }

    private bool CanStartListening =>
        !IsBusy && IsAuthorized && _service.EventSubState is TwitchEventSubState.Stopped or TwitchEventSubState.Error;

    [RelayCommand(CanExecute = nameof(CanStartListening))]
    private async Task StartListeningAsync()
    {
        IsBusy = true;
        try
        {
            await _service.StartListeningAsync();
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(ex.Message, "Twitch EventSub");
        }
        finally
        {
            IsBusy = false;
            ApplyCurrentState();
        }
    }

    private bool CanStopListening =>
        !IsBusy && _service.EventSubState is TwitchEventSubState.Connecting or
            TwitchEventSubState.Connected or TwitchEventSubState.Reconnecting;

    [RelayCommand(CanExecute = nameof(CanStopListening))]
    private async Task StopListeningAsync()
    {
        IsBusy = true;
        try
        {
            await _service.StopListeningAsync();
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(ex.Message, "Twitch EventSub");
        }
        finally
        {
            IsBusy = false;
            ApplyCurrentState();
        }
    }

    [RelayCommand]
    private void ClearLog()
    {
        _service.ClearLog();
        LogEntries.Clear();
        SelectedLogEntry = null;
    }

    [RelayCommand]
    private async Task SaveLogAsync()
    {
        string? path = await _dialogs.PickSaveFileAsync(
            "Save Twitch log", "Twitch.log", "Log file", "*.log");
        if (string.IsNullOrWhiteSpace(path))
            return;

        try
        {
            string[] lines = LogEntries.Select(entry => entry.ToString()).ToArray();
            await File.WriteAllLinesAsync(path, lines);
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(ex.Message, "Save Twitch log");
        }
    }

    private void OnStateChanged(object? sender, TwitchStateChangedEventArgs e) =>
        RunOnUiThread(() => ApplyState(e));

    private void OnLogReceived(object? sender, TwitchLogEventArgs e) =>
        RunOnUiThread(() =>
        {
            LogEntries.Add(e.Entry);
            while (LogEntries.Count > 500)
                LogEntries.RemoveAt(0);
            SelectedLogEntry = e.Entry;
            RocksmithStatus = _service.IsRocksmithConnected ? "Connected" : "Waiting for Rocksmith";
        });

    private void ApplyCurrentState() => ApplyState(new TwitchStateChangedEventArgs(
        _service.AuthenticationState,
        _service.EventSubState,
        _service.Identity,
        StatusDetail));

    private void ApplyState(TwitchStateChangedEventArgs state)
    {
        AuthenticationStatus = DescribeAuthentication(state.AuthenticationState);
        EventSubStatus = DescribeEventSub(state.EventSubState);
        RocksmithStatus = _service.IsRocksmithConnected ? "Connected" : "Waiting for Rocksmith";
        StatusDetail = state.Detail;
        Username = state.Identity?.DisplayName ?? state.Identity?.Login ?? string.Empty;
        ChannelId = state.Identity?.UserId ?? string.Empty;
        IsAuthorized = state.AuthenticationState == TwitchAuthenticationState.Authorized;
        HasStoredAuthorization = _service.HasStoredAuthorization;
        AuthorizeCommand.NotifyCanExecuteChanged();
        ForgetAuthorizationCommand.NotifyCanExecuteChanged();
        StartListeningCommand.NotifyCanExecuteChanged();
        StopListeningCommand.NotifyCanExecuteChanged();
    }

    private static string DescribeAuthentication(TwitchAuthenticationState state) => state switch
    {
        TwitchAuthenticationState.Authorizing => "Authorizing",
        TwitchAuthenticationState.Authorized => "Authorized",
        TwitchAuthenticationState.ReauthorizationRequired => "Reauthorization required",
        TwitchAuthenticationState.Error => "Unavailable; retrying",
        _ => "Not authorized"
    };

    private static string DescribeEventSub(TwitchEventSubState state) => state switch
    {
        TwitchEventSubState.Connecting => "Connecting",
        TwitchEventSubState.Connected => "Listening",
        TwitchEventSubState.Reconnecting => "Reconnecting",
        TwitchEventSubState.Revoked => "Revoked",
        TwitchEventSubState.Error => "Connection error",
        _ => "Stopped"
    };

    private static void RunOnUiThread(Action action)
    {
        if (Dispatcher.UIThread.CheckAccess())
            action();
        else
            Dispatcher.UIThread.Post(action);
    }

    public void Dispose()
    {
        if (_disposed)
            return;
        _disposed = true;
        _authorizationCancellation?.Cancel();
        _authorizationCancellation?.Dispose();
        foreach (TwitchRewardRuleViewModel rule in RewardRules)
            rule.PropertyChanged -= OnRuleChanged;
        _service.StateChanged -= OnStateChanged;
        _service.LogReceived -= OnLogReceived;
    }
}
