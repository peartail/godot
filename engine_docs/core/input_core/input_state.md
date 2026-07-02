# Input State

## Scope

Runtime input singleton state, pressed keys/buttons, mouse state, sensors, vibration, cursor behavior, and event dispatch.

## Entry Points

- `Input`
- `Input::parse_input_event()`
- `Input::is_action_pressed()`
- `Input::set_mouse_mode()`
- `Input::start_joy_vibration()`

## Flow Notes

- Platform/display code feeds events into `Input`.
- Action state is resolved through both raw event state and `InputMap` matching.
- Device state must stay consistent across focus changes and synthetic events.

## Code Links

- `core/input/input.h`
- `core/input/input.cpp`
- `core/input/input.compat.inc`
- `servers/display_server.*`