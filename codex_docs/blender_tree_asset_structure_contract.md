# Blender Tree GLB Export Guide

## 1. Purpose

This document defines the Blender export contract for baked tree variants used by Godot's `OpenWorldTreeVariant`, `OpenWorldTreeSpecies`, and `OpenWorldTree3D` classes.

Follow this guide when exporting a tree from Blender. The preferred delivery format is **one binary GLB file per tree variant**.

```text
One Blender tree variant
    -> one GLB
    -> three named LOD mesh nodes
    -> optional collision reference node
    -> one OpenWorldTreeVariant resource in Godot
```

This is a static tree rendering format. Runtime mesh cutting, falling-tree physics, and construction-piece meshes are outside this export contract.

## 2. Required Deliverable

Export each approved tree variant as one self-contained `.glb` file.

Recommended filename:

```text
tree_<species_id>_<variant_number>.glb
```

Example:

```text
tree_umbrella_broadleaf_01.glb
```

GLB is preferred over separate `.gltf` and `.bin` files because it is easier to move, reimport, validate, and process with a future automatic Tree Import/Bake tool.

## 3. Required Node Structure

The GLB must contain one mesh object per LOD.

```text
Tree_UmbrellaBroadleaf_01
|- SM_Tree_UmbrellaBroadleaf_01_LOD0
|- SM_Tree_UmbrellaBroadleaf_01_LOD1
|- SM_Tree_UmbrellaBroadleaf_01_LOD2
`- COL_Tree_UmbrellaBroadleaf_01       # Optional
```

Required render-node naming pattern:

```text
SM_Tree_<SpeciesName>_<VariantNumber>_LOD0
SM_Tree_<SpeciesName>_<VariantNumber>_LOD1
SM_Tree_<SpeciesName>_<VariantNumber>_LOD2
```

Optional collision-node naming pattern:

```text
COL_Tree_<SpeciesName>_<VariantNumber>
```

Example:

```text
SM_Tree_UmbrellaBroadleaf_01_LOD0
SM_Tree_UmbrellaBroadleaf_01_LOD1
SM_Tree_UmbrellaBroadleaf_01_LOD2
COL_Tree_UmbrellaBroadleaf_01
```

Rules:

- Each LOD must be one Blender `MESH` object.
- A single LOD mesh may contain multiple disconnected geometry islands.
- Trunk, branches, and canopy may intersect where the intersection is visually hidden.
- Do not export source curves, Geometry Nodes helpers, cameras, lights, preview floors, or unused modules.
- Do not export all generated candidates in one GLB. Export only one approved variant.

## 4. Blender Collection Layout

Keep editable source data separate from export-ready data.

```text
TREE_SOURCE
|- Curves
|- GeometryNodes
|- BranchModules
|- CanopyModules
`- GeneratedCandidates

TREE_EXPORT
`- Tree_UmbrellaBroadleaf_01
   |- SM_Tree_UmbrellaBroadleaf_01_LOD0
   |- SM_Tree_UmbrellaBroadleaf_01_LOD1
   |- SM_Tree_UmbrellaBroadleaf_01_LOD2
   `- COL_Tree_UmbrellaBroadleaf_01
```

Export only the relevant child collection under `TREE_EXPORT`.

## 5. Units, Axis, Origin, and Transforms

Use the following scene units:

```text
Unit System = Metric
Unit Scale  = 1.0
1 Blender unit = 1 meter
```

All LOD and collision objects must share the same origin and transform.

```text
Location = (0, 0, 0)
Rotation = (0, 0, 0)
Scale    = (1, 1, 1)
```

Geometry rules:

- The tree stands along Blender positive Z.
- The center of the trunk base is the object origin.
- The ground-contact plane is Blender `Z = 0`.
- No part intended to touch the ground should float above `Z = 0`.
- Root flares may extend sideways but should not extend materially below the ground plane unless intentional.
- Apply object rotation and scale before export.
- Do not leave negative scale on an export object.
- Do not manually rotate the tree to compensate for Godot's Y-up coordinate system.
- Blender's glTF exporter performs the Blender-to-Godot axis conversion.

Recommended finalization sequence:

1. Move the mesh so the trunk-base center is at the intended origin.
2. Set the object's origin to the trunk-base center.
3. Place the object at world origin.
4. Apply Rotation and Scale with `Ctrl+A`.
5. Confirm that Location and Rotation are zero and Scale is one.
6. Repeat the check for LOD0, LOD1, LOD2, and collision.

Do not apply object Location if doing so would move the origin away from the required ground-center pivot. The final result, rather than a specific operator sequence, is the contract.

## 6. LOD Requirements

Initial LOD distance guidance:

| LOD | Suggested distance | Initial triangle guidance |
|---|---:|---:|
| LOD0 | 0-25 m | Approximately 2,000-8,000 |
| LOD1 | 25-55 m | Approximately 40%-60% of LOD0 |
| LOD2 | 55-120 m | Approximately 10%-20% of LOD0 |

LOD rules:

- Preserve the overall height, canopy width, and ground pivot across all LODs.
- Do not let LOD1 or LOD2 become larger than LOD0.
- Preserve the principal trunk and major branch directions.
- Remove small branches before changing the primary silhouette.
- Simplify the canopy while preserving its outer volume.
- Keep the same material-slot order on every LOD.
- Preserve the required UV map and `wind_data` color attribute on every LOD.
- Inspect transitions from multiple camera directions.
- Do not rely solely on an automatic Decimate result.

To keep exported triangle counts stable, use an applied Triangulate modifier on export copies or otherwise finalize triangulation before approval. Blender's glTF exporter triangulates polygons, but explicit triangulation makes topology and triangle-budget reviews reproducible.

