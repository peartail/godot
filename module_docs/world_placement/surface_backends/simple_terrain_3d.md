# Surface Backend: SimpleTerrain3D

World Placement is not owned by SimpleTerrain. SimpleTerrain3D is the Phase 1 **surface backend**: it answers where the ground is so placement can project footprints and anchors.

## Required API

```
Dictionary sample_surface_at_world_xz(Vector3 world_position, float max_distance = 100000.0) const
Dictionary get_brush_hit(Vector3 ray_origin, Vector3 ray_direction) const
```

### Success sample (`sample_surface_at_world_xz`)

- `success`: `true`
- `position`: world-space surface point
- `normal`: surface normal
- `height`: sampled height
- `tile_cell`: terrain tile cell for the sample

### Failure sample

- `success`: `false`
- `error_code`: machine-readable failure reason

## Terrain selection (no `terrain_path`)

`OpenWorldPlacement3D` selects a target terrain automatically:

1. Cast a **vertical** ray at the apply/cursor center XZ (`DOWN`).
2. Among in-tree `SimpleTerrain3D` nodes, take the **closest hit**.
3. If none hit → flat plane at apply center Y.

Ray origin Y:

| Context | Origin Y |
| --- | --- |
| Editor | Scene-view camera `global_position.y` |
| Runtime / default API | `world_position.y + 10000` |

## Placement rules that depend on this backend

- Projection is **always vertical** (XZ → surface) after terrain selection. This is vertical surface projection (사영), not camera-ray placement of anchors.
- Editor click may still use a camera ray only to choose the apply **center**, then switches to vertical projection for candidates and the footprint gizmo.
- Headless scripts and editor apply paths share the same sampler so results match when the same ray origin Y is supplied.
- Height and slope filters in the placement preset use the sampled surface values.
- Missing or failed samples contribute to structured report counters (for example missing-surface) and do not place that candidate.

## Out of scope for this backend doc

- Height sculpt brushes and material painting (SimpleTerrain editor)
- PackedScene profile click-placement (`SimpleWorld*` editor flow)
- Streaming / tile ownership of placement records (deferred World Placement work)

## Related code

- Sampler: `modules/simple_terrain/simple_terrain_3d.*`
- Procedural coordinator: `modules/open_world_terrain/open_world_placement_3d.*`
- Domain overview: [../README.md](../README.md)
