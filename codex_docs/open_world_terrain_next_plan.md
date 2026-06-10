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
