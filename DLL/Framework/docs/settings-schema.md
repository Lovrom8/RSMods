# Settings schema (plan)

> **Status: planned, not built.** This is the design for the last decoupling surface — mod-owned
> configuration. Every other seam (lifecycle, activation, commands, HUD, menu, draw) already lets a
> contributor add a mod in one `.cpp`; settings are the one thing still spread across core files it
> doesn't own. Build it during the Avalonia GUI migration, not after.

## The problem

A mod owns its behavior but not its configuration. Adding one setting today means editing three core
files the mod does not own:

1. `Settings.hpp` — a `constexpr char[]` key in the central `Setting::` namespace.
2. `Settings.cpp` — a hand-written line in the INI-load map that hardcodes the section, the on-disk
   key, and the default: `{Setting::X, reader.GetLongValue("Mod Settings", "X", 17)}`.
3. `GUI` / `GUI.Avalonia` / `GUI.Core` — a hand-placed control so a user can actually flip it.

The behavioral surface is pluggable; the configuration surface is not. A mod nobody can configure isn't
usable by an end user, only by a compiler — so this seam, not the C ABI, is what stands between the
framework and "one file, zero core edits."

## What it is (and isn't)

- **Is:** a declarative descriptor a mod returns from `IMod`, naming each setting it reads — in-code
  key, on-disk `{section, key}`, type, default, and a GUI label/hint. The framework aggregates these
  and *generates* what is hand-maintained today: the INI-load defaults and the GUI control.
- **Isn't:** a runtime enforcement boundary. `ModContext::IsOn(string_view)` already resolves any key
  by string; nothing stops an in-tree mod from reading an undeclared key. Review is the trust boundary
  (see `abi-shim` decision), exactly as with the HUD and draw registries — the schema makes the clean
  path the obvious one and lets tooling flag the dirty one, it does not wall it off.

## The descriptor

```cpp
namespace Framework {
    enum class SettingType { Bool, Int, Enum, String, Color };

    struct SettingDecl {
        std::string_view key;              // in-code key; matches a Settings::Setting constant
        struct { std::string_view section, name; } ini;  // on-disk location (may differ from key)
        SettingType type = SettingType::Bool;
        std::string_view def;              // default as its INI string form ("0", "17", ...)
        std::string_view label;            // GUI label
        std::string_view hint = {};        // optional GUI tooltip
        std::vector<std::string_view> choices = {};  // Enum only: display order = underlying int
    };
}
```

`ini` stays explicit because a setting's on-disk key can differ from its in-code key (`ToggleLoft` on
disk vs `ToggleLoftEnabled` in code — see the note at `Settings.hpp:15`); the schema does not get to
paper over that. `def` is the INI *string* form so one field feeds both the typed default and the GUI's
initial value without a second conversion table.

## IMod integration

One optional virtual, defaulting to empty so nothing existing has to change at once:

```cpp
virtual std::vector<SettingDecl> Settings() const { return {}; }
```

Declared in the mod's own `.cpp` alongside its `MOD_ID` and `ClaimsExclusive`, so the mod is once again
the single place its authors look. The registry harvests every mod's `Settings()` at
`InstantiatePending`/`DispatchInitialize` time into one aggregate schema, rejecting duplicate keys the
same way it rejects duplicate mod IDs.

## What the framework generates from it

1. **INI-load defaults (DLL).** The hand-written `reader.GetLongValue(section, key, default)` block in
   `Settings.cpp` becomes a loop over the aggregate schema: for each decl, read `ini.{section,name}`
   with `def` as the fallback, typed by `type`. The bespoke lines retire as mods declare their schema;
   the map is built from data instead of transcription.

   **Round-trip safety is a hard constraint.** The INI backend must stay lossless — it preserves keys
   and comments it doesn't recognize (see `[[gui-little-cleanup-review-fixes]]`). The schema drives
   *reads and defaults*; it must never become an exclusive writer that drops unknown keys. A key absent
   from every schema is still loaded and still written back untouched.

2. **GUI controls (Avalonia).** The panel renders from the schema instead of a hand-placed control per
   setting. This is the reason to do it *now*: the GUI is mid-rewrite to Avalonia
   (`[[avalonia-migration-cfsm2-reference]]`). A data-driven panel is cheaper to build once during that
   rewrite than to hand-port every existing control and retrofit later — miss the window and every
   control is authored twice. The rendering strategy is not a one-liner (gating, unit scales, runtime
   device lists, and bespoke editors all exist today), so it has its own section below:
   *GUI: rendering the manifest*.

3. **Key-access validation (tooling, optional).** Because `ModContext` accessors take a `string_view`,
   a typo is a silent miss today. With a schema in hand, a debug-build check (or a unit test over the
   aggregate) can warn when a mod reads a key it never declared — the compile-time safety the
   `constexpr` namespace gives, without forcing every key through it.

## Crossing the C++ / C# boundary (the real open question)

The schema is authored in C++ (in the mod) but the GUI is C# (`GUI.Avalonia` / `GUI.Core`), and the GUI
runs as its own process without the game loaded. The schema therefore has to *cross* to C#. Options,
with a recommendation:

- **(Recommended) Generated manifest.** The DLL exposes an exported entry point that serializes the
  aggregate schema to JSON; a build step (or a committed, regenerated `mods.manifest.json`) produces the
  file the GUI reads. The mod stays the single source of truth; the manifest is a generated artifact,
  never hand-edited. Keep the serializer in-box (`DataContractJsonSerializer` on the C# side), never a
  loose managed dep — the installer stays single-exe (`[[installer-single-exe-no-loose-deps]]`).
- **Shared data file.** Both languages read one `settings.json`. Rejected: the mod no longer owns its
  config in its `.cpp`, which is the whole point.
- **P/Invoke at GUI startup.** GUI loads the DLL and queries the schema live. Rejected for now: it
  couples the config UI to a loadable game DLL and complicates the standalone GUI build.

Settle this before writing code — it decides whether `Settings()` returns live objects or is a codegen
input.

## GUI: rendering the manifest

The Avalonia port made the GUI *nicer* to maintain but did not make it data-driven. `ModSettingsViewModel`
is still a hand-written class with one `[ObservableProperty]` per setting, hand-coded `Show* => otherToggle`
gating, per-setting `RsModsLimits` ranges, and a matching hand-placed control in `ModSettingsView.axaml`.
So a mod is transcribed in *three* C# places (VM property, XAML control, load/save mapping) on top of the
three C++ places — the same fixed wall, rebuilt in cleaner bricks. Rendering the manifest is what removes
it. But the existing VM proves a flat "loop and emit a checkbox" is too naive; the approach is **tiered**,
mirroring the DLL side's declarative-default-plus-escape-hatch.

- **Tier 1 — pure declarative fields.** `Bool` / static `Enum` / `Int` with `min`/`max` / plain `String`.
  The majority of toggles. Render straight from the manifest with **zero** hand-code; a new mod's control
  appears automatically. `min`/`max`/`choices` come from the manifest, retiring the duplicate copies in
  `RsModsLimits.cs` and `Enums.cs` for these fields.
- **Tier 2 — declarative + a named runtime hook.** Three things the current VM does that a static list
  can't hold literally: **gating** (`ShowRewindSettings => AllowRewind`) → a `visibleWhen {key, equals}`
  on the descriptor; **unit scale** (stored ms, shown seconds, `/1000` on load) → a `scale`/`unit`;
  **runtime choice lists** (fonts, MIDI/input devices, profiles) → a `choicesSource` naming a provider the
  GUI owns, *not* a literal `choices`. Still data-driven; the GUI supplies provider/transform by name.
- **Tier 3 — custom-editor escape hatch.** Things that are not "a setting" but a bespoke sub-UI (Guitar
  Speak note-mapping, color swatches, tuning strings, the MIDI cluster, ASIO, Twitch) and genuinely
  encoded values (extended-range `value = -index-2`, guitar-speak `note = index+36+octave*12`). The
  descriptor says `editor: "GuitarSpeak"`; the GUI keeps a registry mapping that name → a `UserControl`.
  These stay hand-written, and that is correct — do not force bespoke math into the schema.

This tiering is the same boundary as `plugin-distribution.md`: third-party / out-of-tree mods get tiers
1–2 (pure manifest, no C# needed), while tier-3 editors stay in-tree (an out-of-tree binary plugin can't
ship a C# editor anyway).

**Avalonia mechanics.** An `ObservableCollection<SettingFieldViewModel>` in `GUI.Core` with a base VM and
`Bool`/`Enum`/`Numeric`/`Choice` derivations, each holding the descriptor + live value + an `IsVisible`
computed from its `visibleWhen`. An `ItemsControl` over the collection with **per-type `DataTemplate`s**
(Avalonia matches on `DataType`) picks the control; a `category` field renders the section headers that
replace the hand-laid group boxes. Load/save becomes **generic** — iterate the field VMs, read/write each
key through the existing round-trip-safe `IniManager` / `FlatKeyValueSettingsStore`, which already
preserves unknown keys, so nothing regresses.

## The manifest contract

One generated `mods.manifest.json` is the single source both sides consume — the DLL for defaults/load,
the GUI for controls. Agree its per-setting shape before building either side; it is the coupling point.
First-cut fields:

```jsonc
{
  "key":        "AllowRewind",              // in-code key; matches a Settings::Setting constant
  "ini":        { "section": "Riff Repeater", "name": "AllowRewind" },
  "type":       "Bool",                     // Bool | Int | Enum | String | Color
  "default":    "0",                        // INI string form; feeds typed default AND GUI initial value
  "label":      "Allow rewind",
  "hint":       "",                         // optional tooltip
  "category":   "Riff Repeater",            // GUI section grouping
  "min":        null, "max": null,          // Int only
  "scale":      null,                       // Tier 2: e.g. 0.001 to show ms as seconds
  "choices":    [],                         // Enum: static display list, index = underlying int
  "choicesSource": null,                    // Tier 2: "SystemFonts" | "MidiOutDevices" | "Profiles" | ...
  "visibleWhen": null,                      // Tier 2: { "key": "...", "equals": "..." }
  "editor":     null                        // Tier 3: custom UserControl name; when set, other UI fields ignored
}
```

The DLL authors this from each mod's `SettingDecl` (the C++ struct is the subset the DLL needs — `key`,
`ini`, `type`, `default`; the GUI-only fields ride alongside in the manifest). Because the GUI runs without
the DLL loaded, commit the manifest as a build artifact: a `--dump-manifest` DLL entrypoint regenerates it,
it is embedded as a resource in `GUI.Core`, and CI checks it is current — exactly the pattern the framework
tests already use so it can't rot out of sync.

## Migration phases

The DLL and GUI halves share only the manifest, so once its shape is fixed (the contract above) they can
proceed in parallel. Sequenced end-to-end:

0. **DLL: land `SettingDecl` + the `IMod::Settings()` virtual + the aggregate harvest** with duplicate-key
   rejection and a unit test, mirroring how `HudRegistry`/`MenuRegistry` landed. No consumer yet — pure
   framework, unit-tested in isolation. This *defines* the manifest.
1. **DLL: one end-to-end mod.** Wire a single simple toggle mod (e.g. `GreenScreenWallMod`) through: it
   declares its schema, the `Settings.cpp` default line for that key comes from the schema instead of the
   hardcoded map, and `--dump-manifest` emits it. Prove the whole path on one mod the way the HUD registry
   proved on `VolumeDisplayMod`.
2. **GUI: the tier-1 renderer on one section.** Add the `SettingField` model + loader + the `Bool`
   `DataTemplate` in `GUI.Core`/`GUI.Avalonia`, and render **one** section (the "Fixes & misc" toggle pile
   in `ModSettingsViewModel`) from the manifest *alongside* the existing hand-written VM, deleting those
   properties as they move. This de-risks both the boundary decision and the dynamic-form approach on the
   cheapest possible slice.
3. **Both: fill in the tiers.** DLL declares enum/int/string schemas as mods adopt them; GUI adds the
   `Enum` + `Numeric` templates, then `visibleWhen` gating and `scale`, then `choicesSource` providers —
   migrating the when-clusters and device/font/profile pickers. Each setting's hand-written `Settings.cpp`
   line, VM property, and XAML control delete as it moves, exactly as the mod ports emptied `ModManager`.
4. **Leave tier-3 editors bespoke.** Guitar Speak, colors, tuning, MIDI, ASIO, Twitch keep their custom
   views behind an `editor:` name. Do not schematize them.

## What this deliberately does not do

- **Not the versioned C plugin ABI.** Community mods are reviewed in-tree source; review is the trust
  boundary, not a binary sandbox (`[[abi-shim-trust-boundary]]`, and the README's deferral still
  stands). The schema is a C++ surface authored in-tree like every other `IMod` method.
- **Not runtime enforcement.** A mod can still read an undeclared key. The schema makes configuration
  declarative and GUI-generatable; it does not police access.
- **Not a settings *storage* rewrite.** The INI backend, its round-trip guarantees, and the
  `shared_mutex` in `Settings.cpp` are untouched. The schema feeds the existing loader; it does not
  replace it.

## Why this is the last seam

After this, the three things a contributor touches to ship a mod — a `.cpp` in `Mods/`, its project
entry, and its settings — reduce to just the `.cpp`. The project-entry edit is a separate cheap win
(glob `Mods/*.cpp` in `DLL.vcxproj`). With both done, adding a mod is adding one file, and the
framework has delivered the contributor-facing surface it exists for — without ever shipping a binary
ABI it decided it didn't want.
