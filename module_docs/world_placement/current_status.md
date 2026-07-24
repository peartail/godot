# World Placement — Current Status

Last updated: 2026-07-23

## Phase 1 product status

| Area | Status |
| --- | --- |
| Runtime Entry / Preset / Data / Placement3D | Done |
| Area replacement apply / preview / rebuild / clear | Done |
| Tree / Rock / Bramble weighted mix | Done |
| Entry materials (`trunk` / `foliage` / `stem` / `preview`) | Done |
| Preview meshes externalized under `_generated/world_placement/` | Done |
| Auto terrain vertical projection (no `terrain_path`) | Done |
| Editor Apply mode + lock → confirm UX | Done |
| Preset dock (assign/new/entry/material edit) | Done |
| Undo/redo via placement-data snapshots | Done |
| Legacy SimpleWorld editor UI | Disabled |
| Creeping vines + `vine_density_per_100_square_meters` | Done (Phase 2a) |
| Climbing / Hanging / TreeWrap in placement | Deferred (later Phase 2) |
| MultiMesh / streaming | Deferred |

## Authoring UX (editor)

1. Select `OpenWorldPlacement3D`.
2. Open **Presets** dock → assign preset, entries, profiles, materials.
3. Toolbar **Apply** mode.
4. Move mouse → footprint follows (green).
5. Click → lock footprint (amber) + overlay **Apply** / **Cancel**.
6. **Apply** commits; click elsewhere or **Cancel** unlocks.

No continuous mouse-move `preview_placement` (removed for performance). Solve runs on confirm only.

## Surface projection

- Vertical surface projection (사영): XZ → `sample_surface_at_world_xz`.
- Terrain target: closest `SimpleTerrain3D` hit by vertical ray.
- Editor ray origin Y = scene camera Y; runtime default = `world_position.y + 10000`.
- No terrain → flat plane at apply center Y.

See [surface_backends/simple_terrain_3d.md](surface_backends/simple_terrain_3d.md) and `docs/superpowers/specs/2026-07-23-world-placement-auto-terrain-projection-design.md`.

## Sample content (mcp-test-project)

- Preset: `examples/world_ocean_forest_preset.tres`
- Entries: tree + rock + bramble + creeping (vine density) with sample materials
- Scene: `examples/world_ocean_edit.tscn`

## Code map

| Piece | Path |
| --- | --- |
| Runtime | `modules/open_world_terrain/open_world_placement_*.{h,cpp}` |
| Editor plugin | `modules/open_world_terrain/editor/open_world_placement_editor_plugin.*` |
| Preset dock | `modules/open_world_terrain/editor/open_world_placement_preset_dock.*` |
| Tests | `modules/open_world_terrain/tests/test_open_world_placement.h` |
| ADR | `docs/adr/0001-world-placement-area-replacement.md` |

## Next candidates

1. **Placement vine Phase 2 later** — Climbing / Hanging / TreeWrap into the vine density pool (see [vine_placement_phase2.md](vine_placement_phase2.md)).
2. Preset dock quick-add Rock / Bramble / Creeping buttons.
3. MultiMesh / cell streaming backends consuming `OpenWorldPlacementData`.
