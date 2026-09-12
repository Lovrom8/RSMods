# Menu Registry

The framework-managed surface for in-game settings and debug menus, decoupling [Menu.cpp](file:///c:/Users/Lovro/Documents/Projekti/RSMods/DLL/Menu.cpp) from hardcoded mod implementations and global state.

## What it is (and isn't)

- **Is:** an owner-scoped registry mirroring `HudRegistry` and `CommandRouter`. A mod registers a menu drawer delegate through `ctx.Menu()`; the registry holds the ordered list of entries; the ImGui backend iterates and renders them safely. Entries are automatically removed on mod teardown or fault.
- **Is host-agnostic:** `MenuRegistry.hpp` and `MenuRegistry.cpp` do **not** include DirectX or ImGui headers. The registry stores opaque `MenuDrawFn = std::function<void()>` callbacks. The actual ImGui rendering occurs in `Menu::RenderImGuiMenu()` in `Menu.cpp`.
- **Isn't:** an invasive UI framework. Mods use standard Dear ImGui widget calls (`ImGui::SliderInt`, `ImGui::Button`, `ImGui::BeginCombo`, etc.) directly inside their registered draw functions.

---

## Architecture & Threading

1. **Registration (MainThread / Initialization):**
   - During `OnInitialize`, a mod calls `c.Menu().Register(...)` on `ModContext`.
   - The registration binds the mod's pointer as the entry owner and stores the ID, title, sort order, drawer lambda, availability requirement, and window mode.
2. **Snapshot & Availability (MainThread / D3D Render Thread):**
   - ImGui rendering runs on the Direct3D 9 thread in `D3DHooks::Hook_EndScene` via `Menu::RenderImGuiMenu()`.
   - `ModRegistry` publishes an owner availability snapshot from `MainThread` to `MenuRegistry::PublishAvailability` whenever mod lifecycle states settle.
   - `MenuRegistry::GetEntries()` snapshots registered entries under an internal mutex, filtered against the published availability snapshot, and sorts them by `order asc`, then `title asc` without querying `ModRegistry` from the render thread.
3. **Execution & Fault Isolation:**
   - For non-standalone entries (`standaloneWindow = false`), `Menu.cpp` opens the unified `"RS Mods"` window and wraps each drawer in an `ImGui::CollapsingHeader(entry.title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)`.
   - Each drawer invocation is wrapped in a `try / catch` block so an exception or widget error in one mod's menu does not crash the render frame, corrupt DirectX state, or disrupt other mod menus.

---

## Mod Registration API

From inside any `IMod`'s `OnInitialize(Framework::ModContext& c)`:

```cpp
c.Menu().Register(
    "unique_id",               // Unique string ID for this menu section
    "Display Title",           // Title shown in header or window
    10,                        // Sort order (lower numbers appear higher)
    [this] { DrawMenu(); },    // Drawer delegate
    Availability::Active,      // Availability requirement (Active or Initialized)
    false                      // standaloneWindow: false = collapsing header in "RS Mods", true = own window
);
```

### Availability Modes
- **`Availability::Active` (Default):** The menu entry is only visible and rendered when the mod is in `ModState::Active` (`IsEnabled() == true` and winning all exclusive resources). If the mod is disabled or suppressed, the menu entry disappears from the UI.
- **`Availability::Initialized`:** The menu entry is visible as long as the mod initialized successfully, even if its runtime feature is disabled in the configuration (e.g. `MicrophoneVolumeOverrideMod`, allowing device testing without having volume sync enabled).

---

## Mod Teardown & Lifecycle Cleanup

When a mod is disabled, faulted, or torn down during shutdown:
- `ModRegistry::Fault` and `ModRegistry::BeginTeardown` invoke `Framework::Menus().RemoveMod(record.mod.get())`.
- All registered menu entries owned by that mod are immediately and safely unregistered.

---

## Active Consumers

| Mod Owner | ID | Title | Order | Availability Gate | Window Mode | Description |
|---|---|---|---|---|---|---|
| `MidiMod` | `midi` | MIDI | 10 | `Active` | Collapsible Header | Port selection, PC/CC slider testing, and manual message dispatch. |
| `CalibrationMod` | `calibration` | Calibration | 15 | `Active` | Collapsible Header | Wwise noise floor, tone balance, and input volume calibration sliders. |
| `MicrophoneVolumeOverrideMod` | `microphones` | Microphones | 20 | `Initialized` | Collapsible Header | Active input device selector and volume test button. |
| `VoiceOverControlMod` | `voicelines` | Voicelines | 30 | `Active` | Collapsible Header | Results screen voice-over line dropdown preview and playback. |

---

## Testing

`MenuRegistry` is unit-tested in isolation in [DLL/Framework/Tests/MenuRegistryTests.cpp](file:///c:/Users/Lovro/Documents/Projekti/RSMods/DLL/Framework/Tests/MenuRegistryTests.cpp). It verifies:
- Registration, upserting, and retrieval of multiple entries.
- Stable sort ordering (`order asc`, then `title asc`).
- Safe drawer execution.
- Owner availability gating (entries are filtered out when the owner mod is inactive).
- Owner-scoped removal on teardown.
