# Wwise plugin internals (Rocksmith 2014, Wwise v91 / 2013)

Reference for the drop pedal modules under `DLL/Mods/DropPedal*`. Records how
the pitch shifter is reached, what the surrounding data structures actually are,
and which routes have been ruled out. Every claim is tagged:

- **[proven]** — verified by disassembly or by a log line from a real run.
- **[inferred]** — consistent with evidence but not directly demonstrated.
- **[open]** — not yet established.

Addresses are absolute in the shipped x86 executable.

> **Revision note (v5).** Reverses §7.8. Redirecting the true tuning reference
> **does** move note detection and the tuner, for guitar and for emulated bass;
> the mechanism was correct all along and was masked by a missing `test al, al`
> in the hook. Also records that the reference latches once when the tuner is
> entered.
>
> **Revision note (v4).** Records that the pitch override is **additive** and why
> (§6.1), that redirecting the true tuning reference does **not** move note
> detection or the tuner (§7.8 — since disproven, see v5), and the two reusable
> pieces found in the MIDI auto-tuning mod (§11).
>
> **Revision note (v3).** Adds the construction chain (§2), the plugin id
> encoding (§1), the one-effect-per-node constraint, and the captured MultiPitch
> parameter block (§6). Answers the former headline open question: nothing
> *decides* to build a shifter — instantiation is entirely data-driven.
>
> **Revision note (v2).** An earlier version described `0xEF5750` and `0xEF5850`
> as an *FX chain* walker and inserter, and their 28-byte records as *effects*.
> That was wrong. They are the **RTPC binding** table. See §4.

---

## 1. The working path

**[proven]** `RegisterPlugin` is hooked. The Wwise pitch shifter registers as
plugin id `136`, company `0`. The hook detours the plugin's *create-param*
callback rather than the plugin callback.

**[proven]** On the first param object built, `SetParam` is patched at **vtable
slot 1**. Confirmed two independent ways:

1. By disassembly of the live vtable.
2. By declaration order in `DLL/Wwise/Types.hpp:477` —

```cpp
class IAkRTPCSubscriber
{
protected:
    virtual ~IAkRTPCSubscriber() {}   // slot 0
public:
    virtual AKRESULT SetParam(        // slot 1
        AkPluginParamID in_paramID,
        const void*     in_pParam,
        AkUInt32        in_uParamSize) = 0;
};

class IAkPluginParam : public IAkRTPCSubscriber
```

Calling convention is `__fastcall` with a dummy `edx`.

**[proven]** Param id **6** is pitch, in cents. Overriding it in `SpySetParam`
retunes the shifter whenever the active tone contains a MultiPitch pedal.

**[proven]** `PushPitchToLiveShifters` writes cents directly into tracked param
objects via the original `SetParam`, giving live mid-song pitch changes with no
chain rebuild.

### Plugin id encoding

**[proven]** `RegisterPlugin` reports company and plugin separately, but bank
data carries a single packed id:

```
id = (pluginId << 16) | (companyId << 4) | pluginType
```

`pluginType 3` is an effect. Every id observed in bank data ends in `0003` or
`1003` / `1033` (company `0`, `256`, `259` respectively). The pitch shifter,
plugin `136` from company `0`, is therefore **`0x880003`**.

Ids seen in a single boot, for orientation: `0x690003`, `0x6d0003`, `0x6e0003`,
`0x6e1003`, `0x730003`, `0x760003`, `0x7d0003`, `0x7e0003`, `0x7f0003`,
`0x810003`, `0x830003`, `0x870003`, `0x880003`, `0x41033`, `0x61033`, `0x91033`.

### Hard constraint

**[proven, learned by deadlock]** `SetParam` runs on the audio thread. It must
never log and never take a lock. It queues events into preallocated slots; the
draining and logging happen later from `DropPedal::Poll()` on the game thread.
Any probe added to this path must follow the same rule.

### The executable is packed

**[proven]** `.text` has `SizeOfRawData = 0` and `PointerToRawData = 0`; the
image also carries `.UBX0` / `.UBX1` protection sections. **Game code does not
exist in the file on disk.** Static disassembly of `Rocksmith2014.exe` yields
nothing. All disassembly must come from a dump of the running process — the mod
can write one from `InstallHooks`, before any detour is installed, since a
detour would otherwise appear in the dump as its own jump.

---

## 2. How a plugin instance comes to exist

**[proven]** The full chain, outermost first:

```
0xEDBAC1 / 0xEDBB91   node factory (0xEF5CE0 / 0xEF5D80), after a
                      0xEC4C00 lookup that skips creation if present
0xEDBADE / 0xEDBBAE   call 0xEF5B10   deserialize node
0xEF5B39              call 0xEF5410   instantiate effect
0xEF542C              call 0xEF12A0   plugin factory registry
0xEF12EF              call eax        create-param callback  <- our hook
```

Both call sites run inside the critical section at `0x1351814`
(`0x10B707C` enter, `0x10B7080` leave).

### `0xEF12A0` — the plugin factory registry **[proven]**

`__cdecl`, three args: allocator, packed plugin id, out-pointer.

Walks a global array — begin `[0x1352084]`, end `[0x1352088]`, **stride 12** —
comparing `[eax]` against the id. On a hit it reads the callback at record `+8`
and calls it with the allocator, storing the result through the out-pointer.
Returns `1` (`AK_Success`) or `2` (`AK_Fail`) via `neg/sbb/add 2`.

Record layout is `{+0 packed plugin id, +4 create-plugin, +8 create-param}` —
exactly what `RegisterPlugin` populates. A second registry with the same shape
lives at `[0x1352090]` / `[0x1352094]`.

This function is shared by every plugin type and **decides nothing**.

### `0xEF5410` — instantiate **[proven]**

`__thiscall`, `ret 0xC`. Calls the registry, then invokes the new param object's
**vtable slot 3** with `(allocator, blob, blobSize)` — an Init-from-parameter-
block — and **slot 4** (Term) if that fails. On success it calls `0xEF53C0`.

Note it returns `AK_Success` even when the registry lookup fails and nothing is
created; a missing plugin is silently non-fatal.

### `0xEF53C0` — attach **[proven]**

```
[node+0x10] = packed plugin id
[node+0x14] = param object      (Terms any existing one first, via slot 4)
```

**A node holds exactly one effect.** There is no list and no append: attaching
again evicts and destroys whatever was there. An effect chain is therefore *n*
nodes each holding one effect, not one node holding *n* effects.

### `0xEF5B10` — the node deserializer **[proven]**

`__thiscall` with two stack args (`ret 8` at `0xEF5BEF`, `mov edi, ecx` at
`0xEF5B22`). Prologue `55 8B EC 83 EC 0C 53 56`. Its first stack argument points
into a byte-packed serialized stream:

| Offset | Meaning |
|---|---|
| `+0x04` | packed plugin id, or `0xFFFFFFFF` for "no plugin" |
| `+0x08` | parameter blob size |
| `+0x0C` | parameter blob |

After the blob it reads a **byte** count, then that many **5-byte** `{byte,
dword}` entries into a freshly allocated `count * 8` array at
`[node+0x18] / [node+0x1c] / [node+0x20]`.

**This 8-byte-stride array is not the 28-byte RTPC binding table of §3.** They
are separate structures; do not conflate them.

`cmp eax, -1 / je` is the **only** gate on instantiation. There is no policy, no
condition, no runtime decision: the engine builds a pitch shifter because a
serialized record says `0x880003`.

---

## 3. The RTPC binding table

### `0xEF5750` — the 28-byte record walker **[proven]**

Disassembled from a byte dump of the live image:

```
EF5750: 55              push ebp
EF5751: 8b ec           mov  ebp, esp
EF5753: 56 57           push esi ; push edi
EF5755: 8b f9           mov  edi, ecx        ; this (__thiscall)
EF5757: 8b 77 24        mov  esi, [edi+0x24] ; array begin
EF575A: 3b 77 28        cmp  esi, [edi+0x28] ; array end
EF575D: 74 41           je   EF57A0          ; empty -> return
EF5763:                 ...marshal record...
EF5792: e8 89 4d fd ff  call ECA520
EF5797: 83 c6 1c        add  esi, 0x1c       ; += 28
EF579A: 3b 77 28        cmp  esi, [edi+0x28]
EF579D: 75 c4           jnz  EF5763
EF579F: 5b              pop  ebx
EF57A0: 5f 5e 5d c2 0c  ret  0xc
```

Owner `+0x24` = begin, `+0x28` = end, stride **28 bytes**. `0xEF5797` is the
loop's `add esi, 0x1c` — a *return address*, which is why it first appeared in a
stack snapshot.

### `0xECA520` — dispatch **[proven]**

```
ECA520: 55 8b ec 83 ec 1c       prologue, 0x1C locals
ECA529: b8 1f 00 00 00          mov eax, 0x1F        ; sentinel into a local
ECA53C: 85 ff / 0f 84 4a 02 00  test edi,edi ; jz +0x24A
ECA544: 8b 4d 14 / 33 d2        mov ecx,[ebp+0x14] ; xor edx,edx
ECA549: bb c1 00 00 00          mov ebx, 0xC1        ; 193
ECA54E: f7 f3                   div ebx
ECA555: 8b 94 96 14 06 00 00    mov edx,[esi+edx*4+0x614]
```

Hash-bucket lookup: `id % 193` indexes a table at `+0x614`, then the chain is
walked via `[edx+8]` with key compares at `[edx]` and `[edx+4]`.

### `0xEF5850` — the binding inserter **[proven]**

Signature derived from live bytes, not guessed: `mov ebx, ecx` at `0xEF585F`
puts `this` in `ECX`, and `ret 0x20` at the tail has the callee clean 32 bytes.
So **`__thiscall` with eight stack arguments**.

```
EF5871: 2b 4b 24              ecx = end - begin
EF5874: b8 93 24 49 92 / f7 e9 / 03 d1 / c1 fa 04   ; signed divide by 28
EF5887: 3b 7b 2c / 72 1b      capacity check against [ebx+0x2C]
EF588C: 6a 02 / 8d 4b 24 / e8 ...                   ; grow
EF58B9: 8b 43 28 / 8d 50 1c / 89 53 28              ; end += 28
EF58CF: 89 10                 [rec+0x00] = arg1
EF58D4: 89 50 0c              [rec+0x0C] = arg3
EF58DA: 89 78 08              [rec+0x08] = arg4
EF58DD: 89 50 04              [rec+0x04] = arg2
EF58F1: 8d 48 10 / e8 ...     helper 0xEAB850 fills +0x10/+0x14/+0x18
EF593B: c2 20 00              ret 0x20
```

`0xEF5940` frees `[esi+0x10]` **alone** and then nulls all three of
`+0x10/+0x14/+0x18`, so only `+0x10` is an owned allocation.

---

## 4. The 28-byte record is an RTPC binding

**[proven]** Hooking `0xEF5850` and dumping the bytes at `arg6` settles it. The
buffer parses cleanly as 12-byte groups of `{float, float, uint32}`, and the
group count always equals `arg7`:

```
a7=3   00000000 3bcd7140 01000000  -> (  0.0, 3.778, 1)
       00005c42 0aa28940 07000000  -> ( 55.0, 4.301, 7)
       0000c842 03d26240 04000000  -> (100.0, 3.544, 4)

a7=4   00000000 22987fbf 09000000  -> (0.0, -0.998, 9)
       cdcccc3d 22987fbf 05000000  -> (0.1, -0.998, 5)
       6666663f fffe7fbf 09000000  -> (0.9, -1.000, 9)
       0000803f fffe7fbf 04000000  -> (1.0, -1.000, 4)
```

The first float rises monotonically in every sample — `0→1` on normalised
curves, `0→55→100` on percentage ones — and the third word is a small
interpolation code. These are **Wwise RTPC curves**.

Record layout:

| Offset | Source | Meaning |
|---|---|---|
| `+0x00` | arg1 | RTPC / game parameter id |
| `+0x04` | arg2 | zero in every record captured (high half of `+0x00`) |
| `+0x08` | arg4 | target node id |
| `+0x0C` | arg3 | target parameter index |
| `+0x10` | from arg6 | owned copy of the curve points |
| `+0x14` | arg7 | curve point count |
| `+0x18` | arg5 | scaling mode |

This also explains a detail that never fitted: `+0x14` is restricted to 2–5
because that is how many points a curve has, not because it is a type tag.

---

## 5. Id stability

**[proven]** Ids are deterministic content hashes, stable within and across
sessions. Unrelated bindings log byte-identical every time they rebuild — a
10-record group reappeared as
`f650af73/3eee73d3, 22b36784/112279fc, e564c4d9/22e4ba60…` at four separate
timestamps in one session, and bus ids match across processes.

**[proven]** Editing the tone changes them wholesale. A pure duplicate of the
Emulated Bass tone produced the *identical* four ids; removing the MultiPitch
from that duplicate changed **every** id, in both `+0x00` and `+0x08`, with zero
overlap. Consistent with bindings regenerated for a rebuilt node graph.

**Consequence:** no id can be hard-coded to identify a node across tones. These
ids are diagnostic markers for reading logs, never something to build against.

### The Emulated Bass group **[proven]**

Appears only while that tone is loaded, and only ever with the pitch shifter:

| # | `+00` | `+08` | `+0c` | `+14` | `+18` |
|---|---|---|---|---|---|
| 0 | `62f8bb02` | `2de350c2` | `4` | `2` | `2` |
| 1 | `712defa7` | `0fa365d3` | `6` | `2` | `0` |
| 2 | `62f8bb02` | `0891e5af` | `3` | `2` | `2` |
| 3 | `bdb690a0` | `0dec9b6f` | `8` | `2` | `2` |

Byte-identical across two songs and three processes; only the `+0x10` pointer
moves.

### Gameplay cue owners — not tone data **[proven]**

The owner families `37c3cedf` and `5b659343` are the in-game **string highlight
cues** (Rocksmith flagging which string the next note is on). They appear mid-
song, unrelated to tone loading. Recorded here so they are not mistaken for
chain data in a future log read.

---

## 6. The MultiPitch parameter block

**[proven]** Captured from `0xEF5B10` on a boot with four shifters. The blob is
**38 bytes**, and there are exactly two distinct configurations, each appearing
twice.

Alignment is not uniform: a **2-byte field at `+0x14`** shifts everything after
it off a 4-byte boundary. Reading the tail as aligned dwords produces garbage.

| Offset | Type | config A | config B |
|---|---|---|---|
| `+0x00` | float | 0.0 | 0.0 |
| `+0x04` | float | −96.0 | −96.0 |
| `+0x08` | float | 0.0 | 0.0 |
| `+0x0C` | float | 50.0 | 50.0 |
| `+0x10` | float | 0.0 | 0.0 |
| `+0x14` | uint16 | 0 | 0 |
| `+0x16` | uint32 | 2 | 0 |
| `+0x1A` | uint32 | 0 | 0 |
| `+0x1E` | float | 18000.0 | 1000.0 |
| `+0x22` | float | 0.20 | 1.00 |

Raw bytes:

```
A  00 00 00 00 00 00 c0 c2 00 00 00 00 00 00 48 42 00 00 00 00
   00 00 02 00 00 00 00 00 00 00 00 a0 8c 46 cd cc 4c 3e
B  00 00 00 00 00 00 c0 c2 00 00 00 00 00 00 48 42 00 00 00 00
   00 00 00 00 00 00 00 00 00 00 00 00 7a 44 00 00 80 3f
```

**[inferred]** `−96.0` reads as a dB floor and `50.0` as a percentage; the
differing `18000`/`1000` and `0.2`/`1.0` look like a frequency and a mix or Q.
Field meanings are not established and are not needed for the work below.

**[proven] The pitch is not in the blob.** No `−1200`, and no cents value of any
kind, appears in either configuration. The authored parameter block only
configures the shifter; the pitch arrives afterwards through `SetParam` param 6.

**Consequence, and the most useful fact in this document:** tone-side work does
not have to encode a tuning. It only has to make a shifter **exist**. A
MultiPitch authored at 0 semitones is sufficient, because the mod's existing
param-6 override supplies the value.

### 6.1 The override is additive, not absolute **[proven]**

The mod applies `authoredCents + targetCents` rather than replacing the value.
This is required, not a preference:

- **[proven]** Rocksmith's emulated bass path does **not** pitch shift. Its
  default tone is a bass amp, a bass cabinet, and a `Pedal_BassEmulator`
  pre-pedal that shapes low-end body. There is no octave drop in it.
- **[proven]** A MultiPitch can only occupy the **pre-pedal slot**, of which
  there is exactly one. Adding one to a bass tone therefore **deletes**
  `Pedal_BassEmulator`.
- **[proven, by ear]** With the emulator gone, a bass tone at 0 semitones sounds
  like a cleanish guitar. It needs **-1200** to sit in bass register.

So a bass drop-pedal tone must carry -12 in its own `Pitch1`, and an absolute
override would erase that on every tone load. Additive lets the baseline live in
the tone where it belongs, and keeps one uniform rule for both instruments:
*the tone's authored pitch is the baseline, the pedal moves from there*.

Two consequences worth knowing:

- A tone with a stale non-zero `Pitch1` silently offsets everything. This is a
  real trap: a guitar tone copied from an octave-down one plays an octave down at
  0 semitones. The tell is that shifting **up** by the tone's own amount sounds
  correct.
- The baseline must be re-read on **every** delivery, not cached on first sight.
  Param objects are reused across tone loads, so a baseline captured once goes
  stale and leaks the previous tone's pitch into the next one.

The readout is unaffected: it reports only the player's own shift, so an
emulated-bass tone at -1 still displays `Eb standard (-1)` rather than -13.


---

## 7. Ruled out — do not retry

### 7.1 `SetBusEffect` **[proven dead]**

Never instantiates an effect in this build.

- Export addresses correct: `0xEC2690` (by id), `0xEC4320` (by name).
- Disassembly shows it queues message type `0x27` and returns `AK_Success`
  **unconditionally**.
- Confirmed empirically: probes with a **bad bus name**, a **bad bus id**, and a
  **bad shareset** all returned `1`. **Return values carry no information.**
- Shareset `0x19D9B890` (plugin 136) genuinely exists in the live `init.bnk`
  (byte-identical to the `CustomMods` copy, MD5 `86dd4bb…`). Bus ids are valid.
- Attach attempted at earliest boot tick, at song start, on every plausible bus.

### 7.2 Injecting a binding with `+00 = 0x19D9B890` **[proven dead]**

Shareset `0x19D9B890` and plugin id `136` appear in **no** captured record, in
any field, in any run. Ids are post-registration runtime hashes (§3, §5).

### 7.3 RTPC **[proven dead for this purpose]**

An RTPC can only push a value into an instance that already exists, and stock
tones have no shifter instance. Same wall.

### 7.4 Adding a second effect to an existing node **[proven dead]**

`0xEF53C0` writes a single plugin id and a single param object into
`[node+0x10]` / `[node+0x14]`, terminating any previous one. Calling `0xEF5410`
again on a node replaces its effect rather than adding one.

### 7.5 Claiming an empty node slot **[proven dead]**

The `-1` sentinel at `0xEF5B34` looked like a free slot to claim. A filtered
run accounted for **1493 nodes with zero drops and not one `0xFFFFFFFF`**. No
tone chain contains an unused effect slot.

### 7.6 Static disassembly of the shipped executable **[proven dead]**

The file is packed; `.text` has no bytes on disk. See §1.

### 7.7 Bank / cache facts **[proven]**
No loose `init.bnk`. It lives in `cache.psarc` → `cache3.7z` →
`audio/windows/init.bnk`. The GUI unpacks to
`RSMods\Temp\cache_psarc_RS2014_Pc`.

Bus ids from the HIRC parse: Master `0xE2B7BC37`, Game `0x29DF07D7`,
Guitars `0x29E9905A`, plus named UI / VO / Tuner / Click / Ambience / Pedals /
Amps.

`Constants.cs` declares `WwiseInitBnk_*` paths, currently unreferenced.

### 7.8 Redirecting the true tuning reference **[proven]**

The game's true tuning calculation loads a reference frequency at `0x004DCCF2`
(`fld [0x01224468]`), which the Disable True Tuning mod replaces. Scaling that
value by `2^(V/12)` makes note detection expect a signal V semitones from the
song's written tuning, and the in-game tuner follows it.

This is the same mechanism CDLC charters use when they set an arrangement's
tuning pitch to 311.2 Hz to move every expected note down six semitones
(440 / 2^(6/12) = 311.13).

Note the sign: to shift audio *down* one semitone, expectation moves *up* one,
so a -1 pedal logs `466.164 Hz for 1 semitone(s)`.

**Why this read as dead for two sessions.** The hook branched on the result of
`CanDisableTrueTuning()` with no `test al, al` between the `call` and the `je`.
The callee returns a bool in AL via `mov al, [mem]`, which sets no flags, so the
branch tested whatever the caller happened to leave behind and the override was
applied arbitrarily. The reference was being computed and logged correctly the
whole time — it simply never reached `ST(0)`. The earlier conclusion here was
drawn from runs where the branch never took the path that does the `fld`.

**The reference latches once, on entering the tuner.** Changing the pedal
mid-song moves the audio only; detection keeps scoring against the value latched
at entry, so the notes that score are still the ones under your fingers. Backing
out to the menu and re-entering the tuner re-latches it.

**Emulated bass works too. [proven]** The `cmp dword ptr [ebp+8], -1200` branch
routes CentOffset == -1200 to the game's own true tuning path, bypassing the
redirect. In practice that value is not seen in normal play: bass arrangements
reach the `fld` branch and get the redirect exactly like guitar ones, confirmed
by a bass drop tuning scoring correctly in-game.

An earlier session concluded that bass detection was broken *by* this carve-out.
That was wrong. Bass was failing for the same reason guitar was — the menu gate
(since removed) closed during `SelectionListDialog` at song load, discarding the
reference at the moment detection latched it. **[open]** What actually produces
CentOffset == -1200, and whether the carve-out is still needed at all.


---

## 8. Corrections to earlier claims

Recorded so they are not re-litigated.

- **Redirecting the true tuning reference is a dead end (§7.8).** Wrong, and it
  cost two sessions. The mechanism was right; the hook's branch was reading stale
  flags because the `call` was not followed by `test al, al`. Marking a mechanism
  dead requires a hook whose control flow has been verified in the disassembly,
  not just a log line showing the value was set.
- **Emulated bass is excluded from the redirect by the −1200 carve-out.** Wrong.
  Bass detection was failing for the same reason guitar detection was — the menu
  gate closing at song load — and works once that is removed. The carve-out
  exists but is not reached in normal play.
- **The menu crashes came from `CanDisableTrueTuning()` reading
  `GameState::currentMenu`.** Wrong. That was a genuine data race and worth
  fixing, but neither crash came from it. They were `SongTimer` dereferencing an
  address `MemUtil::FindDMAAddy` returned without validating, and
  `EnumerationThread` rebuilding `Settings::modSettings` at `GameLoaded` while
  other threads read it. Both are upstream RSMods bugs, unrelated to the pedal.

- **`0x880003` in a stack snapshot is a code address.** Wrong — it is the packed
  plugin id of the pitch shifter (§1), passed down the construction chain. It sat
  unexplained in every `shifter built` line for two sessions.
- **`0xEF5750`/`0xEF5850` are the FX chain.** Wrong — RTPC binding table (§4).
- **`+0x10/+0x14/+0x18` is a begin/end/capacity vector.** Wrong — `0xEF5940`
  frees `+0x10` alone; the other two are scalars.
- **`arg6`/`arg7` are begin/end pointers.** Wrong — `arg6` is a pointer, `arg7`
  is a point count.
- **The bass group appears immediately *after* the −1200 SetParam.** Wrong. It
  is captured *before* any param-6 delivery (`pitchCalls=0`).
- **4 records ↔ 4 param-6 calls, so 1:1.** Coincidence.
- **`+0x14` is a type tag, so filter on it.** It is a curve point count.
- **The four boot shifters are a menu chain, inactive during songs.** Wrong —
  they are the only shifter instances that ever exist, and they are exactly what
  a MultiPitch tone drives. They are built during profile/tone load before the
  song, and a tone switch re-parameterises them rather than building more.

---

## 9. Open questions

- **[answered] What decides that a pitch shifter is built?** Nothing does.
  Instantiation is entirely data-driven: `0xEF5B10` reads a packed plugin id out
  of serialized tone data and builds whatever it names (§2). There is no runtime
  decision point to reach or gate to satisfy.

- **[open] How does a tone bind to an existing shifter?** All four shifters are
  built before the song. Selecting a MultiPitch tone mid-song delivers `−1200`
  without building anything, so the remaining mechanism is routing, not
  instantiation.

- **[resolved] How does note detection derive its expected pitch?** From the true
  tuning reference at `0x004DCCF2` (§7.8). Redirecting it moves detection and the
  tuner together. This was the last blocker for V1 and it is closed — audio and
  scoring now agree.

- **[open] Song audio for the tuner feature.** `SetBusEffect` is dead, so the
  game's own time-stretch / true-tuning machinery
  (`ptr_timeStretchCalculations`, `ptr_disableTrueTuning` in `Offsets.hpp`) is
  the remaining unexplored lead.

---

## 11. Reusable pieces for later work

The MIDI auto-tuning mod drives a **physical** Digitech pedal, which sits between
guitar and game and shifts the *input*. Those users never have this mod's
detection problem, so there is no detection code to borrow. Two other pieces are
directly reusable.

### 11.1 A bass indicator **[proven, in shipping code]**

`Midi::AutomateTuning` at `Midi.cpp:366`:

```cpp
int TrueTuning_Hertz = SongTuning::GetTrueTuning();
if (TrueTuning_Hertz < 260)   // Give some leeway for A220 and its true tuned offsets
    highestTuning -= 12;
```

Bass arrangements report a true tuning near **A220**, an octave below A440, and
the mod compensates by an octave. So `GetTrueTuning() < 260` distinguishes bass
from guitar without any new reverse engineering.

Note this independently corroborates §6.1: the game already models bass as an
octave down, which is why a bass drop-pedal tone needs -1200.

This is what a two-slot auto-select would need — a guitar slot and a bass slot,
chosen by this check — since tone slots are shared between the two instruments
and reassigning them in the Tone Designer is slow.

### 11.2 Automatic tuning selection **[available, unused]**

`Midi::AutomateTuning` already does everything needed to set the shift from the
song rather than by hand:

- `SongTuning::GetHighestLowestString()` — what the song is tuned to, `666/666`
  on a bad pointer read.
- `SongTuning::GetTrueTuning()` — the song's reference in Hertz.
- `tuningOffset` — the player's physical guitar reference, adjustable via
  `Keybindings::HandleTuningOffset`.
- `alreadyAutomatedTuningInThisSong`, plus a menu-side reset, for once-per-song
  behaviour.

Wiring the drop pedal in as another pedal type with its own `autoTuneFunction`
would inherit the scaffolding rather than duplicate it.

The digital pedal has one advantage over the hardware path here. The hardware
handles off-concert-pitch songs with `trueTuningMap`, a lookup of the discrete
values a physical pedal supports, and logs an error when the song's value has no
entry. Param 6 is in **cents**, so the offset can be applied exactly and
continuously:

```
shift in cents = (song tuning - player tuning) x 100 + 1200 x log2(reference / 440)
```

A song at A435 is -19.8 cents, applied directly, no table.


---

## 10. Probe and testing discipline

- **Hotkeys are configurable.** Defaults are `,` / `.` = pitch down/up,
  `F8` = toggle, and `F9` / `F10` = base tuning down/up. **Never** bind probes
  to these defaults or nearby keys.
- **Two-tone test.** Enter song → press `2` (has MultiPitch), wait ~10s →
  press `4` (no MultiPitch), wait ~10s → quit. The `-1200` line timestamps the
  first switch precisely, and the 10-second gap makes the second unambiguous.
  This works; use it.
- **Verify against `applied` / `pushed` / `pitchCalls` log lines, not by ear.**
  Expectation has contaminated multiple runs.
- The log at `<game>\RSMods_debug.txt` is **locked while the game runs** and is
  **overwritten on every launch** — it does not append. Quit before reading, and
  do not relaunch before the log has been read.
- **The log does not record song names.** The only per-song marker is the
  `QueryAudioObjectIDs` error naming the play event (e.g. `Play_ThinBoys`,
  `Play_NOFX180Degrees`).
- Probes must log **nothing** inside `SetParam`, `SpyCreateParam`, or any walker
  or deserializer hook: capture into preallocated slots, drain from `Poll()`.
- **Filter at the capture site, not by enlarging buffers.** An unfiltered hook on
  `0xEF5B10` produced ~1440 nodes in one boot and overflowed a 128-slot buffer,
  which silently turned an absence-of-evidence question into an unanswerable one.
  Log the interesting cases and count the rest.
- **Before hooking any new function, dump its bytes and derive the calling
  convention** (`ret N` = callee-cleans, bare `ret` = `__cdecl`). Guessing on a
  hot path crashes the game rather than producing a bad log line. Guard every
  hook with a prologue check.
