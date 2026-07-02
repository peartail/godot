# Scene Registration

## Scope

Scene class/resource registration, unregister cleanup, string name setup, and scene-level singleton/resource setup.

## Entry Points

- `register_scene_types()`
- `unregister_scene_types()`
- `SceneStringNames`

## Flow Notes

- Scene registration happens after core/server registration and before editor/module-specific scene extensions.
- Registration order matters because many scene classes depend on resources and servers.
- Scene string names centralize frequently-used identifiers.

## Code Links

- `scene/register_scene_types.h`
- `scene/register_scene_types.cpp`
- `scene/scene_string_names.h`