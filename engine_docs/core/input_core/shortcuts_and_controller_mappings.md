# Shortcuts And Controller Mappings

## Scope

Shortcut resources, controller database data, generated controller mappings, and input event encoding.

## Entry Points

- `Shortcut`
- `InputEventCodec`
- `default_controller_mappings`
- `input_builders.py`

## Flow Notes

- Shortcuts wrap input events for editor and UI command handling.
- Controller mappings are generated from bundled controller DB files.
- Input event codecs serialize/deserialize events for tooling and remote/debug paths.

## Code Links

- `core/input/shortcut.*`
- `core/input/input_event_codec.*`
- `core/input/default_controller_mappings.*`
- `core/input/gamecontrollerdb.txt`
- `core/input/godotcontrollerdb.txt`
- `core/input/input_builders.py`