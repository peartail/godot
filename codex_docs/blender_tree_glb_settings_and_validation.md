# Blender Tree GLB Settings and Validation

## 12. Blender glTF Export Settings

Open Blender's glTF exporter:

```text
File
  -> Export
  -> glTF 2.0 (.glb/.gltf)
```

Use the following settings.

### Format

```text
Format = glTF Binary (.glb)
```

### Include

```text
Selected Objects or Active Collection = On
Visible Objects                        = Optional safety filter
Renderable Objects                     = Optional safety filter
Custom Properties                      = On
```

Use either Selected Objects or Active Collection consistently. Active Collection is recommended when the `TREE_EXPORT` collection is kept clean.

### Data and Mesh

```text
Mesh             = On
UVs              = On
Normals          = On
Vertex Colors    = On
Materials        = Export
Apply Modifiers  = On
Tangents         = On only when required by the material
```

### Disable Unused Data

```text
Animation   = Off
Shape Keys  = Off unless explicitly required
Skinning    = Off
Cameras     = Off
Lights      = Off
```

### Compression

For the initial pipeline, disable geometry compression:

```text
Draco compression   = Off
MeshOpt compression = Off
```

First validate uncompressed Position, Normal, UV, Material, and Color attributes in Godot. Add compression only after the complete import and rendering path has been validated on target hardware.

## 13. Pre-Export Checklist

### Scene and selection

- [ ] Exactly one approved tree variant is selected for export.
- [ ] Only the intended `TREE_EXPORT` collection is included.
- [ ] No source curves, Geometry Nodes helpers, cameras, lights, or preview objects are included.
- [ ] LOD0, LOD1, and LOD2 objects use the required names.
- [ ] Optional collision uses the required `COL_` name.

### Transform

- [ ] Blender uses Metric units with unit scale 1.0.
- [ ] The trunk-base center is the origin.
- [ ] Ground contact is at Blender `Z=0`.
- [ ] Every export object has Location `(0,0,0)`.
- [ ] Every export object has Rotation `(0,0,0)`.
- [ ] Every export object has Scale `(1,1,1)`.
- [ ] No export object has negative scale.
- [ ] All LODs overlap correctly at the same pivot.

### Geometry and LOD

- [ ] LOD silhouettes are consistent.
- [ ] LOD1 and LOD2 do not expand beyond LOD0.
- [ ] Triangle counts meet the current budget.
- [ ] Face orientation has been inspected with backface culling.
- [ ] There are no accidental duplicate or loose elements.
- [ ] Intended closed geometry does not contain unintended open boundaries.
- [ ] Export topology has stable triangulation.

### Materials and attributes

- [ ] Slot 0 is the trunk material.
- [ ] Slot 1 is the canopy material.
- [ ] Material-slot order is identical on all LODs.
- [ ] `UVMap` exists on all render LODs.
- [ ] `wind_data` exists on all render LODs.
- [ ] `wind_data` is the active color attribute.
- [ ] Wind channel values follow the documented contract.
- [ ] Tangents are enabled only if required.

### Metadata and export options

- [ ] Root custom properties contain asset, species, seed, LOD, and collision metadata.
- [ ] `Custom Properties` is enabled in the exporter.
- [ ] Export format is binary GLB.
- [ ] UVs, normals, vertex colors, materials, and modifiers are enabled.
- [ ] Animation, skinning, cameras, and lights are disabled.
- [ ] Geometry compression is disabled for initial validation.

## 14. Post-Export Godot Validation

Copy the GLB into the game project's `res://` tree asset directory and allow Godot to import it.

Verify:

- [ ] The GLB imports as a `PackedScene` without errors.
- [ ] The expected LOD node names are present.
- [ ] The tree stands upright.
- [ ] The real-world scale is correct.
- [ ] The ground-center pivot is correct.
- [ ] LOD0, LOD1, and LOD2 overlap at the same transform.
- [ ] Each render LOD has two mesh surfaces in the expected material order.
- [ ] UV coordinates are present.
- [ ] Vertex color data is present as `COLOR_0`.
- [ ] Root and node custom properties are present as glTF extras when required.
- [ ] No camera, light, source helper, or unintended mesh was imported.

After mesh extraction and `OpenWorldTreeVariant` creation, verify:

- [ ] LOD0 is selected before `lod1_distance`.
- [ ] LOD1 is selected between `lod1_distance` and `lod2_distance`.
- [ ] LOD2 is selected between `lod2_distance` and `max_distance`.
- [ ] The tree is culled beyond `max_distance`.
- [ ] LOD switching does not produce unacceptable scale or silhouette popping.
- [ ] Multiple instances share mesh and material resources.
- [ ] `INSTANCE_COLOR` and `INSTANCE_CUSTOM` work with the intended vegetation shader.

## 15. Recommended Asset Package

Recommended production package for one variant:

```text
umbrella_broadleaf_01/
|- tree_umbrella_broadleaf_01.glb
|- tree_umbrella_broadleaf_01_manifest.json   # Optional external manifest
|- tree_umbrella_broadleaf_01_preview.png
`- tree_umbrella_broadleaf_01_lod_sheet.png
```

The GLB is the required engine input. The external manifest and preview images are optional but strongly recommended for automated validation, source tracking, and agent review.

## 16. Minimum Acceptance Contract

An exported tree is ready for `OpenWorldTreeVariant` conversion when all of the following are true:

```text
1. One GLB contains exactly one approved tree variant.
2. LOD0, LOD1, and LOD2 use the required node names.
3. All LODs share a ground-centered identity transform.
4. Every LOD uses trunk material slot 0 and canopy material slot 1.
5. Every LOD contains UVMap, normals, and active wind_data vertex colors.
6. Root extras contain stable asset, species, seed, and LOD metadata.
7. The GLB imports into Godot without warnings or missing data.
```

If these requirements remain stable, a Godot Tree Import/Bake tool can reliably extract the three meshes and generate an `OpenWorldTreeVariant` resource without per-asset manual correction.
