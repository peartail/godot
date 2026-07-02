# NodePath

## Scope

Scene node paths, subnames, resource/property path segments, and serialized path representation.

## Entry Points

- `NodePath`
- `NodePath::get_name()`
- `NodePath::get_subname()`
- `NodePath::is_absolute()`

## Flow Notes

- `NodePath` stores path names and subnames separately.
- Scene references, animation tracks, inspector paths, and serialized resources depend on stable NodePath parsing.
- Relative and absolute paths have different resolution behavior in scene tree code.

## Code Links

- `core/string/node_path.h`
- `core/string/node_path.cpp`
- `scene/main/node.*`