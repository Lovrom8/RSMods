# Plugin C ABI — sketch against `ModContext` (2026-10-13)

Not built, not scheduled. This sketches what step 3 of `plugin-distribution.md` would freeze, so the cost is
visible ahead of time. The triggers in that doc still decide *when*.

The shape is chosen for room to change later:

1. **Per-subsystem interfaces.** One tiny bootstrap table that never changes, plus
   `getInterface("rsmods.hud", 1)` for everything else. Each subsystem versions on its own.
2. **Settings schema as JSON**, in the manifest's own shape. The fastest-growing struct leaves the ABI, and the
   same JSON becomes the GUI's manifest fragment.
3. **A conformance plugin** built once against the frozen header and run against every host build, to catch
   the breaks no compiler sees.

## Rules every table follows

- **`extern "C"`, `__cdecl`, POD only.** The DLL is Win32, where the calling convention is part of the ABI, so
  every function pointer names it. No `std::` type, reference, exception or virtual call crosses.
- **Two version numbers per interface.** The `version` passed to `getInterface` is that interface's major:
  the host returns exactly that version or NULL, never "something newer". `structSize` is the minor: fields
  are only appended, and a caller checks `structSize` before touching one newer than its header.
- **Zero means the old behaviour.** A field an older side doesn't have reads as zero (or NULL / empty), so
  every appended field's zero value must keep things working as they did before it existed.
- **Calls that may grow take a struct.** A function with more than a few arguments takes one size-tagged
  struct (`RSKeyBinding`, `RSMenuDesc`) instead, so a new option is a field at the end rather than a new
  function or a new major.
- **A break is a new major beside the old one.** `hud` v2 ships while the host keeps serving `hud` v1,
  implemented as a thin adapter over v2. Old plugins keep loading. A major is retired only on a published
  schedule (say, kept for at least three host releases after its successor ships).
- **No arrays of growable structs.** `arr[i]` strides by the *reader's* `sizeof`, so an older side's smaller
  elements would be misread. Growable structs travel singly or as arrays of pointers. `RSStr` has no
  `structSize`, so arrays of it are fine, and it is frozen for good.
- **Enums are append-only.** A side that receives a value it doesn't know rejects the call (`RSErr`) rather
  than guessing.
- **Borrowed strings.** `RSStr` is UTF-8 pointer + length, valid for the call only. The host copies what it
  keeps. Strings coming back out go into a caller's buffer (`snprintf`-style: returns the length needed).
- **Opaque handles.** `RSMod` replaces `const IMod*`; `RSCtx` replaces `ModContext&` and is valid only during
  the call that received it.
- **Callbacks are `fn + void* user`** in place of `std::function`.
- **Errors are return codes.** A plugin callback returns `RSErr` where in-tree code would throw, and the host
  maps it to `Faulted`, which keeps fault isolation. The SDK's C++ wrapper does the `catch (...)`. An access
  violation inside a plugin still takes the game down, exactly like an in-tree mod today.
- **Every function states its thread.** Moving a call to another thread is a break even when the signature
  doesn't change, so it needs a new major (and the conformance plugin checks it).
- **A plugin imports nothing from the host and touches no game memory before `onInitialize`.** That keeps a
  plugin DLL loadable outside the game, which the schema dump below relies on.

## Bootstrap — `RSModsPlugin.h`

The only part that can never change. Everything else hangs off `getInterface`.

```c
#pragma once
#include <stdint.h>

#define RSCALL __cdecl

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RSMod_* RSMod; // A mod the plugin registered
typedef struct RSCtx_* RSCtx; // Per-hook context; valid only during the call that received it

typedef struct { const char* ptr; uint32_t len; } RSStr;
#ifdef __cplusplus // String literals only; compound literals are C-only
#define RS_STR(lit) (RSStr{ (lit), (uint32_t)(sizeof(lit) - 1) })
#else
#define RS_STR(lit) ((RSStr){ (lit), (uint32_t)(sizeof(lit) - 1) })
#endif
typedef enum { RSOk = 0, RSErr = 1 } RSResult;

// Every interface table starts with this.
typedef struct {
	uint32_t structSize; // Minor: grows as fields are appended
	uint32_t version;     // Major: the one requested
} RSInterfaceHeader;

typedef struct {
	uint32_t structSize;
	RSStr    hostVersion; // "RSMods 1.2.3.4", for logs only; never branch on it
	// Exactly this major of the named interface, or NULL. Any thread, any time after RSModsPluginMain starts.
	const RSInterfaceHeader* (RSCALL *getInterface)(RSStr name, uint32_t version);
} RSHost;

// The single export. Fetch interfaces, register mods, return 1. Return 0 to refuse loading, after logging why.
// Runs on MainThread before any mod initializes; may also run outside the game (schema dump), so no game access.
__declspec(dllexport) int32_t RSCALL RSModsPluginMain(const RSHost* host);

#ifdef __cplusplus
}
#endif
```

## Interfaces, v1

Each lives in its own header (`RSModsHud.h`, ...) and versions on its own.

### `rsmods.mods` v1 — registration and logging (mirrors `IMod`)

```c
typedef enum { RSPhaseLoading, RSPhaseMenu, RSPhaseSong } RSGamePhase;

typedef RSResult (RSCALL *RSHookFn)(void* self, RSCtx ctx);

typedef struct {
	uint32_t     structSize;
	RSStr        id;              // Unique; logs, duplicate detection, conflict tie-break
	int32_t      priority;
	const RSStr* claims; uint32_t claimCount;          // ClaimsExclusive
	RSStr        settingsJson;   // JSON array of setting objects, see "Settings schema"; empty = none

	int32_t (RSCALL *isEnabled)(void* self, RSCtx ctx); // Cheap, side-effect free; NULL = always
	// Any may be NULL. RSErr = Faulted, under the same rules as in-tree mods. All on MainThread.
	RSHookFn onInitialize, onShutdown, onSettingsChanged;
	RSHookFn onEnabled, onDisabled;
	RSHookFn onTick, onMenuTick, onSongEnter, onSongTick, onSongExit;
} RSModDesc;

typedef struct {
	RSInterfaceHeader hdr;
	// From RSModsPluginMain only. Copies `desc` and everything it points to; `self` goes back to each hook.
	// NULL when the id is taken or the settings JSON is rejected (the reason is logged).
	RSMod       (RSCALL *registerMod)(const RSModDesc* desc, void* self);
	void        (RSCALL *log)(RSMod mod, RSStr message);  // Any thread
	RSGamePhase (RSCALL *phase)(RSCtx ctx);               // MainThread
	void        (RSCALL *requestFastTick)(RSCtx ctx);   // MainThread
} RSModsV1;
```

### `rsmods.settings` v1 — reads (mirrors `IsOn` / `Int` / `Value`)

```c
typedef struct {
	RSInterfaceHeader hdr;
	// MainThread. Any declared key, the plugin's own or core's.
	int32_t  (RSCALL *isOn)(RSCtx ctx, RSStr key);
	int32_t  (RSCALL *getInt)(RSCtx ctx, RSStr key);
	uint32_t (RSCALL *getString)(RSCtx ctx, RSStr key, char* buf, uint32_t cap); // Returns length needed
} RSSettingsV1;
```

### `rsmods.commands` v1 (mirrors `CommandBinder`)

```c
typedef enum { RSEdgeDown, RSEdgeUp } RSKeyEdge;
typedef enum { RSAvailActive, RSAvailInitialized } RSAvailability;

typedef struct {
	uint32_t  structSize;
	uint32_t  virtualKey;
	RSKeyEdge edge;
	uint8_t   control, shift, alt, repeat;
} RSKeyEvent;

typedef RSResult (RSCALL *RSKeyActionFn)(void* user, RSCtx ctx, const RSKeyEvent* ev);    // MainThread
typedef int32_t  (RSCALL *RSKeyPredicateFn)(void* user, RSCtx ctx, const RSKeyEvent* ev); // NULL = always

typedef struct {
	uint32_t         structSize;
	RSStr            keySetting;   // bindSetting: the Key setting that holds the key name
	RSStr            name;         // bindKey: the command's name
	uint32_t         virtualKey;   // bindKey: the fixed key
	RSKeyEdge        edge;         // Zero = RSEdgeDown
	RSAvailability   availability; // Zero = RSAvailActive
	RSKeyActionFn    action;
	RSKeyPredicateFn predicate;    // NULL = always
	void*            user;
	RSStr            logMessage;   // Empty = none
} RSKeyBinding;

typedef struct {
	RSInterfaceHeader hdr;
	// From onInitialize only. Each reads only its own fields from RSKeyBinding.
	RSResult (RSCALL *bindSetting)(RSCtx ctx, const RSKeyBinding* binding);
	RSResult (RSCALL *bindKey)(RSCtx ctx, const RSKeyBinding* binding);
} RSCommandsV1;
```

### `rsmods.hud` v1 (mirrors `HudBinder`)

```c
typedef enum {
	RSHudTopLeft, RSHudTopCenter, RSHudTopRight,
	RSHudTopTuning, RSHudHighwayLeft, RSHudMenuBanner
} RSHudAnchor;

typedef struct {
	uint32_t    structSize;
	RSHudAnchor anchor;
	int32_t     order;
	int32_t     visible;
	RSStr       text;
	uint32_t    colorArgb;
	int32_t     fontHeight;        // 0 = shared default font
} RSHudText;

typedef struct {
	RSInterfaceHeader hdr;
	void (RSCALL *set)(RSCtx ctx, RSStr id, const RSHudText* snapshot); // MainThread; cleared on deactivation
} RSHudV1;
```

### `rsmods.audio` v1 (mirrors `AudioBinder` / `IInputProcessor`)

```c
typedef struct { uint32_t structSize, sampleRate, channelCount, maxFrames; } RSCaptureFormat;

typedef struct {
	uint32_t structSize;
	RSResult (RSCALL *prepare)(void* self, const RSCaptureFormat* fmt);     // Off the audio thread
	void     (RSCALL *process)(void* self, float* samples, uint32_t frames, // Audio thread: must not
	                           int32_t active);                             // allocate, lock or fail
	uint32_t (RSCALL *latencyFrames)(void* self);                          // Constant, active or not
} RSInputProcessor;

typedef struct {
	RSInterfaceHeader hdr;
	// From onInitialize only; `vt` and `self` must outlive the plugin.
	RSResult (RSCALL *addProcessor)(RSCtx ctx, const RSInputProcessor* vt, void* self, int32_t order);
	int32_t  (RSCALL *tapLive)(void); // Any thread
} RSAudioV1;
```

### `rsmods.imgui` v1 — optional (mirrors `MenuBinder`)

`getInterface` returns NULL unless the host was built with ImGui; the plugin then also checks
`imguiVersionNum` and runs `IMGUI_CHECKVERSION()` (it compares struct sizes) before using it. A plugin that
gets NULL or a mismatch skips its windows and still loads. An ImGui upgrade in the host is a new major of
this interface; every other interface is unaffected.

```c
#include "RSModsCommands.h" // RSAvailability

typedef RSResult (RSCALL *RSMenuDrawFn)(void* user); // Render thread, inside the host's ImGui frame

enum { RSMenuStandalone = 1u << 0, RSMenuPlayerFacing = 1u << 1 };

typedef struct {
	uint32_t       structSize;
	RSStr          id;
	RSStr          title;        // registerMenu only
	int32_t        order;
	RSMenuDrawFn   draw;
	void*          user;
	RSAvailability availability; // registerMenu only; zero = RSAvailActive
	uint32_t       flags;        // registerMenu only; RSMenu* bits, zero = an entry in the shared menu
} RSMenuDesc;

typedef struct {
	RSInterfaceHeader hdr;
	int32_t  imguiVersionNum;  // IMGUI_VERSION_NUM the host was built with
	void*    (RSCALL *context)(void);
	void     (RSCALL *allocators)(void** allocFn, void** freeFn, void** user);
	// From onInitialize only.
	RSResult (RSCALL *registerMenu)(RSCtx ctx, const RSMenuDesc* menu);
	RSResult (RSCALL *registerOverlay)(RSCtx ctx, const RSMenuDesc* overlay); // Every frame while Active
} RSImGuiV1;
```

### Example: a plugin that tolerates an older host

```c
static const RSHudV1* hud;

int32_t RSCALL RSModsPluginMain(const RSHost* host) {
	const RSModsV1* mods = (const RSModsV1*)host->getInterface(RS_STR("rsmods.mods"), 1);
	if (!mods) return 0;
	hud = (const RSHudV1*)host->getInterface(RS_STR("rsmods.hud"), 1); // NULL = run without a HUD line
	// A field added to RSHudV1 later is used only after:
	//   hud->hdr.structSize >= offsetof(RSHudV1, newField) + sizeof(hud->newField)
	...
}
```

The C++ SDK wraps that check as `RS_HAS(hud, newField)`.

## Settings schema as JSON

`RSModDesc::settingsJson` is a JSON array of setting objects in the **input half of the manifest contract**
(`settings-schema.md`): `key`, `ini`, `type`, `default`, `label`, `hint`, `category`, `min`, `max`, `scale`,
`choices`, `choicesSource`, `visibleWhen`, `heading`, `entryTitle`, `listedUnder`. The host derives `mod` and
`entry` itself, as it does for in-tree mods.

- **Growth costs no ABI change.** A new schema field is a new JSON key. An older host ignores keys it doesn't
  know and logs them once; a plugin that omits a newer key gets the default, as in-tree mods do.
- **Plugin-only rules:** `editor` and `editedBy` are rejected, since they name GUI controls only core can
  ship. Duplicate keys are already rejected by `SettingsSchemaRegistry::Register`. Plugins should keep their
  INI entries in their own section (`"Plugin.<Id>"`), and the loader warns when they don't.
- **The C++ SDK keeps the builders.** Plugin authors still write `Toggle("X", "Label").Hint(...)`; the SDK
  serializes it with the same code as `DumpManifestJson`. Nobody writes the JSON by hand.

**The GUI gets plugins' settings from the same JSON.** The GUI runs without the game, so it can't ask a
running host. Instead a small Win32 tool (`RSMods.PluginSchemaDump.exe`) loads the plugin DLL, calls
`RSModsPluginMain` with a stub host, collects every `settingsJson`, and writes
`RSMods.Plugins.X.manifest.json` next to the DLL. Plugin authors run it as a post-build step. The GUI merges
every fragment it finds beside the game with its embedded `mods.manifest.json`, rejecting duplicate keys
across them. At load time the real host compares the plugin's JSON with its fragment and warns on a mismatch
(a stale fragment means the GUI shows the wrong settings).

## Conformance plugin

`DLL/Framework/Tests/PluginConformance/` builds `RSMods.Plugins.Conformance.dll` against a frozen copy of the
v1 headers. **The built binary is committed**, not rebuilt from source: what it guards is "an old binary
still works", and rebuilding would hide exactly the layout and toolchain drift it exists to catch.

A framework test loads it through the real loader, drives fake game phases, and checks:

- every v1 interface resolves, and its `structSize` is at least the frozen one;
- hooks fire in the documented order, on the documented thread (the plugin records the thread id per call);
- `RSErr` from each hook ends in `Faulted`, with the same revert rules as in-tree mods;
- settings JSON round-trips: the plugin's keys appear in the schema, and reads return the defaults;
- HUD, command and audio calls reach their registries with the right owner.

A second binary, built against a header with an invented `rsmods.hud` v2 and an extra `RSHudV1` field, checks
the other direction: the host returns NULL for the unknown major, and the plugin's `RS_HAS` check skips the
missing field. Retiring an interface major means deleting its checks here, which makes the retirement a
visible, reviewed change.

## What maps, what doesn't

| `ModContext` / `IMod` | v1 interface | Notes |
|---|---|---|
| Lifecycle hooks, `IsEnabled`, `Priority`, `ClaimsExclusive` | `rsmods.mods` | Mechanical. Arbitration and Faulted keep working because the host still drives every call. |
| `Settings()` schema | `RSModDesc::settingsJson` | Minus `editor`/`editedBy`. |
| `IsOn` / `Int` / `Value` | `rsmods.settings` | |
| `When` / `ColorMode` / `NoteColorMode` | None | Typed shortcuts over core's own settings enums. A plugin reads the raw value with `getInt`; freezing core's enums would tie the ABI to `Settings.hpp`. |
| `RequestFastTick`, `phase` | `rsmods.mods` | |
| `Commands()` | `rsmods.commands` | `KeyEvent` is already POD. |
| `Hud()` | `rsmods.hud` | Already a copyable snapshot. |
| `Audio()` | `rsmods.audio` | Already float buffers and a three-method interface. The audio-thread rules carry over unchanged. |
| `Menu()` / overlays | `rsmods.imgui` (optional) | Version-pinned to the host's ImGui; a mismatch costs only the windows. |
| `Draw()` (interceptors, texture lifecycle, device events) | None | `DrawContext` hands out `Mesh`/`ThiccMesh`, which are reverse-engineered game structs, plus `AfterDraw` restore closures. The D3D9 pointer itself is COM and would cross fine; the game-side data around it is what keeps changing. Stays in-tree; a later `rsmods.draw` can add it without touching anything above. |
| A mod's own game-function hooks | Not in the ABI | A plugin can install Detours itself, same as in-tree mods (README "Hooking game functions"). The loader can't see or stop that, which is the trust point in `plugin-distribution.md` #3. |

## Later, when a consumer asks

These fit the same `getInterface` scheme without touching anything above:

- **`rsmods.events`** — `subscribe(name, fn)` / `post(name, payload)` with a size-tagged payload, for
  notifications and plugin-to-plugin messages. New events then cost no ABI change. Lifecycle hooks stay typed.
- **`rsmods.song`** — host-computed song, tuning and arrangement info (the never-built `ISongInfo`
  capabilities layer). Plugins that use it survive game-side reverse-engineering changes; only the host
  absorbs them.
- **A plugin sidecar** listing the minimum interface majors it needs, so the loader can say "X needs `hud` v2"
  before loading it. Until then, `RSModsPluginMain` returning 0 after logging does the same job, just later.

## Problems the ABI doesn't solve on its own

1. **Loader and trust.** Which DLLs load, the verified badge and the unverified warning are all in
   `plugin-distribution.md`. The sketch assumes they ship together with these interfaces.
2. **Unloading.** Nothing here unloads a plugin. `FreeLibrary` while a hook, a pending `MainThreadInbox`
   item, an audio processor or an ImGui draw still points into it is a use-after-free. v1 loads once per game
   run, the same as in-tree mods.
3. **Freeze cost today.** Most recent churn (draw callbacks, before-reset, menu flags) sits in `Draw()`, which
   is out, and ImGui, which is now its own optional interface. The settings schema also kept growing
   (`heading`, `entryTitle` and `listedUnder` arrived on 2026-10-06), but as JSON that growth is free. So the
   per-interface split lets the stable parts freeze on their own clock: `mods`, `settings`, `commands`, `hud`
   and `audio` could reasonably be frozen well before the whole of `ModContext` stops moving.
