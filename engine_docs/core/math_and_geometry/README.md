# Math And Geometry

Core numeric types, transforms, geometry algorithms, spatial structures, pathfinding, random, and expressions.

## Subtopics

- [Vectors And Transforms](vectors_and_transforms.md): vectors, rectangles, transforms, basis, projection, quaternion, color, and planes.
- [Geometry Helpers](geometry_helpers.md): 2D/3D geometry utilities, triangulation, hulls, triangle meshes, and ray helpers.
- [Spatial Structures](spatial_structures.md): BVH, dynamic BVH, broadphase structures, and culling helpers.
- [Pathfinding](pathfinding.md): `AStar`, `AStarGrid2D`, and graph/grid path queries.
- [Random](random.md): `RandomPCG` and script-facing random number generation.
- [Expression](expression.md): math expression parser/evaluator used by engine APIs.

## Global Entry Points

- Vector and transform types are Variant-supported value types.
- Geometry helpers are stateless utility APIs used by physics, rendering, editor, and tools.
- Spatial structures provide acceleration data for culling and geometry queries.

## Code Links

- `core/math/vector*.h`
- `core/math/transform_*.h`
- `core/math/geometry_2d.*`
- `core/math/geometry_3d.*`
- `core/math/a_star.*`
- `core/math/bvh*`