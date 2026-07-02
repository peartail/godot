# Global Settings And Autoloads

## Scope

Global setting definitions, override lookup, feature tags, autoload metadata, global class cache, and scene group cache.

## Entry Points

- `_GLOBAL_DEF()`
- `GLOBAL_GET()`
- `ProjectSettings::add_autoload()`
- `ProjectSettings::refresh_global_class_list()`
- `ProjectSettings::has_custom_feature()`

## Flow Notes

- Global settings are registered by engine systems during initialization.
- Feature-specific overrides let projects specialize setting values per export/runtime feature.
- Autoloads and global class lists are project metadata consumed by runtime and editor systems.

## Code Links

- `core/config/project_settings.*`
- `core/core_globals.h`
- `main/main.cpp`