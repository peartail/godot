# Umbrella Broadleaf Agent Review Sample

This directory contains an agent-readable glTF 2.0 sample of `Tree_UmbrellaBroadleaf_01`, generated from master seed `1207`.

The editable preview remains in `cartoon_palm_tree.blend`. The files here are baked ordinary meshes intended for engine-pipeline review, not final approved shipping assets.

## Start here

1. Read `agent_manifest.json` for the complete data contract and measured values.
2. Inspect `tree_umbrella_broadleaf_01_agent_sample.gltf` as JSON.
3. Keep its sibling `.bin` file in the same directory when importing.
4. The combined sample contains four co-located nodes: LOD0, LOD1, LOD2, and collision.
5. Enable only one render LOD at a time in the engine.

## Files

| Role | glTF JSON | Binary buffer |
|---|---|---|
| Combined review sample | `tree_umbrella_broadleaf_01_agent_sample.gltf` | `tree_umbrella_broadleaf_01_agent_sample.bin` |
| LOD0 | `tree_umbrella_broadleaf_01_lod0.gltf` | `tree_umbrella_broadleaf_01_lod0.bin` |
| LOD1 | `tree_umbrella_broadleaf_01_lod1.gltf` | `tree_umbrella_broadleaf_01_lod1.bin` |
| LOD2 | `tree_umbrella_broadleaf_01_lod2.gltf` | `tree_umbrella_broadleaf_01_lod2.bin` |
| Collision | `tree_umbrella_broadleaf_01_collision.gltf` | `tree_umbrella_broadleaf_01_collision.bin` |

## Naming contract

- `SM_Tree_UmbrellaBroadleaf_01_LOD0`
- `SM_Tree_UmbrellaBroadleaf_01_LOD1`
- `SM_Tree_UmbrellaBroadleaf_01_LOD2`
- `COL_Tree_UmbrellaBroadleaf_01`
- `MAT_Tree_UmbrellaBroadleaf_Trunk`
- `MAT_Tree_UmbrellaBroadleaf_Canopy`

Custom node extras include the asset ID, source seed, LOD index, triangle count, material-slot contract, wind-data contract, and export status.

## Geometry and vertex data

Every render LOD has:

- ground-centered origin;
- identity rotation and scale;
- `Z=0` ground contact before glTF axis conversion;
- one UV set exported as `TEXCOORD_0`;
- normals;
- two stable material slots;
- wind data exported as `COLOR_0`.

`COLOR_0` uses this channel contract:

```text
R = primary trunk bend weight
G = branch-tip / secondary bend weight
B = canopy flutter weight
A = stable phase variation
```

Tangents are intentionally absent because this sample has no texture or normal map.

## Suggested engine assembly

Create one logical tree scene or prefab with three render children and one collision child. All children use the same transform and pivot.

```text
Tree_UmbrellaBroadleaf_01
|- SM_Tree_UmbrellaBroadleaf_01_LOD0
|- SM_Tree_UmbrellaBroadleaf_01_LOD1
|- SM_Tree_UmbrellaBroadleaf_01_LOD2
`- COL_Tree_UmbrellaBroadleaf_01
```

Initial distance bands for review:

```text
LOD0: 0-25 m
LOD1: 25-55 m
LOD2: 55-120 m
```

These distances are provisional. Tune them using the intended gameplay camera, shadow settings, target hardware, and tree density.

For collision, the imported mesh can be used as a reference, but a native cylinder or capsule is preferable when available. The sample collision dimensions are radius `0.57 m` and height `3.75 m`; the canopy has no collision.

For a large forest, use shared meshes and materials. Group instances by active LOD and use an instancing path rather than generating unique runtime meshes.

## Review gates before approval

- Confirm scale, axis conversion, and ground pivot in the target engine.
- Confirm that `COLOR_0` is readable by the vegetation shader.
- Confirm that LOD switching does not produce unacceptable silhouette or color popping.
- Confirm that material names remain stable after import and re-import.
- Confirm that the native collision replacement aligns with the trunk.
- Decide whether production assets require an atlas, normal map, tangents, or an impostor stage.
