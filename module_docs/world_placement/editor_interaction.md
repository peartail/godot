# Editor Interaction (World Placement)

## Primary path (forward)

Authoring surface population for trees, rocks, and Bramble vines goes through **World Placement**:

- Runtime: `OpenWorldPlacement3D` + `OpenWorldPlacementPreset` / `Entry` / `Data`
- Interaction: **lock footprint → confirm Apply** (area replacement)
- Surface backend (optional): auto-selects `SimpleTerrain3D` via vertical ray; otherwise flat-plane placement
- Editor: `OpenWorldPlacementEditorPlugin`
- Preset dock: **World Placement Presets** (preset/entry/profile/material editing)

### Editor usage

1. Add `OpenWorldPlacement3D` to the scene.
2. Assign `active_preset` (tree / rock / bramble entries + materials) via Inspector or **Presets** dock.
3. Optionally rely on in-scene `SimpleTerrain3D` for surface projection (no `terrain_path` property).
4. Select the placement node → toolbar **Apply** mode.
5. Move mouse → green footprint follows the cursor.
6. Click → lock footprint (amber) and show overlay **Apply** / **Cancel**.
7. Press **Apply** to commit, or click elsewhere / **Cancel** to unlock and move again.
8. Undo/redo restores placement-data snapshots and rebuilds generated nodes.
9. To refresh or strip preview meshes without deleting placement records: Scene Tree right-click on `OpenWorldPlacement3D` → **Rebuild Generated** / **Clear Generated**, or the same buttons at the bottom of the Inspector. Agent path: `rebuild_generated()` / `clear_generated()`.

Mouse motion does **not** run `preview_placement` (performance). Solving happens on confirm only.

### Preset dock

1. Select `OpenWorldPlacement3D`.
2. Open **Presets** from the spatial editor menu or the dock.
3. Assign/create a preset, add weighted entries, set profiles/request templates, and optional materials:
   - Tree: `trunk_material`, `foliage_material`
   - Rock: `preview_material`
   - Vine (Bramble): `stem_material`, `foliage_material`
4. Validation status appears at the top of the dock.

See [editor_click_to_apply_plan.md](editor_click_to_apply_plan.md) for the original MVP checklist (now completed).

## Legacy path (disabled in editor)

PackedScene library placement (`SimpleWorld*`) is **legacy**:

| Piece | Status |
| --- | --- |
| Placement Mode / dock / “World Objects…” inspector button | Disabled (`SIMPLE_WORLD_PLACEMENT_EDITOR_ENABLED = 0`) |
| `SimpleTerrain3D` / `SimpleWorldPlacement3D` `world_placement_*` Inspector | Hidden (`PROPERTY_USAGE_NO_EDITOR`) |
| ClassDB + scene STORAGE | Kept so existing scenes still load |
| Navigation hooks that read placement data | Still compile; scenes should be cleaned later |

Flag: `modules/simple_terrain/simple_world_placement_legacy.h`

## Out of scope here

- OpenWorldTerrain height/layer sculpt brushes (unrelated)
- Deleting `SimpleWorld*` ClassDB types (after scene cleanup)
- Non-Bramble vine modes in placement (see [vine_placement_phase2.md](vine_placement_phase2.md))
- Preset dock Rock/Bramble one-click add buttons (convenience)

Rejected interaction (not planned): continuous drag strokes.
