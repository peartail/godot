# World Placement Editor — Click-to-Apply MVP Plan

**Goal:** Viewport click applies `OpenWorldPlacement3D.apply_placement` on SimpleTerrain3D with undo and footprint cursor.

**Architecture:** Dedicated `OpenWorldPlacementEditorPlugin` (not SimpleTerrain legacy UI). Handles selected `OpenWorldPlacement3D`; resolves terrain via `terrain_path`; uses `get_brush_hit` for click, runtime still samples via `sample_surface_at_world_xz`.

**MVP scope**

1. Toolbar: Select | Apply mode
2. Overlay: preset id, requested/accepted from last preview, seed
3. Click in Apply mode → `apply_placement` + UndoRedo (data snapshot + `rebuild_generated`)
4. Motion → footprint cursor (circle/rect/ellipse from preset) + lightweight `preview_placement` status
5. Docs: mark editor path as available in `editor_interaction.md`

**Out of scope:** preset library dock, non-Bramble vines, MultiMesh

Rejected (not a future candidate): continuous drag strokes. Authoring stays click-to-apply.
