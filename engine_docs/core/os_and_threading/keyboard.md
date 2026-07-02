# Keyboard

## Scope

Key enum definitions, key modifiers, key locations, and key string conversion helpers.

## Entry Points

- `Key`
- `KeyModifierMask`
- `KeyLocation`
- `keycode_get_string()`
- `keycode_has_unicode()`

## Flow Notes

- Key enums are shared by input events, editor shortcuts, and platform input translation.
- String conversion is used by UI display and serialization-facing code.
- Platform key mapping code must align with these core enum values.

## Code Links

- `core/os/keyboard.h`
- `core/os/keyboard.cpp`
- `core/input/input_event.*`