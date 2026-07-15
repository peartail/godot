# Blender Tree Material, Wind, and Collision Contract

## 7. Material Contract

Every render LOD must use the same two material slots in the same order.

```text
Slot 0 = MAT_Tree_<SpeciesName>_Trunk
Slot 1 = MAT_Tree_<SpeciesName>_Canopy
```

Example:

```text
Slot 0 = MAT_Tree_UmbrellaBroadleaf_Trunk
Slot 1 = MAT_Tree_UmbrellaBroadleaf_Canopy
```

Rules:

- Assign all trunk and branch faces to slot 0.
- Assign all canopy faces to slot 1.
- Remove empty and unused material slots.
- Reuse the same material names across variants of one species.
- Keep material-slot order identical on LOD0, LOD1, and LOD2.
- Prefer single-sided materials for closed trunk and canopy geometry.
- Do not enable `Double Sided` merely to hide open trunk or branch boundaries.
- Fix unintended open boundaries before production export.
- Keep metallic at zero for ordinary bark and foliage unless the art direction explicitly requires otherwise.

Godot's `OpenWorldTreeVariant.material_override` applies one override to the whole mesh. Leave it empty when trunk and canopy need different materials. The materials embedded in the mesh surfaces will then be used.

## 8. UV Contract

Every render LOD must contain one production UV set.

```text
Blender UV map name = UVMap
glTF attribute       = TEXCOORD_0
```

Rules:

- Ensure every visible face has valid UV coordinates.
- Keep UV islands inside their intended texture or atlas region.
- Keep texel density reasonably consistent across variants of the same species.
- Avoid conspicuous seams at the expected gameplay distance.
- If a production texture atlas is planned, repack UVs before final approval.
- Do not treat generated inspection UVs as production-ready without review.

Tangents are required when the production material uses a normal map or tangent-space calculations. Tangents may be omitted for flat-color stylized materials that do not use them.

## 9. Wind Vertex-Color Contract

Every render LOD must contain an active Blender Color Attribute with the following settings:

```text
Name      = wind_data
Domain    = Point
Data Type = Float Color
```

The Blender glTF exporter writes the active color attribute as `COLOR_0`.

Channel contract:

```text
R = primary trunk bend weight
G = branch-tip or secondary bend weight
B = canopy flutter weight
A = stable wind phase or random variation
```

Recommended values:

```text
Trunk base      R=0, low G, B=0
Upper trunk     gradually increasing R
Branch root     lower G
Branch tip      higher G
Canopy          B greater than 0
Each cluster    stable A value
```

Rules:

- Keep all channel values within `0..1`.
- Anchor the trunk base with primary bend close to zero.
- Use smooth gradients along trunks and branches.
- Avoid abrupt weight boundaries that create tearing-like deformation.
- Preserve the attribute during LOD generation.
- Make `wind_data` the active/render color attribute before export.
- Confirm in Godot that the imported mesh contains vertex color data.

`COLOR_0` is per-vertex tree deformation data. It is separate from `OpenWorldTreePlacementData.custom_data`, which becomes per-instance `INSTANCE_CUSTOM` shader data.

## 10. Collision Reference Contract

Collision export is optional in the current tree-system version.

Recommended collision node:

```text
COL_Tree_<SpeciesName>_<VariantNumber>
```

Rules:

- Use a simple 6- to 8-sided cylinder, capsule-like mesh, or similarly cheap trunk approximation.
- Do not generate canopy collision by default.
- Do not copy the detailed render mesh as collision.
- Use the same origin and identity transform as all LODs.
- Do not assign render materials.
- Record the intended radius and height as custom properties.

`OpenWorldTree3D` does not currently generate physics bodies from this mesh. The node is a reference for a future import/bake or interaction system. The current `OpenWorldTreeVariant` stores collision radius and height as metadata.

## 11. Custom Properties for Automatic Import

Add custom properties to the variant's export collection root or root Empty. These values are intended for a future Tree Import/Bake tool and are exported to glTF `extras` when `Custom Properties` is enabled.

Recommended root properties:

| Property | Example | Purpose |
|---|---|---|
| `schema_version` | `1.0.0` | Metadata contract version |
| `asset_id` | `Tree_UmbrellaBroadleaf_01` | Stable variant identifier |
| `species_id` | `umbrella_broadleaf` | Stable species identifier |
| `source_seed` | `1207` | Authoring generator seed |
| `lod1_distance_m` | `25.0` | LOD1 transition distance |
| `lod2_distance_m` | `55.0` | LOD2 transition distance |
| `max_distance_m` | `120.0` | Render-culling distance |
| `collision_radius_m` | `0.57` | Simplified trunk radius |
| `collision_height_m` | `3.75` | Simplified trunk collision height |

Recommended properties on each render LOD object:

```text
asset_id       = "Tree_UmbrellaBroadleaf_01"
source_seed    = 1207
lod_index      = 0, 1, or 2
export_status  = "BAKED"
```

Optional measured properties:

```text
triangle_count
material_slot_0
material_slot_1
wind_attribute
wind_contract
```

Custom properties supplement validation. Node names, mesh data, materials, transforms, and attributes remain the authoritative export contract.

