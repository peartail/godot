# World Placement × Vine — Phase 2

This document separates two different “Phase 2” tracks so they are not confused.

## 1. Vine *generator* roadmap (already largely implemented)

Canonical docs live under `codex_docs/`:

| Doc | Role |
| --- | --- |
| [open_world_vine_system_plan.md](../../codex_docs/open_world_vine_system_plan.md) | Original phased plan |
| [open_world_vine_system_current_status.md](../../codex_docs/open_world_vine_system_current_status.md) | What exists today |
| [open_world_vine_agent_api.md](../../codex_docs/open_world_vine_agent_api.md) | Public API |
| [open_world_vine_bramble_guide.md](../../codex_docs/open_world_vine_bramble_guide.md) | Bramble authoring |
| [open_world_vine_test_cases.md](../../codex_docs/open_world_vine_test_cases.md) | Manual TC matrix |

In the **generator plan**:

- **Phase 1** — static bake MVP (Climbing / Creeping / Hanging)
- **Phase 2** — surface attachment (anchors, projection, support query)
- **Phase 3** — visual quality / LOD / decorations
- **Phase 4** — world batching / optional gameplay

Current status doc reports Creeping, Climbing, Hanging, TreeWrap, and Bramble modes already available on `OpenWorldVineGenerator3D`. Remaining “detail” work maps mainly to **generator Phase 3**.

## 2. World Placement *integration* Phase 2

Design note: [2026-07-23-placement-creeping-vine-density-design.md](../../docs/superpowers/specs/2026-07-23-placement-creeping-vine-density-design.md).

### Phase 2a (implemented) — Creeping + vine_density

- Preset `vine_density_per_100_square_meters` for **non-Bramble** vine entries (Creeping now; future modes share this pool).
- Primary density (`density_per_100_square_meters`) still covers Tree / Rock / **Bramble**.
- Entry validation accepts `MODE_BRAMBLE` and `MODE_CREEPING`; Climbing / Hanging / TreeWrap return `VINE_MODE_PHASE2`.
- Creeping support: use template `support_path` when set; otherwise auto-wire to the apply-selected `SimpleTerrain3D`.
- Creeping generate runs **after** the vine node is parented so relative `NodePath` resolves.
- Spacing remains shared across both pools. `max_objects_per_operation` checks `primary_count + vine_count`.

### Later slices

1. Climbing / Hanging / TreeWrap placement (same vine density pool; support refs / stable IDs).
2. Optional editor dock Creeping quick-add.
3. Tree-to-tree bridge vines (generator feature first — not a Placement-only change).

## Practical recommendation for “덩굴 디테일”

| If the goal is… | Start from… |
| --- | --- |
| Better vine look (thorns, leaves, materials, LOD) | Generator Phase 3 in `codex_docs/` |
| More placement vine modes | Later slices above |
| Tree↔tree bridges | New generator mode, then Placement |
