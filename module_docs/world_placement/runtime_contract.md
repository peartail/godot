# World Placement — Phase 1 Runtime Contract

Moved from `module_docs/open_world_terrain/world_placement_brush_implementation_plan.md`. Domain name and ClassDB types use **World Placement** (`OpenWorldPlacement*`, `preview_placement` / `apply_placement`).

## 1. Scope

Phase 1 + Phase 2a implement a deterministic, preset-driven apply that projects a circle, rectangle, or ellipse vertically onto SimpleTerrain3D and places:

- OpenWorldTreeGenerator3D / OpenWorldRockGenerator3D / Bramble vines via **primary** density
- Creeping vines via independent **vine** density (`vine_density_per_100_square_meters`)

Climbing, Hanging, and TreeWrap vines remain deferred and return `VINE_MODE_PHASE2`. They must not share the primary tree/rock/Bramble density contract; when enabled they join the vine density pool.

Terrain streaming, placement streaming, regional culling, MultiMesh conversion, and editor viewport UI are deferred. The runtime and data APIs must remain fully scriptable and headless.

## 2. Accepted authoring semantics

One apply is a destructive **area replacement** operation, not a persistent layered stamp. Continuous drag strokes are rejected; authoring is click-to-apply only.

1. Validate the preset and requested object count.
2. Reject the operation before mutation if requested_count exceeds max_objects_per_operation.
3. Find managed placement records whose anchor point lies inside the footprint.
4. Keep managed records outside the footprint; they participate in spacing checks.
5. Solve and instantiate the replacement candidates transactionally.
6. Only after successful generation, remove managed records/nodes inside the footprint.
7. Attach the replacement nodes and append authoritative placement records.

Unrelated children under the selected output parent are never deleted.

A zero-density operation may act as an area clear after preset validation. Explicit `clear_generated` and `clear_placements` APIs are also provided.

## 3. Density and weighted entries

Density is expressed as objects per 100 square metres. Two pools:

```
primary_count = round(footprint_area * density_per_100_square_meters / 100)
vine_count    = round(footprint_area * vine_density_per_100_square_meters / 100)
requested_count = primary_count + vine_count
```

- **Primary pool:** Tree, Rock, Vine+`MODE_BRAMBLE` — weights split `density_per_100_square_meters`.
- **Vine pool:** Vine+`MODE_CREEPING` (later non-Bramble modes) — weights split `vine_density_per_100_square_meters`.

Disabled or zero-weight entries do not participate. Default safety limit: 500 requested objects per operation (`primary_count + vine_count`). Exceeding the limit produces `MAX_OBJECTS_EXCEEDED` and leaves existing output untouched.

If primary density &gt; 0 with no weighted primary entries → `PRIMARY_ENTRIES_MISSING`. If vine density &gt; 0 with no weighted vine-pool entries → `VINE_ENTRIES_MISSING`.

## 4. Data model

### OpenWorldPlacementEntry (Placement Entry)

An entry selects Tree, Vine, or Rock generation and supplies the matching reusable profile or request template. Optional materials on the entry (`trunk_material`, `foliage_material`, `stem_material`, `preview_material`) are copied onto generated nodes during apply and rebuild.

Vine validation accepts `MODE_BRAMBLE` and `MODE_CREEPING`. Climbing / Hanging / TreeWrap return `VINE_MODE_PHASE2`. Creeping does not require `support_path` on the template; Placement auto-wires the apply terrain when empty.

### OpenWorldPlacementPreset (Placement Preset)

A reusable resource containing stable ID, display name, shape, size, footprint yaw, primary density and vine density per 100 square metres, minimum spacing, height/slope filters, maximum objects per operation, and a weighted entry array.

Circle uses `size.x` as diameter. Rectangle uses full width/depth. Ellipse uses size multiplied by 0.5 as radii.

### OpenWorldPlacementData

This is the authoritative placement result. It stores parallel arrays of stable IDs, source entries, world positions, rotations, scales, terrain normals, seeds, and spacing radii.

Generated scene nodes are saved derived output, but placement data remains sufficient to clear and rebuild them deterministically. Data validation rejects array length mismatches, missing entries/IDs, duplicate IDs, and invalid transforms.

### OpenWorldPlacement3D (Placement Coordinator)

Scriptable coordinator properties are `output_parent_path`, `active_preset`, `placement_data`, and `default_seed`. Terrain is auto-selected by vertical ray (see surface backend doc); optional `vertical_ray_origin_y` on preview/apply controls selection height.

Public operations are `preview_placement`, `apply_placement`, `find_terrain_at_world_xz`, `rebuild_generated`, `clear_generated`, `clear_placements`, and `get_generation_report`.

Doc-preferred verbs: preview / apply / rebuild / clear.

Editor UX is lock-footprint → confirm Apply (no mouse-move preview solve). Legacy SimpleWorld editor UI is disabled. See [editor_interaction.md](editor_interaction.md).

## 5. Surface projection

SimpleTerrain3D exposes `sample_surface_at_world_xz` and `get_brush_hit`.

World Placement auto-selects a terrain with a vertical ray (closest hit), then projects candidates with `sample_surface_at_world_xz`. There is no `terrain_path` property. See [surface_backends/simple_terrain_3d.md](surface_backends/simple_terrain_3d.md).

Successful samples contain `success`, `position`, `normal`, `height`, and `tile_cell`. Failed samples contain `success = false` and `error_code`.

Projection of anchors is always vertical (사영). Editor click may use a camera ray only to choose the apply center.

Details: [surface_backends/simple_terrain_3d.md](surface_backends/simple_terrain_3d.md).

## 6. Determinism and placement rules

The solver derives independent deterministic random streams for footprint position, weighted entry selection, and transform/generator seed. The same preset, center, and seed reproduce the same semantic placements.

Spacing is checked against existing managed placements outside the replacement footprint and candidates already accepted in the current operation.

Anchor-point footprint containment defines replacement ownership. Objects whose meshes cross the boundary but whose anchors are outside survive.

Structured reports include requested, accepted, and replacement counts plus missing-surface, height, slope, spacing, and generator failure counts.

## 7. Generated scene ownership

The coordinator creates one dedicated generated root beneath the selected output parent and tags that root and every generated child with placement ownership metadata.

`clear_generated` removes only children owned by this coordinator. Area replacement removes only children whose stable placement IDs are selected through authoritative placement records.

Generation is transactional: candidates are created first, and prior valid output is not deleted when validation or generator creation fails.

## 8. Phase 1 verification

Required automated coverage:

- multiple weighted entries and maximum-count rejection
- Bramble and Creeping accepted; Climbing / Hanging / TreeWrap rejected with `VINE_MODE_PHASE2`
- Independent `vine_density_per_100_square_meters` and Creeping terrain support auto-wire
- area replacement keeps exactly one authoritative record for a repeated footprint
- placement arrays validate after replacement
- tests build through `scripts/build.ps1` with `tests=yes`
- agent docs expose the four placement classes and SimpleTerrain3D sampler

## 9. Phase 2

### Placement × Vine

Phase 2a accepts Bramble (primary density) and Creeping (vine density) with SimpleTerrain3D support auto-wire. Climbing / Hanging / TreeWrap still return `VINE_MODE_PHASE2` and must not be silently converted to Bramble or Creeping.

Later Phase 2 slices may add those modes into the same vine density pool with explicit support selection / stable IDs.

See [vine_placement_phase2.md](vine_placement_phase2.md) and the design note under `docs/superpowers/specs/`.

### Other

Editor lock→confirm and undo/redo are already in Phase 1. Continuous drag strokes remain rejected.

Non-Bramble modes must not be silently converted to Bramble.

## 10. Deferred scalability work

Streaming and culling remain separate systems. A future backend may partition authoritative placements into 128 m or 256 m cells, restrict each cell to a small deterministic variant palette, and generate tile-local MultiMesh or RID output.

Godot scene deletion does not imply arena/block deallocation. Cell ownership must explicitly release cell-owned nodes/resources while allowing shared profiles, meshes, materials, and textures to remain referenced. This future backend consumes the same placement records and must not require re-applying the world.
