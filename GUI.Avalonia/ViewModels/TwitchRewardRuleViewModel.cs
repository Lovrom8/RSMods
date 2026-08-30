using CommunityToolkit.Mvvm.ComponentModel;
using RSMods.Twitch;

namespace RSMods.ViewModels;

internal sealed partial class TwitchRewardRuleViewModel : ObservableObject
{
    public int LocalId { get; }
    public string Name { get; }
    public string Description { get; }
    public string EffectCode { get; }
    public TwitchTriggerKind TriggerKind { get; }
    public string TriggerLabel => TriggerKind switch
    {
        TwitchTriggerKind.ChannelPoints => "Channel Points",
        TwitchTriggerKind.Subscription => "Subscription",
        _ => "Bits"
    };
    public bool UsesAmount => TriggerKind != TwitchTriggerKind.Subscription;
    public bool IsSolidNotes => string.Equals(EffectCode, "solidnotes", StringComparison.OrdinalIgnoreCase);
    public string SwatchHex => RandomColor ? string.Empty : HexColor;

    [ObservableProperty]
    private bool _enabled;

    [ObservableProperty]
    private string _durationText = "10";

    [ObservableProperty]
    private string _amountText = "1";

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(SwatchHex))]
    private bool _randomColor = true;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(SwatchHex))]
    private string _hexColor = "FFFFFF";

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(IsValid))]
    private string _validationMessage = string.Empty;

    public bool IsValid => string.IsNullOrEmpty(ValidationMessage);

    public TwitchRewardRuleViewModel(TwitchReward reward)
    {
        LocalId = TwitchRewardRules.GetLocalId(reward);
        Name = reward.Name ?? string.Empty;
        Description = reward.Description ?? string.Empty;
        EffectCode = reward.InternalMsgEnable ?? string.Empty;
        TriggerKind = TwitchRewardRules.GetTriggerKind(reward);
        Enabled = reward.Enabled;
        DurationText = reward.Length.ToString();
        AmountText = UsesAmount ? TwitchRewardRules.GetAmount(reward).ToString() : string.Empty;

        if (IsSolidNotes)
        {
            RandomColor = string.IsNullOrWhiteSpace(reward.AdditionalMsg) ||
                          string.Equals(reward.AdditionalMsg, "Random", StringComparison.OrdinalIgnoreCase);
            if (!RandomColor)
                HexColor = reward.AdditionalMsg;
        }

        Validate();
    }

    public TwitchRewardRuleViewModel(TwitchReward catalogEffect, TwitchTriggerKind triggerKind, int localId)
        : this(TwitchRewardRules.Create(
            catalogEffect,
            triggerKind,
            localId,
            durationSeconds: 10,
            amount: triggerKind == TwitchTriggerKind.Subscription ? 0 : 1,
            enabled: true,
            additionalMessage: string.Equals(catalogEffect.InternalMsgEnable, "solidnotes", StringComparison.OrdinalIgnoreCase)
                ? "Random"
                : string.Empty))
    {
    }

    public bool TryBuildReward(out TwitchReward? reward)
    {
        reward = null;
        if (!int.TryParse(DurationText, out int duration) || duration < 0)
        {
            ValidationMessage = "Duration must be a non-negative whole number of seconds.";
            return false;
        }

        int amount = 0;
        if (UsesAmount && (!int.TryParse(AmountText, out amount) || amount < 0))
        {
            ValidationMessage = "Amount must be a non-negative whole number.";
            return false;
        }

        string additionalMessage = string.Empty;
        if (IsSolidNotes)
            additionalMessage = RandomColor ? "Random" : (HexColor ?? string.Empty).Trim().ToUpperInvariant();

        var catalogEffect = new TwitchReward(Name, Description, EffectCode);
        TwitchReward candidate = TwitchRewardRules.Create(
            catalogEffect,
            TriggerKind,
            LocalId,
            duration,
            amount,
            Enabled,
            additionalMessage);
        if (!TwitchRewardRules.TryValidate(candidate, out string error))
        {
            ValidationMessage = error;
            return false;
        }

        ValidationMessage = string.Empty;
        reward = candidate;
        return true;
    }

    partial void OnEnabledChanged(bool value) => Validate();
    partial void OnDurationTextChanged(string value) => Validate();
    partial void OnAmountTextChanged(string value) => Validate();
    partial void OnRandomColorChanged(bool value) => Validate();
    partial void OnHexColorChanged(string value) => Validate();

    private void Validate() => TryBuildReward(out _);
}
