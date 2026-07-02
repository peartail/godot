# Packed Scene And Text Format

## Scope

Packed scene serialization, scene state, instancing, text resource format, and scene/resource text parsing.

## Entry Points

- `PackedScene`
- `SceneState`
- `ResourceFormatLoaderText`
- `ResourceFormatSaverText`

## Flow Notes

- `PackedScene` stores node tree data for instancing and serialization.
- Text resource format is used by `.tscn`, `.tres`, and text-based scene/resource files.
- Node/resource references interact with `ResourceUID`, paths, and load caches.

## Code Links

- `scene/resources/packed_scene.*`
- `scene/resources/resource_format_text.*`
- `core/io/resource_loader.*`
- `core/io/resource_saver.*`