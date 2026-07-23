# Placement × Vine Phase 2a — Creeping + vine_density

Date: 2026-07-23

## Goal

Open World Placement to `MODE_CREEPING` vines with an independent non-Bramble density pool, without inventing new vine generators.

## Locked decisions

1. **Scope:** Placement accepts `MODE_BRAMBLE` (primary pool) and `MODE_CREEPING` (vine pool). Climbing / Hanging / TreeWrap stay `VINE_MODE_PHASE2`.
2. **Density (Approach B):** Preset property `vine_density_per_100_square_meters` feeds all non-Bramble vine entries. Creeping uses it now; future modes reuse the same pool.
3. **Bramble** remains in `density_per_100_square_meters` with Tree / Rock (Phase 1 compatibility).
4. **Support auto-wire:** If the Creeping request template has an empty `support_path`, Placement sets a relative path to the `SimpleTerrain3D` found for surface projection. Explicit template paths win when set.
5. **Vine generator:** `OpenWorldVineGenerator3D` accepts `SimpleTerrain3D` as Creeping/Climbing support (via `get_brush_hit`), because SimpleTerrain keeps `MeshInstance3D::get_mesh()` empty and uses chunk meshes.

## Pools

| Pool | Density field | Entries |
| --- | --- | --- |
| Primary | `density_per_100_square_meters` | Tree, Rock, Vine+Bramble |
| Vine | `vine_density_per_100_square_meters` | Vine+Creeping (later: other non-Bramble) |

```
primary_count = round(area * density / 100)
vine_count    = round(area * vine_density / 100)
max check: primary_count + vine_count <= max_objects_per_operation
```

Spacing uses one shared XZ candidate / existing-placement check across both pools.

## Creeping instantiate order

Bramble can generate before parenting. Creeping needs a resolvable `support_path`, so:

1. Create vine node + transform/meta (no generate yet when support must be wired).
2. `add_child` under the generated root.
3. Resolve support (`template` path if non-empty and resolvable, else `get_path_to(terrain)`).
4. `generate_vine()`; null LOD0 / missing support fails the transaction.

## Out of scope

- Climbing / Hanging / TreeWrap placement
- Tree-to-tree bridge vines
- Moving Bramble into the vine density pool
- Editor dock Creeping quick-add button
