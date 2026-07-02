# SplineMesh3D Spec

## Purpose

`SplineMesh3D` is a native 3D node that deforms a source `Mesh` along a `Path3D` / `Curve3D`.
It targets roads, rivers, cables, shoreline strips, and long water planes.

The node inherits `MeshInstance3D`, but the output `mesh` is generated and owned internally.
Input geometry must be assigned through `source_mesh`.

## Node Model

- Class: `SplineMesh3D`
- Base class: `MeshInstance3D`
- Input mesh: `source_mesh`
- Output mesh: internal `ArrayMesh` assigned to inherited `mesh`
- Path selector: `path_node`

Path lookup order:
1. Explicit `path_node`
2. First child `Path3D`
3. Parent `Path3D`

In editor builds, a new node with no usable path creates child `Path3D` named `SplinePath`.
The default curve runs from `(-1, 0, 0)` to `(1, 0, 0)`.

## Mesh Ownership

`SplineMesh3D` manages inherited `mesh`.
Direct assignment is hidden from the Inspector and rejected with a warning.
Use `spline.source_mesh = plane_mesh`, not `spline.mesh = plane_mesh` or `spline.set("mesh", plane_mesh)`.

`MeshInstance3D::set_mesh()` also defensively handles changed-signal connect/disconnect so invalid or repeated mesh assignment does not crash the editor.

## Axis Mapping

- `forward_axis`: source axis mapped to curve distance
- `up_axis`: source axis used as local up
- Width axis: perpendicular to forward and up after source-to-deform conversion

`forward_axis` and `up_axis` must be different.
For horizontal water, use `forward_axis = AXIS_X`, `up_axis = AXIS_Y`; width is local Z.

## Frame Generation

`frame_mode` controls orientation frames along the curve.

- `FRAME_PARALLEL_TRANSPORT`: default; minimizes twisting and does not require baked up vectors.
- `FRAME_CURVE_UP_VECTOR`: uses `Curve3D.sample_baked_with_rotation()` and requires `Curve3D.up_vector_enabled = true`.

`tilt_enabled` controls whether `Curve3D` tilt is applied around the path tangent.

## Rebuild Behavior

The generated mesh rebuilds when `source_mesh`, path / curve, axis, frame, stretch, UV, or debug settings change.
Thread-sensitive rebuild requests are deferred with `call_deferred()` when needed to avoid wrong-thread editor warning updates.

## Length Mapping

`stretch_to_fit` controls source length mapping:

- `true`: source forward extent is normalized and stretched to the full curve length.
- `false`: source forward units map directly to curve meters and clamp at curve length.

Curve sampling options:

- `cubic_interp`: use cubic baked curve sampling.
- `frame_interval`: spacing between cached frames for parallel transport.

## UV U Mapping

`uv_mode` controls U:

- `UV_KEEP_SOURCE`: keep source mesh U values.
- `UV_PATH_DISTANCE`: compute U from path distance.

```text
U = path_offset_meters / uv_tile_length
```

`uv_tile_length` is meters per U tile.
Smaller values repeat the texture more frequently along the path.

## UV V Mapping

`uv_v_mode` controls V independently from U:

- `UV_V_KEEP_SOURCE`: keep source mesh V values.
- `UV_V_NORMALIZED`: normalize source mesh width to `0.0 .. 1.0`.
- `UV_V_DISTANCE`: compute V from source mesh width distance in meters.

```text
V = width_distance_meters / uv_v_tile_width
```

`uv_v_tile_width` is meters per V tile.
Smaller values repeat the texture more frequently across the spline width.
Width is measured in the source-to-deform local width axis, not from final curved world space.

## UV Debug Display

`uv_debug_enabled` visualizes generated UVs with a temporary `material_override`.

- Red: fractional U
- Green: fractional V
- Blue: alternating UV tile checker

When enabled, the previous `GeometryInstance3D.material_override` is cached.
When disabled, the previous override is restored.
Generated mesh surface materials remain copied from `source_mesh`.

## Public Properties

- `source_mesh: Mesh`
- `path_node: NodePath`
- `forward_axis: Axis`
- `up_axis: Axis`
- `frame_mode: FrameMode`
- `stretch_to_fit: bool`
- `cubic_interp: bool`
- `tilt_enabled: bool`
- `frame_interval: float`
- `uv_mode: UVMode`
- `uv_tile_length: float`
- `uv_v_mode: UVVMode`
- `uv_v_tile_width: float`
- `uv_debug_enabled: bool`

## Current Limitations

- Collision generation is not part of `SplineMesh3D`.
- LOD generation is not part of `SplineMesh3D`.
- Width V mapping is based on source mesh width, not dynamic path banking width.
- Direct `mesh` assignment is intentionally blocked; use `source_mesh`.

## Verification

Verified with Windows Mono editor build, Mono glue generation, Mono assemblies build, headless GDScript UV tests for U distance / V normalized / V distance / V keep-source modes, and agent docs query for `SplineMesh3D`.
