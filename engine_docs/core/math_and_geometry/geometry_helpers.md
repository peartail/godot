# Geometry Helpers

## Scope

2D and 3D geometry operations, intersection tests, triangulation, convex hulls, triangle meshes, and static ray casting helpers.

## Entry Points

- `Geometry2D`
- `Geometry3D`
- `Triangulate`
- `ConvexHull`
- `QuickHull`
- `TriangleMesh`
- `StaticRaycaster`

## Flow Notes

- Geometry helpers are widely used by editor tools, physics setup, navigation, importers, and procedural mesh code.
- Intersection and triangulation behavior can affect serialized resources and editor-generated geometry.
- Many helpers assume finite inputs and explicit coordinate-space handling by callers.

## Code Links

- `core/math/geometry_2d.*`
- `core/math/geometry_3d.*`
- `core/math/triangulate.*`
- `core/math/convex_hull.*`
- `core/math/quick_hull.*`
- `core/math/triangle_mesh.*`
- `core/math/static_raycaster.*`