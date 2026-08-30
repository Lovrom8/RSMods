# Twitch migration plan

A step-by-step plan for moving the WinForms Twitch Bot tab (`GUI/UI.Twitch.cs` and `GUI/Twitch/`)
to Avalonia. This is written as a handoff that can be implemented one slice at a time while the
WinForms frontend remains usable.

Twitch was the final user-facing configurator feature to migrate. As of 2026-08-11 its code migration is
complete, with the live Twitch/Rocksmith checklist retained as a release-acceptance task. The migration
document also mentions live MIDI-in listening, but that checkbox only sends incoming MIDI messages to
`Debug.WriteLine`; it is intentionally retired from migration scope.

## The key decision: replace PubSub instead of porting it

### Historical note: the implicit authorization flow

The retired `ImplicitAuth`/localhost callback implementation was a deliberate workaround for an important
constraint at the time: a desktop application could not safely ship Twitch client-secret credentials.
Capturing the browser redirect fragment allowed RSMods to authorize without embedding or disclosing those
private keys. Although the approach became legacy and was difficult to maintain, it addressed the security
problem creatively until Twitch provided the supported Device Code Grant for public clients. The code remains
available in Git history.

The current implementation is built around `TwitchLib.PubSub`, but Twitch permanently shut down
legacy PubSub on 2025-04-14. Copying `PubSub.cs` into `GUI.Core` would therefore preserve code that can
no longer deliver live events.

Use Twitch EventSub over WebSockets instead:

- `channel.cheer` for Bits (`bits:read`).
- `channel.channel_points_custom_reward_redemption.add` for custom Channel Point redemptions
  (`channel:read:redemptions`).
- `channel.subscribe` for new and gifted subscriptions (`channel:read:subscriptions`).
- `channel.subscription.message` for resubscriptions (`channel:read:subscriptions`).

WebSockets fit the installed desktop app better than webhooks: they do not require RSMods to expose a
public HTTPS endpoint. The client must handle the EventSub welcome/session ID, subscription creation,
keepalives, reconnect messages, connection loss, subscription revocation, and duplicate notification
IDs.

For authentication, use Twitch's Device Code Grant with a **public** Twitch application registration.
It does not embed a client secret, provides refresh tokens, and avoids the current loopback server plus
URL-fragment JavaScript workaround. The required scopes are only the three listed above; remove the
unused `chat:read` and `channel:read:hype_train` scopes.

Official references:

- [Twitch PubSub shutdown and EventSub migration](https://dev.twitch.tv/docs/pubsub)
- [EventSub WebSocket lifecycle](https://dev.twitch.tv/docs/eventsub/handling-websocket-events)
- [EventSub subscription types and scopes](https://dev.twitch.tv/docs/eventsub/eventsub-subscription-types/)
- [Device Code Grant](https://dev.twitch.tv/docs/authentication/getting-tokens-oauth/#device-code-grant-flow)
- [Token validation requirements](https://dev.twitch.tv/docs/authentication/validate-tokens/)

## Why this is the thorny screen

1. **It is a runtime service, not just a settings page.** The tab currently owns authentication,
   Twitch event listening, periodic validation, reconnect behavior, logging, reward matching, and a
   localhost TCP server used by the injected DLL.
2. **The current Twitch transport is retired.** `GUI/Twitch/PubSub.cs` and its 2020-era TwitchLib
   packages must be replaced with EventSub, not moved as-is.
3. **Secrets and settings are unsafe.** The access token is stored in plaintext in
   `GUI_Settings.ini`, can be revealed and copied with the credentials, and may be included in support
   messages. `TwitchSettings.SaveSettings` and `Constants.SaveBaseSettings` both rewrite the entire
   file, so either frontend can erase keys it does not know about.
4. **Background work has no coherent lifetime.** Constructors start fire-and-forget work, the UI uses
   `async void`, reconnect handlers call `Thread.Sleep`, the 150-second timer may repeatedly open an
   authorization browser, and sockets/listeners are not reliably disposed at application shutdown.
5. **The effect bridge is a real protocol boundary.** Rocksmith connects to a TCP listener on
   `127.0.0.1:45659` and exchanges null-terminated JSON. The replacement must preserve the native
   `DLL/CC/ControlServer.cpp` contract, including effect codes, durations, RGB parameters, response
   statuses, retries, and connection recovery.
6. **The reward grid hides domain behavior.** Bits and Channel Point rules match exact amounts, while
   every enabled subscription rule runs for a subscription. Solid Notes adds an RGB/random parameter,
   and changing TurboSpeed's enabled state sends a separate game message.

## Current behavior to preserve

| Trigger | Existing match rule | EventSub source |
| --- | --- | --- |
| Bits | Run every enabled `BitsReward` whose `BitsAmount` exactly equals the cheer amount | `channel.cheer` |
| Channel Points | Run every enabled `ChannelPointsReward` whose `PointsAmount` exactly equals the redeemed custom reward cost | `channel.channel_points_custom_reward_redemption.add` |
| Subscription | Run every enabled `SubReward`; the configured amount is not used | `channel.subscribe` plus `channel.subscription.message` |

The 23-entry built-in effect catalog in `TwitchSettings.LoadDefaultEffects` remains the source of
selectable effects. Existing `TwitchEnabledEffects.xml` files must continue to load without user
conversion or data loss.

The following legacy pieces are not product behavior and should not be ported:

- `GUI/Twitch/RSModderBot.cs`: unreferenced experimental chatbot code that uses a third-party token
  generator.
- `GUI/Twitch/PubSubImplicit.cs`: unreferenced earlier authentication prototype.
- Reveal/copy access-token controls and the “copy credentials for developers” action.
- “Force reauth” and its 150-second timer. Replace this with normal refresh/validation plus an explicit
  Reauthorize action.

---

## Step 0 - Shared Core foundation (no Avalonia UI)

**Status: completed 2026-08-11.** The shared models, catalog, matcher, XML repository, flat settings
store, DPAPI token store, native TCP bridge, WinForms compatibility wiring, and Core test suite are in
place.

### 0.1 Move the domain model, not the UI singleton

Create `GUI.Core/Twitch/` and move/refactor the framework-neutral pieces into it:

- `TwitchReward`, `BitsReward`, `ChannelPointsReward`, and `SubReward`.
- The Crowd Control request/response DTOs currently in `EffectMessage.cs`.
- A `TwitchRewardCatalog` that returns a fresh immutable/read-only list of the 23 stock effects. Do not
  append to a singleton list on each load.
- A `TwitchRewardRepository` that loads and atomically saves the existing polymorphic XML schema at
  `TwitchEnabledEffects.xml`.
- A normalized `TwitchTriggerEvent` model containing event ID, trigger kind, viewer, amount/cost, and
  timestamp.
- A pure `TwitchRewardMatcher` that maps a normalized event to all matching enabled rewards.

Keep these types free of `INotifyPropertyChanged`, `SynchronizationContext`, WinForms, and Avalonia.
The frontends own observable collections and UI-thread dispatch.

### 0.2 Make `GUI_Settings.ini` round-trip-safe

Add a small flat key/value store for the unsectioned `GUI_Settings.ini` format. It must preserve unknown
keys, comments, blank lines, and ordering while updating named entries atomically. Use it from both:

- `Constants.SaveBaseSettings`, so Avalonia startup no longer erases Twitch or future settings.
- The WinForms Twitch compatibility layer for non-secret values such as username/channel ID during the
  transition.

Do not force this file through `IniManager`: `IniManager` intentionally ignores keys outside a section,
while `GUI_Settings.ini` is an unsectioned file.

### 0.3 Introduce protected token storage

Add a `TwitchTokenStore` for the access token, refresh token, expiry, scopes, and client ID. Because the
current migration is Windows-first, protect its payload with Windows DPAPI for the current user and save
it atomically beside the GUI settings. Add the conditional net8.0 `ProtectedData` package/reference that
the multi-targeted Core project requires.

On first load, import a valid legacy plaintext `AccessToken` once, then remove or blank that key using the
round-trip-safe store. Never include either token in logs, property notifications, exceptions shown to
the user, clipboard text, or exported diagnostics. If the legacy token belongs to a different client ID
or lacks the EventSub scopes, request one explicit reauthorization.

### 0.4 Replace `EffectServerTCP` with a lifecycle-safe effect bridge

Implement an application-scoped `RocksmithEffectServer : IAsyncDisposable` in Core:

- Listen only on `127.0.0.1:45659` and accept the game connection asynchronously.
- Use one ordered writer queue and unique request IDs.
- Preserve null-terminated JSON framing and correctly handle partial or multiple response frames.
- Preserve the native effect-code mapping, Solid Notes random/RGB parameters, and seconds-to-duration
  semantics after verifying them against `DLL/CC/ControlServer.cpp` and `DLL/CC/CCEffect.cpp`.
- Treat native status `3` as retryable with cancellation-aware bounded retry/backoff; do not retry
  forever or grow an unbounded queue.
- Expose connection state and structured log events without referencing either UI framework.
- Close the listener, client, stream, queue, and worker tasks deterministically on stop/disposal.

The test-reward path should be able to start/use this bridge without Twitch authorization.

### 0.5 Add focused Core tests

Add a net8.0 `GUI.Core.Tests` project (or an equivalent repository-standard test project if one is added
first) covering:

- Loading and saving the existing reward XML, including all three derived reward types.
- Catalog initialization being idempotent.
- Exact Bits/Points matching and subscription fan-out.
- Flat settings updates preserving unknown lines and comments.
- Legacy token import without exposing the token in output.
- Effect JSON for ordinary and Solid Notes rewards, null framing, response parsing, retry status, and
  cancellation using a loopback fake client.

Repoint the WinForms project to the shared models/repository/bridge, but leave its current Twitch event
source in place until Step 1 is ready. Build both Core target frameworks and both frontends before adding
Avalonia UI.

---

## Step 1 - Modern Twitch runtime and WinForms compatibility

**Status: completed 2026-08-11.** Core now owns Device Code authorization, public-client token refresh,
startup/hourly validation, identity lookup, EventSub WebSockets, notification normalization and bounded
deduplication, reconnect/revocation handling, bounded logs, reward matching, and the application-scoped
runtime. WinForms starts and stops that runtime independently of tab navigation and uses Device Code
authorization. The automated suite has 20 passing tests. A real Twitch smoke test remains part of Step 4
because it requires the RSMods developer application to be configured as a public Twitch client and a
test broadcaster account. Step 2 is the next implementation slice.

### 1.1 Authentication and token maintenance

Add Core services with injected `HttpClient`, clock/delay abstractions where tests need them, and explicit
cancellation:

- `TwitchAuthService` starts Device Code Grant, returns the verification URI/user code to the frontend,
  polls at Twitch's requested interval, and supports cancellation/expiry.
- `TwitchApiClient` validates tokens and obtains the authorized user/channel identity.
- Refresh access tokens before expiry, atomically rotate the one-use refresh token, and fall back to a
  visible Reauthorize-required state when refresh fails.
- Validate at startup and hourly while a Twitch session is active, as Twitch requires. Stop EventSub and
  clear the active authenticated session on revocation/invalid token.

Centralize the public client ID and requested scopes in one options object. A real end-to-end test requires
the RSMods Twitch developer application to be registered/configured as a public client; no client secret
may be committed or shipped.

### 1.2 EventSub WebSocket client

Implement `TwitchEventSubClient : IAsyncDisposable` using `ClientWebSocket` plus the Helix subscription
endpoint:

- Connect to `wss://eventsub.wss.twitch.tv/ws`.
- On `session_welcome`, create the four subscriptions listed above with the returned `session_id` and the
  user access token.
- Parse notifications into `TwitchTriggerEvent`; keep raw Twitch payload DTOs internal.
- Deduplicate **all** notifications by EventSub `message_id` with a bounded TTL cache. EventSub is
  at-least-once delivery, so this replaces the current unbounded Bits-only hash list.
- Track keepalive deadlines, follow Twitch-provided reconnect URLs without duplicating subscriptions,
  and reconnect/resubscribe with capped exponential backoff after an unexpected disconnect.
- Surface revocation and scope errors as structured state. Never reconnect in a tight loop and never use
  `Thread.Sleep`.
- Verify gift/new-sub/resub behavior with Twitch's documented payloads so a gift is not executed twice.

### 1.3 Application-scoped orchestration

Add one `TwitchService : IAsyncDisposable` that owns auth state, EventSub, reward matching, the effect
bridge, bounded logs, and start/stop gates. There must be at most one WebSocket session and one TCP
listener per process.

The service is application-scoped rather than page-scoped: an already-authorized streamer must receive
events even if the Twitch page has never been opened. Startup should launch it independently so a slow or
offline Twitch connection does not block the rest of the configurator.

Adapt `GUI/UI.Twitch.cs` to this shared service before adding Avalonia controls. This proves the new runtime
against the complete existing UI and restores real Twitch behavior before the frontend is changed. Remove
the timer-based resubscribe/force-reauth behavior, `SynchronizationContext` singleton plumbing, and
fire-and-forget constructors.

---

## Step 2 - Avalonia screen shell, authorization, status, and log

**Status: completed 2026-08-11.** Avalonia now starts the application-scoped `TwitchService` after normal
startup, exposes the Twitch page through the main navigation, and provides authorization/listening controls,
device-code browser UX, account/EventSub/Rocksmith status, identity, and a bounded clearable/saveable log.
The view-model owns only UI state and dispatcher marshalling and leaves the runtime alive across navigation.
Step 3 is the next implementation slice.

Add `TwitchViewModel`/`TwitchView` and register them in:

- `GUI.Avalonia/App.axaml.cs` (service + view-model lifetime).
- `MainWindowViewModel` (typed page and navigation command).
- `Views/MainWindow.axaml` (data template and “Twitch” navigation entry).

The first UI slice contains:

- Connection cards for Twitch authorization/EventSub state and the Rocksmith effect-bridge state.
- Authorized username/channel identity. Do not display an access or refresh token.
- Authorize/Reauthorize, Cancel authorization, Disconnect/Forget authorization, Start listening, and Stop
  listening actions with correct busy/can-execute state.
- Device authorization UX: show the user code and verification URI, open it via
  `Process.Start(... UseShellExecute = true)`, and continue polling asynchronously until success,
  cancellation, or expiry.
- A bounded, timestamped log view that follows new entries without binding one ever-growing string.
- Clear log and Save Log, using `IDialogService.PickSaveFileAsync` rather than a hard-coded path.

The view-model subscribes to Core state/events and marshals collection/property updates through Avalonia's
dispatcher. It must unsubscribe/dispose its UI subscriptions, but it must not dispose the application-wide
Twitch runtime when the user navigates away.

This slice lands navigation and proves auth, token refresh, startup auto-connect, explicit disconnect,
EventSub state, offline/error presentation, and clean app shutdown before the reward editor is ported.

---

## Step 3 - Reward catalog and enabled-rule editor

**Status: completed 2026-08-11.** Avalonia now has the stock catalog and editable persisted rules with an
explicit trigger choice, inline numeric/color validation, stable legacy IDs, save/revert/remove/test
actions, and Solid Notes preview. Both frontends update the shared runtime through one Core path, which
also owns the TurboSpeed side effect. The Core suite has 21 passing tests. Step 4 subsequently completed
the legacy cleanup and hardening pass.

Port the two-grid workflow as a proper MVVM editor:

- A read-only stock effect catalog showing Name and Description.
- An enabled-rules `DataGrid` backed by `ObservableCollection<TwitchRewardRuleViewModel>` with Enabled,
  Effect, Duration, Trigger Type, Amount/Cost, and local ID columns.
- An explicit trigger selector (`Subscription`, `Bits`, or `Channel Points`) when adding a catalog effect;
  do not reuse customized MessageBox button captions.
- Amount is editable/required only for Bits and Channel Points. Subscription rules do not pretend their
  amount affects matching.
- Non-negative numeric validation and overflow/wire-range validation belong in Core and are surfaced by
  the view-model without allowing a bad edit to crash the page.
- Remove, Save, and Revert actions with dirty tracking. Saves go through the atomic XML repository and
  remain readable by the adapted WinForms frontend.
- Solid Color Notes shows Random plus a six-digit RGB editor/live swatch. Reuse the existing Avalonia hex
  validation/swatch approach instead of adding a UI-framework dependency solely for a color picker.
- Test Reward dispatches the selected rule through `RocksmithEffectServer`, includes a synthetic viewer
  label, and reports “game not connected” through bridge state rather than process-name polling.
- Keep the TurboSpeed enable/disable side effect in one Core rule-update path so WinForms and Avalonia do
  not diverge or send duplicate game messages.

Prefer a stable model/local ID over resolving rows by display name and subtype-specific ID. If the XML
schema cannot gain a common ID without breaking compatibility, add an in-memory adapter that maps the
existing `BitsID`/`PointsID`/`SubID` fields consistently and preserve those fields on disk.

---

## Step 4 - Hardening, legacy cleanup, and migration completion

**Status: code complete 2026-08-11; real-account acceptance pending.** The retired PubSub, implicit-auth,
localhost auth-server, chatbot, and response-model files are gone; the repurposed WinForms adapter is now
named `TwitchRuntime.cs`. The five TwitchLib packages, redundant `System.Net.Http` package, and unused
`System.Web` reference were removed. WinForms no longer has reveal/copy-token controls or its obsolete
reauthorization timer. The 21 Core tests and all three build targets pass without warnings. A live Twitch
broadcaster plus RSMods-enabled Rocksmith was not available in this workspace, so the manual checklist below
remains the release-acceptance gate.

After both frontends use the shared runtime and the Avalonia editor is complete:

1. Delete retired/unreferenced implementations:
   - `GUI/Twitch/PubSub.cs`
   - `GUI/Twitch/PubSubImplicit.cs`
   - `GUI/Twitch/RSModderBot.cs`
   - `GUI/Twitch/Auth/ImplicitAuth.cs`
   - `GUI/Twitch/Auth/WebServer.cs`
   - WinForms-local model/settings/effect-server files that now live in Core
2. Remove the obsolete `TwitchLib`, `TwitchLib.Api.Core`, `TwitchLib.Api.Helix.Models`,
   `TwitchLib.Client`, and `TwitchLib.PubSub` package references. Remove `System.Web` only if the final
   reference search confirms nothing else uses it.
3. Remove the WinForms reveal/copy-token controls and Force Reauth timer while retaining the WinForms tab
   as a frontend over the shared service until the project formally switches to Avalonia.
4. Scrub exported logs/errors and verify no token values are emitted. Document the one-time plaintext-token
   migration and what “Forget authorization” removes.
5. Update `docs/avalonia-migration.md`: mark Twitch complete, state that live MIDI-in debug listening is
   intentionally retired, and move the remaining work to installer/publishing/default-frontend decisions.
6. Run a real Twitch smoke test with a test broadcaster account plus offline/reconnect tests. Do not require
   real credentials in automated tests or commit generated token/settings files.

### Authorization storage and removal

On the first post-migration load, `LoadOrImportLegacy` reads the old plaintext `AccessToken` setting once,
writes it to `TwitchAuth.dat` encrypted for the current Windows user with DPAPI, and removes the active INI
key while preserving unrelated settings and comments. “Forget authorization” stops EventSub, deletes that
encrypted token file, and clears the in-memory token and Twitch identity. It does not delete reward rules or
other RSMods settings.

Runtime and effect-bridge logs are bounded and contain curated status text rather than OAuth payloads. Saved
logs export only that bounded view; the automated service test asserts that access and refresh token values
do not appear in it.

## Cross-cutting rules

- `GUI.Core` must not reference WinForms or Avalonia.
- Keep the WinForms Twitch tab buildable and working after every step.
- Frontends own dialogs, navigation, observable state, clipboard/browser actions, and UI-thread dispatch.
- Core owns auth/session state, persistence, Twitch payload parsing, reward matching, TCP framing, retries,
  and cancellation.
- No `async void` outside UI event handlers, constructor-started work, blocking `.Wait()`, `Thread.Sleep`,
  swallowed background exceptions, or unbounded queues/caches/logs.
- No access token, refresh token, client secret, or full authorization response in UI text, logs, clipboard,
  exception dialogs, test snapshots, or source control.
- Preserve unknown `GUI_Settings.ini` content and existing `TwitchEnabledEffects.xml` data.
- Network loss is a normal state: keep the rest of the app usable, expose status, and reconnect with bounds.
- Each step builds `GUI.Core` for both target frameworks, `GUI.Avalonia`, and WinForms with zero new warnings.

## Verification checklist

Automated:

```powershell
dotnet test GUI.Core.Tests/GUI.Core.Tests.csproj
dotnet build GUI.Core/GUI.Core.csproj --no-restore
dotnet build GUI.Avalonia/GUI.Avalonia.csproj --no-restore
MSBuild GUI/GUI.csproj /t:Build /p:Configuration=Debug /p:Platform=x64 /p:PostBuildEvent= /v:minimal
```

Manual, using a Twitch test broadcaster and a running RSMods-enabled Rocksmith instance:

1. First authorization, app restart/token refresh, explicit reauthorization, disconnect, denied/expired
   device code, and revoked authorization.
2. One Bits event, one custom Channel Point redemption, one new sub, one gift sub, and one resub; confirm
   each expected rule runs once and duplicate EventSub message IDs do not run twice.
3. EventSub planned reconnect, forced network drop, offline startup, and recovery without reopening the page.
4. Add/edit/remove/save/revert each reward type; restart both frontends and confirm the same XML is loaded.
5. Solid Notes Random/custom RGB, TurboSpeed enable/disable, ordinary timed effects, native retry status,
   game restart/reconnect, Test Reward without Twitch auth, and clean application shutdown.
6. Inspect `GUI_Settings.ini`, token storage, saved logs, and UI/clipboard output for data loss or leaked tokens.

## Definition of completion

The Twitch migration is complete when the Avalonia app can authorize safely, listen through EventSub,
edit and persist all three reward-rule types, dispatch/test every stock effect through the native TCP
protocol, recover from Twitch/game disconnects, and shut down cleanly; the WinForms frontend uses the same
Core runtime; all retired TwitchLib/implicit-auth/chatbot code is removed; existing reward XML survives;
and the migration document no longer lists a user-facing feature gap.
