# World Placement

World Placement is an independent domain: it populates a world region from a reusable preset by applying a footprint once. It is not a sub-feature of any terrain module.

## Definition

World Placement samples a **surface backend**, then runs one **apply** that performs **area replacement** inside a projected footprint (circle, rectangle, or ellipse). Density, weighted entries, spacing, filters, and optional materials come from a **placement preset**. Interaction is **lock footprint → confirm Apply** (not continuous drag painting).

## Terminology

| Prefer | Avoid / limit |
| --- | --- |
| World Placement | Brush (as product or domain name) |
| Placement Preset / Entry / Data | Brush Preset (in new docs) |
| apply / preview / rebuild / clear | paint / stroke / stamp layer (rejected interaction) |
| vertical surface projection (사영) | camera-ray placement of anchors |
| area replacement (semantic description) | persistent layered stamp |
| surface backend | “feature inside terrain” |

Primary runtime types: `OpenWorldPlacementEntry`, `OpenWorldPlacementPreset`, `OpenWorldPlacementData`, `OpenWorldPlacement3D`. Methods: `preview_placement` / `apply_placement` / `find_terrain_at_world_xz`.

**Editor:** `OpenWorldPlacementEditorPlugin` (lock → Apply confirm) + **World Placement Presets** dock. SimpleWorld PackedScene placement UI remains legacy-disabled. See [editor_interaction.md](editor_interaction.md).

## Relationship to terrain

- Terrain modules own height, materials, streaming, and sculpt brushes.
- World Placement owns presets, authoritative placement records, and generated content under an output parent.
- Terrain is **auto-selected** (vertical ray). Phase 1 backend: `SimpleTerrain3D` only.

See [surface_backends/simple_terrain_3d.md](surface_backends/simple_terrain_3d.md).

## Document index

| Doc | Purpose |
| --- | --- |
| [current_status.md](current_status.md) | What is implemented now |
| [runtime_contract.md](runtime_contract.md) | Phase 1 runtime/data contract |
| [area_replacement.md](area_replacement.md) | Why apply is destructive area replacement |
| [surface_backends/simple_terrain_3d.md](surface_backends/simple_terrain_3d.md) | SimpleTerrain3D sampling + auto selection |
| [editor_interaction.md](editor_interaction.md) | Editor UX (lock/confirm, preset dock, legacy) |
| [editor_click_to_apply_plan.md](editor_click_to_apply_plan.md) | Original MVP plan (completed / superseded notes) |
| [vine_placement_phase2.md](vine_placement_phase2.md) | Vine generator vs Placement vine Phase 2 |
| [../simple_terrain/simple_world_placement_editor_plan.md](../simple_terrain/simple_world_placement_editor_plan.md) | Legacy PackedScene library (editor disabled) |

## Code locations (today)

| Concern | Location |
| --- | --- |
| Procedural World Placement runtime | `modules/open_world_terrain/open_world_placement_*` |
| Editor plugin + preset dock | `modules/open_world_terrain/editor/open_world_placement_*` |
| Surface sampler | `modules/simple_terrain/simple_terrain_3d.*` |
| Decision record | `docs/adr/0001-world-placement-area-replacement.md` |
| Auto-terrain design | `docs/superpowers/specs/2026-07-23-world-placement-auto-terrain-projection-design.md` |

Procedural generator placement and PackedScene profile placement share the domain idea (data-first, rebuildable) but remain separate pipelines for now.
