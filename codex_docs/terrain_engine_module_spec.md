# Terrain Engine Module Specification

## Module Layout

The terrain implementation is packaged as a Godot engine module under `modules/simple_terrain`.

```text
modules/simple_terrain/
  SCsub
  config.py
  register_types.h
  register_types.cpp
  simple_terrain_data.h
  simple_terrain_data.cpp
  simple_terrain_3d.h
  simple_terrain_3d.cpp
  editor/
    simple_terrain_editor_plugin.h
    simple_terrain_editor_plugin.cpp
```

## Runtime Classes

`SimpleTerrainData` is a `Resource` that stores a square height field.

- `grid_size`: number of terrain quads per axis.
- `cell_size`: world spacing between neighboring vertices.
- `height_data`: `(grid_size + 1) * (grid_size + 1)` height values.
- New resources initialize as flat terrain, not random noise.

`SimpleTerrain3D` is a `MeshInstance3D`-derived node that renders a `SimpleTerrainData` resource.

- Terrain geometry is split into chunks using `chunk_size`.
- Each chunk is rendered through its own internal `RenderingServer` instance.
- The inherited `MeshInstance3D::mesh` is kept empty to avoid a single full-terrain renderable.
- Brush picking uses XZ DDA traversal to test only cells crossed by the ray.
- Brush edits rebuild only affected chunks.

## Height Triplanar Material

`SimpleTerrain3D` includes a first-pass built-in height-based triplanar material.

- `terrain_material` applies a user-specified material directly to all internal terrain chunks.
- `use_builtin_triplanar_material` enables the generated triplanar shader material when `terrain_material` is empty.
- `triplanar_low_texture`, `triplanar_mid_texture`, and `triplanar_high_texture` provide three albedo layers.
- `triplanar_low_color`, `triplanar_mid_color`, and `triplanar_high_color` are color multipliers and make the material useful even before textures are assigned.
- `triplanar_low_height`, `triplanar_high_height`, and `triplanar_blend_width` control automatic height-based layer blending.
- `triplanar_texture_scale` controls world-space texture frequency.
- `triplanar_blend_sharpness` controls triplanar projection blending based on surface normal.

Material priority:

1. `terrain_material`
2. built-in triplanar material when enabled
3. inherited `material_override`

## Editor Features

`SimpleTerrainEditorPlugin` is registered only in editor builds.

- Adds a Terrain toolbar in the 3D editor.
- Supports Raise, Lower, Smooth, and Flatten brush modes.
- Provides Flat and Random generation buttons.
- Commits each paint stroke as one undo/redo action.
- Registers a `SimpleTerrain3D` gizmo plugin.

`SimpleTerrain3D.show_chunk_gizmos` enables chunk boundary visualization in the 3D viewport.

## Build Integration

The module is enabled when 3D is enabled. It is compiled automatically by Godot's module build system.

Typical development build:

```powershell
scons platform=windows target=editor dev_build=yes tools=yes module_mono_enabled=yes -j4
```

After changing exposed methods or properties in a Mono build, regenerate glue and assemblies:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --generate-mono-glue modules\mono\glue
python modules\mono\build_scripts\build_assemblies.py --godot-output-dir bin --godot-platform windows --dev-debug
```

To verify the engine still builds without this module:

```powershell
scons platform=windows target=editor dev_build=yes tools=yes module_mono_enabled=yes module_simple_terrain_enabled=no -j4
```

When the option is disabled, `SimpleTerrainData`, `SimpleTerrain3D`, and the terrain editor plugin are not registered.

## Class Documentation

The module provides Godot class reference XML files under `modules/simple_terrain/doc_classes`.

- `SimpleTerrainData.xml`
- `SimpleTerrain3D.xml`

These are listed by `modules/simple_terrain/config.py` through `get_doc_classes()` and `get_doc_path()`.

## GDExtension Migration Notes

The current implementation is an engine module, but the source layout keeps runtime and editor code separated so it can be migrated later.

- Runtime classes are contained in `simple_terrain_data.*` and `simple_terrain_3d.*`.
- Editor-only code is contained in `editor/simple_terrain_editor_plugin.*` and guarded by editor-only module registration.
- Runtime headers use local includes where possible, reducing hard dependency on engine-module include paths.
- `register_types.cpp` is the only module-specific class registration entry point.

A future GDExtension port would replace `register_types.cpp`, `SCsub`, and `config.py` with a GDExtension initialization entry point and an addon manifest. The largest API compatibility point to recheck is the editor gizmo plugin, because editor extension APIs can differ more than runtime node/resource registration.

## Current Limitations

- Chunk normals are calculated per chunk, so border lighting can be improved later.
- LOD is not implemented yet.
- Collision generation is not implemented yet.
- Terrain chunks are internal render instances, not scene tree child nodes.
- Splatmap painting is not implemented yet. The current material is height-driven; a later pass should add editable per-layer weights to `SimpleTerrainData`.
