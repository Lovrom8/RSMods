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

- **The tap** (one mod that hooks the audio driver, e.g. through ASIO): feeds the chain.

  ```cpp
  // createBuffers, while bufferSwitch can't run:
  const uint32_t latency = Framework::AudioInput().PrepareTap({ sampleRate, channels, maxFrames });
  // bufferSwitch, audio thread, after converting the input to interleaved float:
  Framework::AudioInput().ProcessTap(samples, frames);
  // disposeBuffers:
  Framework::AudioInput().ReleaseTap();
  ```

The framework doesn't hook any audio driver itself. Which driver, how it gets loaded before RS_ASIO, and
sample-format conversion all stay in the mod that provides the tap. That keeps RSMods out of RS_ASIO
support. A tap mod should claim `"audio-input-tap"` so two taps never run together.

## Rules

- **The chain is fixed at the first `PrepareTap`.** After that, the audio thread walks a fixed array with no
  locks or refcounts, reading one atomic flag per processor. A processor added later is logged and ignored.
  Add processors from `OnInitialize`, which runs long before the driver creates its buffers.
- **Latency is fixed too.** Rocksmith calibrates input latency once. `GetLatencyFrames()` must return the
  same value whether the processor is active or not, and `PrepareTap` returns the total for the tap to
  report.
- **Processors never leave.** When the owner isn't `Active` (disabled, suppressed, faulted, shut down),
  `Process` still runs with `active = false`: pass the audio through with the same delay. The framework
  sets the flag from the registry's state, so mods don't track it.
- **`Process` is `noexcept`** and runs on the audio thread: no allocating, locking, logging or blocking.
  Nothing can catch a throw there, so `noexcept` makes it terminate instead of unwinding into the driver.
- **`Prepare` runs off the audio thread** (on the tap's thread), on the first `PrepareTap` and again on
  each later one (a driver reset or format change). Allocate there. A `Prepare` that throws is logged and
  that processor sits out until the next `PrepareTap`; it adds no latency.
- **Order:** lower `order` runs first, ties go by owner Id.
- `c.Audio().InputTapLive()` tells a mod whether the tap is running, e.g. to fall back to another engine.

## Threads

| Call | Thread |
|---|---|
| `AddInputProcessor`, registry state changes | MainThread |
| `PrepareTap`, `ReleaseTap`, `Prepare` | the tap's (driver) thread, while `ProcessTap` can't run |
| `ProcessTap`, `Process` | audio thread |

The tap must not call `PrepareTap` while `ProcessTap` might be running; ASIO guarantees that between
`disposeBuffers` and `createBuffers`.

## Tests

`Tests/AudioInputTests.cpp`: order, the fixed chain, latency totals, inactive pass-through, removal
before and after the chain is fixed, a failed `Prepare`, release and re-prepare.
