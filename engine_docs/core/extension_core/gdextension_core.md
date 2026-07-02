# GDExtension Core

## Scope

Extension resource behavior, initialization levels, entry points, lifecycle, and extension resource format support.

## Entry Points

- `GDExtension`
- `GDExtensionResourceFormatLoader`
- `GDExtensionResourceFormatSaver`
- `GDExtension::open_library()`
- `GDExtension::initialize_library()`

## Flow Notes

- Extensions initialize by level so they can hook into core, servers, scene, or editor phases.
- Extension resources load metadata and library paths from project files.
- Lifecycle order must align with engine registration and cleanup order.

## Code Links

- `core/extension/gdextension.h`
- `core/extension/gdextension.cpp`
- `core/extension/gdextension_resource_format.*`
- `core/extension/gdextension.compat.inc`