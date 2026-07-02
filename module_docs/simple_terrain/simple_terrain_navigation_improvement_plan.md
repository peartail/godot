# SimpleTerrain3D Navigation Mesh Improvement Plan

## Goal

Improve `SimpleTerrain3D` navigation mesh generation so click-to-move paths are less jagged on terrain. The current implementation emits every walkable terrain quad as two triangle polygons, which creates a very dense corridor for `NavigationAgent3D` and can produce visible zigzag movement even when corridor funnel post-processing is enabled.

## Current State

Main implementation:

- `modules/simple_terrain/simple_terrain_3d.cpp`
- `SimpleTerrain3D::_build_chunk_navigation_mesh(int p_origin_x, int p_origin_z, int p_quad_width, int p_quad_depth)`

Current flow:

1. Build one vertex for each terrain grid point in the chunk.
2. For each terrain quad, split it into two triangles.
3. Reject triangles whose normal exceeds `navigation_max_slope`.
4. Add every accepted triangle directly to `NavigationMesh`.
5. Register one navigation region per terrain chunk.

Main limitation:

- The path corridor is built from many small triangles.
- Funnel post-processing has too many tiny portals to pass through.
- Chunk boundaries can add additional region edge connections.
- The actor follows returned path points directly, so small path oscillations become visible movement.

## Desired Direction

Keep the existing `SimpleTerrain3D` navigation API, but improve the generated `NavigationMesh` polygons.

Existing public API should continue to work:

- `navigation_enabled`
- `navigation_layers`
- `navigation_max_slope`
- `rebuild_navigation()`

New options can be added later, but the first implementation should preserve old behavior as a fallback.

## Proposed Phases

## Phase 1: Add Navigation Build Mode

Add an enum to `SimpleTerrain3D`:

```cpp
enum NavigationBuildMode {
	NAVIGATION_BUILD_TRIANGLES,
	NAVIGATION_BUILD_QUADS,
	NAVIGATION_BUILD_MERGED_RECTS,
};
```

Suggested property:

```cpp
NavigationBuildMode navigation_build_mode = NAVIGATION_BUILD_QUADS;
```

Expose to ClassDB:

- `set_navigation_build_mode()`
- `get_navigation_build_mode()`
- Inspector enum: `Triangles,Quads,Merged Rectangles`

Default recommendation:

- Use `NAVIGATION_BUILD_QUADS` if backward compatibility risk is acceptable.
- Use `NAVIGATION_BUILD_TRIANGLES` if exact current behavior must remain default.

## Phase 2: Quad Polygon Generation

Replace the unconditional two-triangle output with quad output when the quad is planar enough.

For each terrain quad:

1. Compute both triangle normals.
2. Check both triangles against `navigation_max_slope`.
3. Check normal similarity between both triangles.
4. Check the fourth point against the plane of the first triangle.
5. If valid, emit one 4-point polygon.
6. If invalid, fall back to two walkable triangles.

Suggested properties:

```cpp
real_t navigation_quad_max_normal_angle = 5.0;
real_t navigation_quad_planar_tolerance = 0.05;
```

Implementation notes:

- Keep polygon winding consistent with current triangle code.
- Only emit convex polygons.
- Preserve original chunk-local vertex positions.
- Do not change terrain render mesh generation.

Expected result:

- Polygon count drops by up to 50%.
- Funnel corridor becomes less noisy.
- Low risk compared with full contour generation.

## Phase 3: Rectangle Merge Mode

Build a walkable cell grid per chunk and merge compatible cells into larger rectangles.

Per-cell data:

```cpp
struct NavigationCell {
	bool walkable = false;
	bool used = false;
	Vector3 normal;
	Plane plane;
};
```

Compatibility checks:

- Both cells are walkable.
- Normal angle difference is below `navigation_merge_max_normal_angle`.
- Shared edge height discontinuity is below `navigation_merge_max_height_delta`.
- All rectangle vertices remain close to the seed plane within `navigation_merge_planar_tolerance`.

Suggested properties:

```cpp
real_t navigation_merge_max_normal_angle = 8.0;
real_t navigation_merge_max_height_delta = 0.25;
real_t navigation_merge_planar_tolerance = 0.15;
int navigation_merge_max_rect_size = 8;
```

Rectangle merge algorithm:

1. Iterate cells row-major.
2. Skip non-walkable or used cells.
3. Start a seed rectangle at the current cell.
4. Expand width while compatible.
5. Expand height while every cell in the new row is compatible.
6. Mark cells as used.
7. Emit one rectangular polygon from the four corner terrain vertices.

Fallback rules:

- If a rectangle fails convexity or planarity checks, fall back to quads or triangles for that area.
- Keep chunk-local merge only at first. Cross-chunk merge is a later optimization.

Expected result:

- Large flat areas become larger navigation polygons.
- Path corridors become much cleaner.
- Debug navigation mesh becomes easier to inspect.

## Phase 4: Chunk Boundary Portal Improvement

The terrain currently registers navigation per chunk. Even with merged polygons, chunk borders can create artificial portal sequences.

Possible improvements:

- Ensure boundary polygons share exact same vertex coordinates across neighboring chunks.
- Avoid adding duplicate or slightly mismatched border vertices.
- Keep `NavigationServer3D` edge connection margin compatible with `cell_size`.
- Consider larger `chunk_size` for navigation than rendering chunks.

Optional property:

```cpp
int navigation_chunk_size = 0;
```

Where `0` means use render `chunk_size`.

Expected result:

- Fewer region-to-region transitions.
- Less path wobble at chunk borders.

## Phase 5: Contour-Based Builder

If rectangle merging is insufficient, implement a terrain-specific contour builder.

High-level flow:

1. Generate walkable cell mask.
2. Flood-fill connected regions.
3. Extract boundary edges for each region.
4. Build ordered contours.
5. Simplify contours by removing collinear or near-collinear points.
6. Split contours into convex polygons.
7. Emit polygons to `NavigationMesh`.

Risks:

- Convex decomposition is required.
- Holes and obstacles require more bookkeeping.
- This is significantly more complex than rectangle merging.

Use this only after Phase 3 has been evaluated.

## Phase 6: Recast-Style Pipeline

A full Recast-like pipeline is the long-term robust solution:

```text
terrain heightfield
-> walkable span filtering
-> compact heightfield
-> erosion by agent radius
-> region partition
-> contour extraction
-> contour simplification
-> convex polygon mesh
-> optional detail mesh
```

This should be implemented as a separate builder class, not inside `SimpleTerrain3D` directly.

Suggested class:

```cpp
class SimpleTerrainNavigationBuilder {
public:
	Ref<NavigationMesh> build_chunk_navigation_mesh(...);
};
```

This phase should only be started if the project needs agent radius erosion, obstacle carving, or high-quality outdoor navigation comparable to Recast.

## Recommended First Implementation

Implement Phases 1-3 first.

Priority order:

1. Add `NavigationBuildMode`.
2. Keep current triangle mode intact.
3. Implement quad mode.
4. Implement merged rectangle mode.
5. Add tests and debug metrics.

This provides a safe migration path:

- `Triangles`: exact fallback.
- `Quads`: low-risk improvement.
- `Merged Rectangles`: stronger smoothing for flat terrain.

## Test Plan

## Build Tests

Run:

```powershell
scons platform=windows target=editor dev_build=yes module_mono_enabled=yes
```

Or the local project build command already used for this engine checkout.

## Runtime Load Tests

Run project load:

```powershell
C:\GithubProjects\godot\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --path . --quit
```

Run terrain scene:

```powershell
C:\GithubProjects\godot\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --path . --scene res://examples/Terrain/simpleTerrain.tscn --quit-after 3
```

## Behavior Tests

Use `res://examples/Terrain/simpleTerrain.tscn`.

Check:

- Navigation debug mesh is visible.
- Character starts on nearest valid navigation point.
- Clicking flat terrain creates a mostly straight path.
- Clicking hills still follows terrain height correctly.
- Steep slopes above `navigation_max_slope` are excluded.
- Chunk boundaries do not create visible path jitter.

## Metrics To Log During Development

Add temporary debug prints or editor-only stats:

- Chunk count.
- Navigation vertices per chunk.
- Navigation polygons per chunk.
- Triangle/quad/merged rectangle count.
- Rebuild time.
- Path point count before and after `NavigationAgent3D` simplification.

Expected metric improvements:

- Quad mode: polygon count roughly 50% lower than triangle mode.
- Merged rectangle mode: polygon count much lower on flat terrain.
- Path point count should drop on open areas.

## Compatibility Notes

- Do not remove existing `rebuild_navigation()` behavior.
- Keep `navigation_max_slope` semantics unchanged.
- Keep `navigation_layers` applied per chunk region.
- Avoid changing render mesh chunking.
- Keep generated vertices in `SimpleTerrain3D` local space, matching current implementation.
- Preserve fallback to triangle generation for non-planar or irregular cells.

## Open Questions

- Should `navigation_build_mode` default to `Triangles` for compatibility or `Quads` for better behavior?
- Should navigation chunk size be independent from render chunk size?
- Should object placement data eventually contribute obstacles or carved holes?
- Should agent radius be handled by terrain navmesh erosion or only by avoidance?
- Should merged rectangle mode allow non-flat but gently sloped rectangles?

## Suggested Acceptance Criteria

The change is acceptable when:

- Existing terrain scenes still load.
- `Triangles` mode reproduces current behavior.
- `Quads` mode produces valid navigation on the current sample terrain.
- `Merged Rectangles` mode reduces path zigzag on flat/open terrain.
- Headless scene execution succeeds.
- No crash occurs when painting terrain and calling `rebuild_navigation()`.
