# Project Settings

## Scope

Project configuration loading, saving, path localization, property metadata, custom features, and editor overrides.

## Entry Points

- `ProjectSettings`
- `ProjectSettings::setup()`
- `ProjectSettings::load_custom()`
- `ProjectSettings::save()`
- `ProjectSettings::get_setting()`
- `ProjectSettings::set_setting()`

## Flow Notes

- Project settings are loaded early by `Main::setup()`.
- Settings can have custom property info, restart flags, basic/internal flags, and default values.
- Path localization/globalization is central to resource and project file handling.

## Code Links

- `core/config/project_settings.h`
- `core/config/project_settings.cpp`
- `main/main.cpp`