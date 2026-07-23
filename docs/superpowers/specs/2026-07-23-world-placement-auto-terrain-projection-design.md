# World Placement — Auto Terrain Vertical Projection

Date: 2026-07-23  
Status: Approved for implementation

## Goal

Remove `OpenWorldPlacement3D.terrain_path`. When an apply footprint overlaps terrain, select a surface backend automatically and project the editor footprint gizmo and placement anchors onto that surface via **vertical projection** (XZ → surface).

## Terminology

**Vertical surface projection (수직 표면 사영):** drop each footprint sample straight down (world −Y) onto the terrain surface. Not camera ray picking for placement anchors (editor click may still use a camera ray only to choose the apply center).

## Terrain selection (option B)

1. Build a vertical ray at the apply/cursor center XZ.
2. Cast `DOWN` through all in-tree `SimpleTerrain3D` nodes.
3. Use the **closest hit** (first targeted terrain).
4. If none hit → flat plane at apply center Y, normal UP.

### Ray origin Y

| Context | Origin Y |
| --- | --- |
| Editor (preview/apply/gizmo) | Scene-view camera `global_position.y` |
| Runtime / headless / default API | `world_position.y + 10000` |

API: optional `vertical_ray_origin_y` on `preview_placement` / `apply_placement`. When infinite (default sentinel), use runtime rule above. Editor passes camera Y.

## Projection after selection

Once a terrain is selected for the operation:

- Candidate positions and footprint gizmo ring points call `sample_surface_at_world_xz`.
- Failed samples increment `rejected_missing_surface` and are skipped.
- Report includes `terrain_projection` bool.

## Out of scope

- Blending multiple terrains per footprint
- `OpenWorldTerrain3D` as backend
- Persistent camera NodePath property

## Files

- `modules/open_world_terrain/open_world_placement_3d.*`
- `modules/open_world_terrain/editor/open_world_placement_editor_plugin.*`
- docs/XML/tests under World Placement
