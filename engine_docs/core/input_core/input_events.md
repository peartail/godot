# Input Events

## Scope

Input event class hierarchy, event metadata, event matching, accumulation, and device-specific event data.

## Entry Points

- `InputEvent`
- `InputEventKey`
- `InputEventMouseButton`
- `InputEventMouseMotion`
- `InputEventJoypadButton`
- `InputEventJoypadMotion`
- `InputEventAction`

## Flow Notes

- Input events are Objects and are exposed to scripts.
- Event matching is used by `InputMap`, shortcuts, and editor input handling.
- Device IDs and modifier state are part of event identity for many comparisons.

## Code Links

- `core/input/input_event.h`
- `core/input/input_event.cpp`
- `core/input/input_enums.h`
- `core/os/keyboard.*`