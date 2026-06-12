# OpenWorldTerrain Next Development Plan

## Current Status

- `OpenWorldTerrain3D` is separated from `SimpleTerrain` as an experimental GPU displacement terrain.
- Terrain rendering uses tiled patch meshes, one height texture per tile, and GPU vertex displacement.
- Brush edits update only dirty texel regions through `ImageTexture.update_region()`.
- RD region upload is now implemented through `RenderingDevice::texture_update_region()` using staging-buffer partial uploads.
- The D3D12 `CreateResource failed 0x80070057` issue was caused by adding `CAN_COPY_TO_BIT` to normal texture creation and has been fixed.
- Height splatting supports low, mid, and high albedo textures with triplanar sampling.
- Splatting now exposes per-layer texture scale, slope high-layer strength, and debug view modes.

## Verified Before Handoff

- C++ editor build passed:
  - `scons platform=windows target=editor dev_build=yes tools=yes module_mono_enabled=yes -j4`
- Mono glue generation passed.
- GodotSharp assemblies build passed after API additions.
- Headless editor startup passed.
- User verified scene loading and texture/RID errors were resolved before splatting quality work.
- Shader compile error from `return` in `fragment()` was fixed by using a final `if/else` albedo assignment.

## Next Priority 1: Splatting Quality

- Test `Splat Debug Mode`.
  - `Height`: confirm normalized height distribution is useful.
  - `Slope`: confirm steep surfaces are detected as expected.
  - `Weights`: confirm low/mid/high weights blend smoothly.
- Tune default splatting values.
  - `low_height`
  - `high_height`
  - `blend_width`
  - `slope_start`
  - `slope_end`
  - `slope_high_strength`
- Improve layer control if needed.
  - Add per-layer roughness.
  - Add per-layer normal textures.
  - Add per-layer color strength or tint strength.
- Consider renaming `texture_scale` to clarify it is a compatibility/global setter.

## Next Priority 2: Material Feature Expansion

- Add normal map support for low/mid/high layers.
- Add roughness support for low/mid/high layers.
- Add optional ORM texture support only after roughness/normal workflow is stable.
- Keep built-in material usable without requiring every texture slot.
- Add fallback behavior:
  - Missing normal texture should use flat normal.
  - Missing roughness texture should use scalar roughness.
  - Missing albedo texture should use layer color.

## Next Priority 3: Brush UX And Editing

- Add brush falloff curve.
- Add brush cursor preview in 3D viewport.
- Add flatten height picker from terrain hit point.
- Add brush spacing controls to avoid too many updates while dragging.
- Confirm undo/redo still stores only changed indices and before/after values.
- Profile brush release hitch on 2048 and 4096 heightmaps.

## Next Priority 4: Terrain Data And Persistence

- Confirm `OpenWorldTerrainData` saves height data reliably in scenes/resources.
- Add import/export helpers for heightmaps.
  - PNG/R16/RF export can be considered.
  - Import should normalize data into `0.0..1.0`.
- Add explicit dirty-state handling for edited height data.
- Consider separating editor-only brush deltas from runtime terrain data.

## Next Priority 5: Rendering Scale

- Keep current tiled patch rendering as the stable baseline.
- Add tile bounds validation for frustum culling.
- Investigate quadtree LOD or clipmap terrain after material/brush workflows are stable.
- Add collision proxy later, not in the next immediate step.
- Avoid reintroducing global texture creation usage flags for region upload.

## Important Constraints

- Do not modify `SimpleTerrain` for OpenWorldTerrain experiments.
- Do not add `CAN_COPY_TO_BIT` to general texture creation again.
- Keep `ImageTexture.update_region()` limited to uncompressed, mipmap-free 2D region images unless the RD implementation is expanded carefully.
- Keep height data normalized in CPU storage: `0.0..1.0`.
- The initial OpenWorldTerrain state should remain flat unless the user explicitly clicks Random.

## Suggested Next Session Start

1. Open a test scene with `OpenWorldTerrain3D`.
2. Assign low/mid/high albedo textures with `Repeat`, `Filter`, and `Mipmaps` enabled.
3. Use `Splat Debug Mode = Weights` to inspect layer distribution.
4. Tune defaults in C++ if the current blend/slope defaults feel poor.
5. Implement normal/roughness texture support after the weight distribution is visually good.

## Proposal: Tile Array Based Terrain Generation

### Problem To Fix

The current sparse-grid experiment still treats `OpenWorldTerrain3D` as one square terrain data domain.

- `OpenWorldTerrainData.world_size` describes the full square terrain size.
- `heightmap_resolution` describes the full square height/layer data resolution.
- `tile_size` subdivides that full square into render tiles.
- `active_grid_cells` decides which subdivisions are rendered.
- Empty-cell `Create` currently works inside that full square domain, so it feels like creating part of a pre-existing square world instead of creating one independent tile.

The intended behavior is different:

- The terrain should start as an empty tile grid.
- Clicking one empty grid cell should offer creation of exactly one tile-sized terrain unit.
- Tile size rules should remain stable.
- The world should grow as a sparse array of tiles, not as one pre-sized square.
- `PatchResolution` is no longer meaningful and should be removed.

### Target Mental Model

`OpenWorldTerrain3D` should represent a sparse tile map.

```text
OpenWorldTerrain3D
  tile_world_size: meters per tile edge
  tile_resolution: height vertices per tile edge
  tiles:
    (0, 0) -> OpenWorldTerrainTileData
    (1, 0) -> OpenWorldTerrainTileData
    (0, 1) -> OpenWorldTerrainTileData
```

Each tile is a self-contained terrain patch. The node itself owns an expandable tile array, not one square `world_size`.

### Proposed Public Properties

Replace or reinterpret the current size properties:

- Remove `patch_resolution`.
- Keep `tile_size`, but clarify whether it means render subdivision or terrain tile resolution.
- Add `tile_world_size`.
  - World-space size of one terrain tile in meters.
  - Example: `256.0`.
- Add `tile_resolution`.
  - Vertex resolution of one tile heightmap.
  - Example: `257` vertices, `256` quads.
- Replace `world_size` with a derived concept.
  - `world_size` should not be an editable scalar for sparse terrain.
  - If needed, expose read-only `used_world_bounds` or `used_tile_rect`.
- Replace `active_grid_cells` with tile ownership data.
  - Better names:
    - `created_tiles`
    - `tile_cells`
    - `terrain_tiles`
  - The important point: a cell means an actual tile exists, not just "render this part of a global heightmap".

Recommended naming:

```text
tile_world_size
tile_resolution
created_tile_cells
```

### Data Model Proposal

Current `OpenWorldTerrainData` stores one large square:

```text
heightmap_resolution
world_size
height_data
layer_data
```

Proposed structure:

```text
OpenWorldTerrainData
  tile_world_size
  tile_resolution
  HashMap<Vector2i, OpenWorldTerrainTileData>

OpenWorldTerrainTileData
  cell: Vector2i
  height_data: PackedFloat32Array
  layer_data: PackedColorArray
```

Notes:

- `OpenWorldTerrainTileData` can start as an internal struct first.
- It can become a Resource later if per-tile streaming or separate file persistence becomes important.
- Initial implementation can serialize tile cells and arrays inside `OpenWorldTerrainData`.

### Coordinate Rules

Tile coordinates should be integer grid cells:

```text
cell = floor(local_xz / tile_world_size)
tile_origin = Vector3(cell.x * tile_world_size, 0, cell.y * tile_world_size)
local_in_tile = local_xz - tile_origin.xz
```

This removes the current `-world_size * 0.5` centered-square assumption.

Recommended anchor:

- Cell `(0, 0)` occupies local X/Z range:
  - `x: 0 .. tile_world_size`
  - `z: 0 .. tile_world_size`
- Negative cells are allowed:
  - `(-1, 0)`, `(0, -1)`, etc.

Why this is better:

- The grid can grow in every direction.
- No precomputed max cell count is needed.
- Create button can appear anywhere on the infinite editor grid.

### Create Button Behavior

The editor should treat an empty grid cell as a possible tile location.

Flow:

1. User selects `OpenWorldTerrain3D`.
2. User switches to OpenWorldTerrain `Edit` mode.
3. User clicks an empty cell in the 3D view.
4. Editor shows `[Create]` at that cell center.
5. User clicks `[Create]`.
6. A new tile is created at that cell only.

Important correction:

- Create must not create a full world-sized terrain.
- Create must add exactly one tile entry:

```text
create_tile(cell)
```

Undo/redo should record tile creation and deletion:

```text
Create OpenWorld Terrain Tile
  do: add tile at cell
  undo: remove tile at cell
```

### Rendering Rules

Current rendering loops over the global heightmap and skips inactive cells.

That should change to:

```text
for each tile in terrain_data.tiles:
  build/render tile mesh at tile.cell * tile_world_size
```

Each render tile:

- Uses one tile mesh.
- Uses one tile height texture.
- Uses one tile layer texture.
- Has a transform offset based on tile cell.

This means the current `TerrainTile` runtime struct should probably contain:

```text
Vector2i cell
RID instance
Ref<ArrayMesh> mesh
Ref<ImageTexture> height_texture
Ref<ImageTexture> layer_texture
```

### Brush Rules

Brush editing must query affected tiles by bounds.

For a brush at world/local position:

```text
brush_min_cell = floor((center_xz - radius) / tile_world_size)
brush_max_cell = floor((center_xz + radius) / tile_world_size)
```

Then:

- Skip cells that do not have a tile.
- Apply brush into each affected tile's local coordinates.
- Record undo per tile.

This naturally supports brushing across tile borders.

Border handling:

- Adjacent tiles should share edge heights visually.
- First implementation can accept duplicated edge vertices per tile.
- Brush should update both neighboring tile edges when the brush overlaps both tiles.
- Later improvement: add edge synchronization helpers if seams appear.

### Picking Rules

There are two picking modes:

1. Existing tile hit
   - Raycast/march against created tile height data.
   - Used for brush painting.

2. Empty grid hit
   - Intersect ray with terrain edit plane.
   - Compute infinite grid cell.
   - If tile does not exist, show `[Create]`.

The current `_get_grid_cell_at_mouse()` should be changed to use:

```text
cell = floor(local_plane_hit.xz / tile_world_size)
center = (cell + Vector2(0.5, 0.5)) * tile_world_size
```

No `world_size`, no `heightmap_resolution`, no max cell clamp.

### API Migration Plan

Phase 1: Rename and remove confusing properties.

- Remove `patch_resolution`.
- Keep compatibility only if needed for old scenes, but hide it from the Inspector.
- Decide whether `tile_size` means:
  - tile vertex/quads resolution, or
  - render chunk subdivision.

Recommended decision:

- Rename current `tile_size` to `tile_resolution` if it means tile grid resolution.
- Later add `render_chunk_size` only if a single tile needs subchunks.

Phase 2: Tile creation data.

- Add tile existence map.
- Add `create_tile(cell)`.
- Add `remove_tile(cell)`.
- Add `has_tile(cell)`.
- Replace `active_grid_cells` usage with real tile data.

Phase 3: Rendering from tile data.

- Rebuild renderer to iterate existing tiles.
- Place each tile by cell offset.
- Remove full-square iteration.

Phase 4: Editor create workflow.

- Update empty-grid click to infinite grid cell calculation.
- Show `[Create]` at one tile center.
- Undo/redo tile creation.

Phase 5: Brush/pick conversion.

- Convert brush hit and brush edit from global heightmap coordinates to per-tile coordinates.
- Support multi-tile brush overlap.
- Keep undo deltas grouped by tile.

Phase 6: Clean documentation and Inspector.

- Update class docs.
- Update Settings dock property grouping.
- Remove or deprecate `world_size` as user-editable terrain size.
- Add clear labels:
  - Tile World Size
  - Tile Resolution
  - Created Tiles

### Compatibility Notes

Existing scenes using the current sparse-grid experiment may not migrate perfectly.

Suggested compatibility behavior:

- If old `active_grid_cells` exists and no tile data exists:
  - Create flat tiles for those cells.
- If old `world_size` and `heightmap_resolution` exist:
  - Do not try to perfectly split old global height data in the first pass.
  - Prefer a simple migration warning or flat generated tiles.

This module is still experimental, so correctness and clean future structure are more important than preserving every interim scene.

### Recommended Next Implementation Session

Start with the smallest solid slice:

1. Remove `patch_resolution` from API/property/docs.
2. Add `tile_world_size` and `tile_resolution` naming decisions.
3. Change empty grid picking to infinite tile cells.
4. Make `[Create]` create exactly one tile cell.
5. Keep tile data initially flat.
6. Rebuild rendering from created tile cells only.
7. Build Mono editor and verify:
   - editor opens,
   - empty click shows `[Create]`,
   - create adds one tile,
   - neighboring empty cells remain empty,
   - selecting other objects works in Select mode.
