# Input Map

## Scope

Action registration, event-to-action matching, deadzones, built-in input actions, and project setting integration.

## Entry Points

- `InputMap`
- `InputMap::add_action()`
- `InputMap::action_add_event()`
- `InputMap::event_is_action()`
- `InputMap::load_from_project_settings()`

## Flow Notes

- `InputMap` is initialized during startup and populated from project settings.
- Built-in actions are added by project settings defaults.
- Event matching must account for exact-match and non-exact action queries.

## Code Links

- `core/input/input_map.h`
- `core/input/input_map.cpp`
- `core/input/input_map.compat.inc`
- `core/config/project_settings.*`