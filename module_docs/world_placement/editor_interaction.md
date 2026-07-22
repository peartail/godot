# Editor Interaction (World Placement)

## Primary path (forward)

Authoring surface population for trees, rocks, and Bramble vines goes through **World Placement**:

- Runtime: `OpenWorldPlacement3D` + `OpenWorldPlacementPreset` / `Entry` / `Data`
- Interaction model: **click-to-apply** (center + footprint + density), area replacement
- Surface backend (optional): `SimpleTerrain3D.sample_surface_at_world_xz` when `terrain_path` is set; otherwise flat-plane placement
- Editor: `OpenWorldPlacementEditorPlugin` — select `OpenWorldPlacement3D`, enable **Apply** mode, click
- Preset dock: toolbar **Presets** or dock **World Placement Presets** — assign preset, edit entries, profiles, and materials

### Editor MVP usage

1. Add `OpenWorldPlacement3D` to the scene.
2. Assign `active_preset` (entries for tree / rock / bramble).
3. Optionally set `terrain_path` to a `SimpleTerrain3D` for surface projection. Leave empty to place on a flat plane at the node height.
4. Select the placement node → toolbar **Apply** → click.
5. Undo/redo restores placement data snapshots and rebuilds generated nodes.

Footprint cursor follows preset shape/size. Overlay shows requested count and last preview accepted/replace counts.

### Preset dock

1. Select `OpenWorldPlacement3D`.
2. Open **Presets** from the spatial editor menu or the **World Placement Presets** dock.
3. Assign/create a preset, add weighted entries, set `tree_profile` / rock / vine templates, and optional materials (`trunk_material`, `foliage_material`, `stem_material`, `preview_material`).
4. Validation status appears at the top of the dock.
5. Switch to **Apply** mode and click to place using the active preset.

See [editor_click_to_apply_plan.md](editor_click_to_apply_plan.md).

## Legacy path (disabled in editor)

PackedScene library placement (`SimpleWorld*`) is **legacy**:

| Piece | Status |
| --- | --- |
| Placement Mode / dock / “World Objects…” inspector button | Disabled (`SIMPLE_WORLD_PLACEMENT_EDITOR_ENABLED = 0`) |
| `SimpleTerrain3D` / `SimpleWorldPlacement3D` `world_placement_*` Inspector | Hidden (`PROPERTY_USAGE_NO_EDITOR`) |
| ClassDB + scene STORAGE | Kept so existing scenes still load |
| Navigation hooks that read placement data | Still compile; scenes should be cleaned later |

Flag: `modules/simple_terrain/simple_world_placement_legacy.h`

Re-enable editor UI only for migration debugging by setting `SIMPLE_WORLD_PLACEMENT_EDITOR_ENABLED` to `1`.

## Out of scope here

- OpenWorldTerrain height/layer sculpt brushes (unrelated)
- Deleting `SimpleWorld*` ClassDB types (after scene cleanup)
- Non-Bramble vine modes (Phase 2 runtime)
- Preset library dock (implemented: entry/profile/material editing; rock/vine quick-add buttons remain future convenience)

Rejected interaction (not planned): continuous drag strokes. World Placement stays click-to-apply.
