# World Placement Brush — Phase 1 Implementation Contract

## 1. Scope

Phase 1 implements a deterministic, preset-driven brush that projects a circle, rectangle, or ellipse vertically onto SimpleTerrain3D and places a weighted mix of:

- OpenWorldTreeGenerator3D
- OpenWorldRockGenerator3D
- OpenWorldVineGenerator3D in Bramble mode only

Creeping, Climbing, Hanging, and TreeWrap vines are Phase 2. They require their own density and support-selection/projection rules and must not share the Phase 1 tree/rock density contract.

Terrain streaming, placement streaming, regional culling, MultiMesh conversion, and editor viewport painting UI are deferred. The runtime and data APIs must remain fully scriptable and headless.

## 2. Accepted authoring semantics

One brush application is a destructive area replacement operation, not a persistent stamp.

1. Validate the preset and requested object count.
2. Reject the operation before mutation if requested_count exceeds max_objects_per_operation.
3. Find managed placement records whose anchor point lies inside the footprint.
4. Keep managed records outside the footprint; they participate in spacing checks.
5. Solve and instantiate the replacement candidates transactionally.
6. Only after successful generation, remove managed records/nodes inside the footprint.
7. Attach the replacement nodes and append authoritative placement records.

Unrelated children under the selected output parent are never deleted.

A zero-density operation may act as an area clear after preset validation. Explicit clear_generated and clear_placements APIs are also provided.

## 3. Density and weighted entries

Density is expressed as objects per 100 square metres.

requested_count = round(footprint_area * density_per_100_square_meters / 100)

The preset can contain multiple entries of the same content type. Total density belongs to the preset; positive entry weights split that total. Disabled or zero-weight entries do not participate.

Default safety limit: 500 requested objects per operation. Exceeding the limit produces MAX_OBJECTS_EXCEEDED and leaves existing output untouched.

Phase 1 Bramble entries participate in this shared total density. Phase 2 will introduce independent vine density/settings for non-Bramble vine workflows.

## 4. Data model

### OpenWorldPlacementBrushEntry

A weighted source entry with stable ID, enabled state, content kind, weight, scale range, random yaw, normal alignment, surface offset, spacing override, and one category-matching generator profile/request.

Vine validation accepts MODE_BRAMBLE only and returns VINE_MODE_PHASE2 for every other mode.

### OpenWorldPlacementBrushPreset

A reusable resource containing stable ID, display name, shape, size, footprint yaw, density per 100 square metres, minimum spacing, height/slope filters, maximum objects per operation, and a weighted entry array.

Circle uses size.x as diameter. Rectangle uses full width/depth. Ellipse uses size multiplied by 0.5 as radii.

### OpenWorldPlacementData

This is the authoritative placement result. It stores parallel arrays of stable IDs, source entries, world positions, rotations, scales, terrain normals, seeds, and spacing radii.

Generated scene nodes are saved derived output, but placement data remains sufficient to clear and rebuild them deterministically. Data validation rejects array length mismatches, missing entries/IDs, duplicate IDs, and invalid transforms.

### OpenWorldPlacementBrush3D

Scriptable coordinator properties are terrain_path, output_parent_path, active_preset, placement_data, and default_seed.

Public operations are preview_brush, apply_brush, rebuild_generated, clear_generated, clear_placements, and get_generation_report.

## 5. Terrain projection

SimpleTerrain3D exposes the read-only method sample_surface_at_world_xz(world_position, max_distance = 100000.0).

Successful samples contain success, position, normal, height, and tile_cell. Failed samples contain success = false and error_code.

The brush uses this API rather than physics collision or viewport picking. Projection is always vertical, so scripts and headless tests produce the same result as editor usage.

## 6. Determinism and placement rules

The solver derives independent deterministic random streams for footprint position, weighted entry selection, and transform/generator seed. The same preset, center, and seed reproduce the same semantic placements.

Spacing is checked against existing managed placements outside the replacement footprint and candidates already accepted in the current operation.

Anchor-point footprint containment defines replacement ownership. Objects whose meshes cross the boundary but whose anchors are outside survive.

Structured reports include requested, accepted, and replacement counts plus missing-surface, height, slope, spacing, and generator failure counts.

## 7. Generated scene ownership

The brush creates one dedicated generated root beneath the selected output parent and tags that root and every generated child with placement ownership metadata.

clear_generated removes only children owned by this brush. Area replacement removes only children whose stable placement IDs are selected through authoritative placement records.

Generation is transactional: candidates are created first, and prior valid output is not deleted when validation or generator creation fails.

## 8. Phase 1 verification

Required automated coverage:

- multiple weighted entries and maximum-count rejection
- Bramble accepted; all other vine modes rejected with VINE_MODE_PHASE2
- area replacement keeps exactly one authoritative record for a repeated footprint
- placement arrays validate after replacement
- tests build through scripts/build.ps1 with tests=yes
- agent docs expose the four placement classes and SimpleTerrain3D sampler

## 9. Phase 2

Phase 2 may add independent density/settings for Creeping, Climbing, Hanging, and TreeWrap, explicit support selection, support-aware anchors, editor viewport controls, and undo/redo integration.

Non-Bramble modes must not be silently converted to Bramble.

## 10. Deferred scalability work

Streaming and culling remain separate systems. A future backend may partition authoritative placements into 128 m or 256 m cells, restrict each cell to a small deterministic variant palette, and generate tile-local MultiMesh or RID output.

Godot scene deletion does not imply arena/block deallocation. Cell ownership must explicitly release cell-owned nodes/resources while allowing shared profiles, meshes, materials, and textures to remain referenced. This future backend consumes the same placement records and must not require repainting the world.