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
  // Stream `input` opened or changed format:
  const uint32_t latency = Framework::AudioInput().PrepareTap(input, { sampleRate, channels, maxFrames });
  // That stream's audio thread, with the buffer as planar float:
  if (Framework::AudioInput().ProcessTap(input, channelPointers, frames))
      ConvertBack(...);   // otherwise the captured buffer is untouched
  // Stream closed:
  Framework::AudioInput().ReleaseTap(input);
  ```

The framework doesn't hook any audio driver itself. Where the tap sits (ASIO `createBuffers`, the game's
capture stream `GetBuffer`), how it gets loaded before RS_ASIO, and sample-format conversion all stay in
the mod that provides the tap. That keeps RSMods out of RS_ASIO support. A tap mod should claim
`"audio-input-tap"` so two taps never run together.

## Buffers

Planar float in [-1, 1], up to `kMaxInputChannels` (8) channels, as ASIO delivers them. A buffer longer
than `maxFrames` is split before it reaches the processors.

## Streams

Up to `kMaxAudioInputs` (2) capture streams, each with its own format, latency, audio thread and live
state. The tap picks the index; for Rocksmith's capture streams it's the player (0 = player 1, 1 = player 2
in multiplayer, each mono). Processors register once and get the index on every call, so a processor that
keeps state (a pitch shifter, a delay line) keeps one per input. Player 1 and player 2 can be in different
tunings, so state isn't shared between them.

`Prepare` for one input can run while `Process` for another is running on its own thread: never touch
another input's state from `Prepare`.

The first `PrepareTap` opens a stream and fixes its latency, which Rocksmith calibrates against. Later
`PrepareTap` calls (a format change) keep it; `ReleaseTap` closes the stream. For a tap, a driver reset or
a reconnected device is close then open; a buffer size change on a running stream is `PrepareTap` alone.

## Rules

- **The chain is fixed at the first `PrepareTap` on any input.** After that, the audio threads walk a fixed
  array with no locks or refcounts. A processor added later is logged and ignored. Add processors from
  `OnInitialize`, which runs long before any stream opens.
- **Latency is fixed while a stream is open.** `GetLatencyFrames(input)` must return the same value whether
  the processor is active or not. A processor that fails `Prepare` or changes its latency on an open stream
  is bypassed by a delay of its original latency. One that fails when the stream opens adds nothing.
- **Processors never leave.** When the owner isn't `Active` (disabled, suppressed, faulted, shut down),
  `Process` still runs with `active = false`: pass the audio through with the same delay. The framework
  sets the flag from the registry's state, so mods don't track it.
- **`Process` is `noexcept`** and runs on the audio thread: no allocating, locking, logging or blocking.
  It returns whether it changed the buffer.
- **`Prepare` runs off that input's audio thread**, on each `PrepareTap`. Allocate there.
- **Order:** lower `order` runs first, ties go by owner Id. Same order on every input.
- `c.Audio().InputTapLive()` tells a mod whether the tap is running on any input, `InputTapLive(input)` on
  one, e.g. to fall back to another engine.

## Aborted processors

A processor whose output has NaN or infinity (zeroed on the spot), or that overloads the audio thread (more
than half the buffer's duration, three buffers in a row or 8 of the last 64), is aborted on that input and
bypassed by a delay of its latency. MainThread faults its owner from `TakeFaults`, like a render-thread
fault. It runs again from its input's next `PrepareTap`. A processor that hangs or crashes still takes the
audio thread with it.

## Threads

| Call | Thread |
|---|---|
| `AddInputProcessor`, `PublishActive`, `TakeFaults`, registry state changes | MainThread |
| `PrepareTap(i)`, `ReleaseTap(i)`, `Prepare(i)` | any thread, except from inside `ProcessTap(i)` |
| `ProcessTap(i)`, `Process(i)` | input `i`'s audio thread |

`PrepareTap` and `ReleaseTap` stop new `ProcessTap` calls on that input and wait for one in flight to
return, so they need no quiet window from the driver. Each input has its own lock for this; MainThread's
calls never wait on audio.

## Tests

`Tests/AudioInputTests.cpp`: order, the fixed chain, latency totals, inactive pass-through, removal
before and after the chain is fixed, a failed `Prepare`, release and re-prepare, independent inputs,
out-of-range indices, waiting for an in-flight buffer, the latency hold, aborts, split buffers, refused
formats and the change flag. `Tests/StateMachineTests.cpp` checks that an abort faults the mod.
