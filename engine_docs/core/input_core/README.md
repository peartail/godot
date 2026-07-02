# Input Core

Input singleton state, input events, action maps, shortcuts, controller mappings, and event codec support.

## Subtopics

- [Input State](input_state.md): input singleton state, device state, sensors, vibration, and event dispatch.
- [Input Events](input_events.md): keyboard, mouse, joypad, touch, gesture, MIDI, and action events.
- [Input Map](input_map.md): action definitions, event matching, deadzones, and built-in actions.
- [Shortcuts And Controller Mappings](shortcuts_and_controller_mappings.md): shortcuts, controller DB generation, and event codecs.

## Global Entry Points

- `Input` stores runtime input state.
- `InputEvent` subclasses describe incoming user input.
- `InputMap` maps events to named actions.

## Code Links

- `core/input/input.*`
- `core/input/input_event.*`
- `core/input/input_map.*`
- `core/input/shortcut.*`