# Surface Backend: SimpleTerrain3D

World Placement is not owned by SimpleTerrain. SimpleTerrain3D is the Phase 1 **surface backend**: it answers where the ground is so placement can project footprints and anchors.

## Required API

```
Dictionary sample_surface_at_world_xz(Vector3 world_position, float max_distance = 100000.0) const
```

### Success sample

- `success`: `true`
- `position`: world-space surface point
- `normal`: surface normal
- `height`: sampled height
- `tile_cell`: terrain tile cell for the sample

### Failure sample

- `success`: `false`
- `error_code`: machine-readable failure reason

## Placement rules that depend on this backend

- Projection is **always vertical** (XZ → surface). No physics ray or viewport picking in the runtime path.
- Headless scripts and editor apply paths must share this sampler so results match.
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
