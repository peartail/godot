# Config And Engine

Process-wide engine state, project settings, global definitions, feature overrides, autoloads, and singleton registration.

## Subtopics

- [Engine Singleton](engine_singleton.md): runtime metadata, singleton registry, timing settings, and process-wide flags.
- [Project Settings](project_settings.md): project discovery, loading/saving settings, path localization, and settings metadata.
- [Global Settings And Autoloads](global_settings_and_autoloads.md): global definitions, feature overrides, autoloads, groups, and caches.

## Global Entry Points

- `Engine` owns runtime-wide flags and singleton metadata.
- `ProjectSettings` stores project configuration and built-in setting definitions.
- `_GLOBAL_DEF()` and related macros define engine settings.

## Code Links

- `core/config/engine.*`
- `core/config/project_settings.*`
- `core/core_globals.h`