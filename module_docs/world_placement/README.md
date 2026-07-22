# World Placement

World Placement is an independent domain: it populates a world region from a reusable preset by applying a footprint once. It is not a sub-feature of any terrain module.

## Definition

World Placement samples a **surface backend**, then runs one **apply** that performs **area replacement** inside a projected footprint (circle, rectangle, or ellipse). Density, weighted entries, spacing, and filters come from a **placement preset**. The interaction model is **click-to-apply** only (center + footprint + density).

## Terminology

| Prefer | Avoid / limit |
| --- | --- |
| World Placement | Brush (as product or domain name) |
| Placement Preset / Entry / Data | Brush Preset (in new docs) |
| apply / preview / rebuild / clear | paint / stroke / stamp layer (rejected interaction) |
| area replacement (semantic description) | persistent layered stamp |
| surface backend | “feature inside terrain” |

Primary runtime types: `OpenWorldPlacementEntry`, `OpenWorldPlacementPreset`, `OpenWorldPlacementData`, `OpenWorldPlacement3D`. Methods: `preview_placement` / `apply_placement`.

**Editor:** `OpenWorldPlacementEditorPlugin` provides click-to-apply and the **World Placement Presets** dock (preset/entry/material editing). SimpleWorld PackedScene placement UI remains legacy-disabled. See [editor_interaction.md](editor_interaction.md).

## Relationship to terrain

- Terrain modules own height, materials, streaming, and sculpt brushes.
- World Placement owns presets, authoritative placement records, and generated content under an output parent.
- A terrain type becomes a **surface backend** when it can answer surface queries. Phase 1 uses `SimpleTerrain3D` only.

See [surface_backends/simple_terrain_3d.md](surface_backends/simple_terrain_3d.md).

## Document index

| Doc | Purpose |
| --- | --- |
| [runtime_contract.md](runtime_contract.md) | Phase 1 runtime/data contract (scriptable, headless) |
| [area_replacement.md](area_replacement.md) | Why apply is destructive area replacement |
| [surface_backends/simple_terrain_3d.md](surface_backends/simple_terrain_3d.md) | SimpleTerrain3D sampling contract |
| [editor_interaction.md](editor_interaction.md) | Primary vs legacy editor paths; click-to-apply usage |
| [editor_click_to_apply_plan.md](editor_click_to_apply_plan.md) | Editor MVP plan |
| [../simple_terrain/simple_world_placement_editor_plan.md](../simple_terrain/simple_world_placement_editor_plan.md) | Legacy: PackedScene profile library (editor disabled) |

## Code locations (today)

| Concern | Location |
| --- | --- |
| Procedural World Placement runtime | `modules/open_world_terrain/open_world_placement_*` |
| Surface sampler | `modules/simple_terrain/simple_terrain_3d.*` (`sample_surface_at_world_xz`) |
| Scene-profile editor placement | `modules/simple_terrain/simple_world_*` + editor plugin |
| Decision record | `docs/adr/0001-world-placement-area-replacement.md` |

Procedural generator placement and PackedScene profile placement share the domain idea (data-first, rebuildable) but remain separate pipelines for now.
