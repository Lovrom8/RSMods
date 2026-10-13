# Audio input chain (`AudioInput`)

Lets mods edit the guitar signal before the game's pitch detection sees it, without each one hooking the
audio driver. Built for DropPedal and similar audio mods (`droppedal-port.md`, "ASIO input tap").

## Two roles

- **Processors** (any mod): implement `Framework::IInputProcessor` and add it from `OnInitialize`:

  ```cpp
  void MyMod::OnInitialize(ModContext& c) {
      c.Audio().AddInputProcessor(shifter, /*order*/ 0);   // shifter is a member
  }
  ```

- **The tap** (one mod that hooks the audio input): feeds the chain, one call set per capture stream.

  ```cpp
  // Stream `input` opened or changed format (any thread but that stream's audio thread mid-ProcessTap):
  const uint32_t latency = Framework::AudioInput().PrepareTap(input, { sampleRate, channels, maxFrames });
  // That stream's audio thread, after converting to interleaved float:
  Framework::AudioInput().ProcessTap(input, samples, frames);
  // Stream gone:
  Framework::AudioInput().ReleaseTap(input);
  ```

The framework doesn't hook any audio driver itself. Where the tap sits (ASIO `createBuffers`, the game's
capture stream `GetBuffer`), how it gets loaded before RS_ASIO, and sample-format conversion all stay in
the mod that provides the tap. That keeps RSMods out of RS_ASIO support. A tap mod should claim
`"audio-input-tap"` so two taps never run together.

## Inputs

Up to `kMaxAudioInputs` (2) capture streams, each with its own format, latency, audio thread and live
state. The tap picks the index; for Rocksmith's capture streams it's the player (0 = player 1, 1 = player 2
in multiplayer, each mono). Processors register once and get the index on every call, so a processor that
keeps state (a pitch shifter, a delay line) keeps one per input. Player 1 and player 2 can be in different
tunings, so state isn't shared between them.

`Prepare` for one input can run while `Process` for another is running on its own thread: never touch
another input's state from `Prepare`.

## Rules

- **The chain is fixed at the first `PrepareTap` on any input.** After that, the audio threads walk a fixed
  array with no locks or refcounts, reading one atomic flag per processor. A processor added later is
  logged and ignored. Add processors from `OnInitialize`, which runs long before any stream opens.
- **Latency is fixed per input.** Rocksmith calibrates input latency once. `GetLatencyFrames(input)` must
  return the same value whether the processor is active or not, and `PrepareTap` returns that input's total
  for the tap to report. Inputs can differ (different sample rates).
- **Processors never leave.** When the owner isn't `Active` (disabled, suppressed, faulted, shut down),
  `Process` still runs with `active = false`: pass the audio through with the same delay. The framework
  sets the flag from the registry's state, so mods don't track it.
- **`Process` is `noexcept`** and runs on the audio thread: no allocating, locking, logging or blocking.
  Nothing can catch a throw there, so `noexcept` makes it terminate instead of unwinding into the driver.
- **`Prepare` runs off that input's audio thread**, on its first `PrepareTap` and again on each later one
  (a format change or reopened stream). Allocate there. A `Prepare` that throws is logged and that
  processor sits out of that input until its next `PrepareTap`; it adds no latency there.
- **Order:** lower `order` runs first, ties go by owner Id. Same order on every input.
- `c.Audio().InputTapLive()` tells a mod whether the tap is running on any input, `InputTapLive(input)` on
  one, e.g. to fall back to another engine.

## Threads

| Call | Thread |
|---|---|
| `AddInputProcessor`, registry state changes | MainThread |
| `PrepareTap(i)`, `ReleaseTap(i)`, `Prepare(i)` | any thread, except from inside `ProcessTap(i)` |
| `ProcessTap(i)`, `Process(i)` | input `i`'s audio thread |

`PrepareTap` and `ReleaseTap` don't need a quiet window from the driver. They stop new `ProcessTap` calls
on that input, wait for one in flight to return (at most one block's processing), then prepare. So a tap
on a stream with no dispose/create pair, like `GetBuffer`, can prepare from MainThread when it sees the
format change. Calling either from inside `ProcessTap` for the same input would wait on itself; from the
audio thread between blocks is fine. Other inputs keep running throughout.

## Tests

`Tests/AudioInputTests.cpp`: order, the fixed chain, latency totals, inactive pass-through, removal
before and after the chain is fixed, a failed `Prepare`, release and re-prepare, independent inputs,
out-of-range indices, and a re-prepare waiting for its own input's in-flight block but not another's.
