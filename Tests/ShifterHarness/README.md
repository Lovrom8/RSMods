# Shifter Harness

Standalone console harness for `DelayLinePitchShifter`. Feeds synthetic guitar-like
signals through the shifter and reports, per case:

- pop/click events: single-sample discontinuities far above the local envelope,
  with input-attack events subtracted so only shifter artifacts are counted
- pitch accuracy: dominant output frequency vs expected shifted frequency, in cents
- harmonic purity: output energy at expected shifted harmonics vs off-harmonic
  probe bins, in dB (higher is cleaner)

Cases cover held notes with vibrato across all six strings, Karplus-Strong plucks,
bends, double stops (fifth/fourth/third), staccato palm-mute repeats, mid-note
retunes (engage/disengage/slam), light playing over a noise floor, silence, and
pure noise.

## Build and run

From a Developer Command Prompt in Tests\ShifterHarness:

    build.bat
    harness.exe
    harness.exe live_input_HHMMSS.wav

Or with any C++17 compiler:

    g++ -O2 -std=c++17 -I ../../DLL -I ../../DLL/Audio -o harness harness.cpp ../../DLL/Audio/DelayLinePitchShifter.cpp

## Reading results

Running `harness.exe` with no arguments runs the synthetic battery. Running it
with a WAV path runs the recorded-take regression at the default -2 semitone
shift and writes `shifted_<input>.wav`. Use `harness.exe input.wav -4 out.wav`
when testing a different shift amount.

A healthy run shows output pop counts at or below input pop counts (inputs contain
real attack transients that the detector legitimately flags), pitch within a few
cents, and purity above roughly +40dB on held notes. Any `POP at ...` line is a
shifter artifact: it lists time, magnitude, local envelope, and the ratio between
them. Use these before and after any shifter change; the splice-commit glitch,
the integer-jump buzz, and the chord/staccato pops were all found and verified
fixed with this harness.
