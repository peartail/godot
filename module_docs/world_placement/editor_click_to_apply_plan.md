# World Placement Editor — Click-to-Apply MVP Plan

**Status:** Completed (2026-07). See [current_status.md](current_status.md) and [editor_interaction.md](editor_interaction.md) for the live UX.

**Original goal:** Viewport click applies `OpenWorldPlacement3D.apply_placement` on SimpleTerrain3D with undo and footprint cursor.

**Architecture:** Dedicated `OpenWorldPlacementEditorPlugin` (not SimpleTerrain legacy UI). Handles selected `OpenWorldPlacement3D`; auto-selects terrain via vertical ray (camera Y in editor); uses camera `get_brush_hit` for click center, runtime samples via `sample_surface_at_world_xz`.

## MVP checklist (done)

1. Toolbar: Select | Apply mode — **done**
2. Overlay: preset info, last apply report, seed — **done**
3. Lock footprint on click → confirm **Apply** / **Cancel** — **done** (supersedes instant-click apply)
4. Footprint cursor (circle/rect/ellipse) with terrain projection — **done**
5. UndoRedo via placement-data snapshots + `rebuild_generated` — **done**
6. Preset dock with materials — **done** (was “out of scope” in first draft)
7. Docs updated — **done**

## Intentionally removed after MVP

- Continuous mouse-move `preview_placement` (too expensive when footprints overlap existing records)

## Still out of scope

- Non-Bramble vines in placement — [vine_placement_phase2.md](vine_placement_phase2.md)
- MultiMesh / streaming

Rejected (not a future candidate): continuous drag strokes. Authoring stays lock → confirm Apply.
