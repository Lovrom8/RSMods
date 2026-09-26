# Input capture

> **Status: built and active.** Pure filters in `InputCapture.hpp`, unit-tested in
> `Tests/InputCaptureTests.cpp`; dinput8 glue in `DLL/DirectInputCapture.cpp`. Ported from the
> RSModsPlus fork's `OverlayInputCapture`.

## The problem

Rocksmith reads the mouse only through DirectInput 8: no raw input, no `GetCursorPos`, no WM_ mouse
messages. So when the ImGui menu is open, a click or wheel scroll on a panel also clicks or scrolls the
game behind it. Swallowing messages in `WndProc` can't stop that.

## How it works

- **Framework side** (`Framework::Input()`): one atomic "mouse captured" flag and two filters. The
  flag is on while the menu is open **and** ImGui wants the mouse (`io.WantCaptureMouse`), so gameplay
  input stays live whenever the cursor is outside a panel.
  - `FilterDeviceState` blanks a `DIMOUSESTATE`/`DIMOUSESTATE2` (axes, wheel, buttons).
  - `FilterDeviceData` drops every buffered event, so a press and its release never split between the
    overlay and the game.
- **Host side** (`DirectInputCapture`): the game creates its DirectInput interface before we load, so
  hooking `DirectInput8Create` never fires. Instead, `Install()` creates a throwaway mouse device and
  patches `GetDeviceState` (slot 9) and `GetDeviceData` (slot 10) in dinput8's device vtables (one
  each for A and W). Every device shares them, the game's too. The keyboard and joysticks share those
  vtables as well, so each hook asks the device for its type (`GetCapabilities`, cached per device)
  and only filters mice.

`Install()` runs once from `Menu::Init`, on the render thread, not in `DllMain`, because creating a DirectInput
object under the loader lock can deadlock. The vtable bookkeeping (originals per vtable and slot, lookup from
inside the hook, restore) is `MemUtil::VTablePatcher`. `EndScene` sets the flag after rendering the ImGui frame, and
`DLL_PROCESS_DETACH` restores the original vtable slots.

**Keyboard** goes through `WndProc` instead. While the menu is open and an ImGui widget is active
(a text field being typed into, a slider being dragged), key messages stop there, so they can't also
fire mod hotkeys. The test is `ImGui::IsAnyItemActive()`, not `io.WantCaptureKeyboard`: keyboard nav
is on, which makes the latter true whenever the menu has focus, and that would swallow the Backspace
that closes the menu.

Mods don't call this. Anything drawn through `MenuRegistry` is inside the menu, so the host's flag
already covers it.
