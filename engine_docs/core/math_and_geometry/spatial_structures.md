# Spatial Structures

## Scope

Bounding volume hierarchies, dynamic BVH behavior, culling helpers, and spatial partition support structures.

## Entry Points

- `BVH`
- `BVH_Tree`
- `DynamicBVH`
- `Face3`
- `DisjointSet`

## Flow Notes

- BVH structures accelerate broad geometry queries and culling.
- Dynamic structures are sensitive to update/refit behavior and allocation patterns.
- Culling helpers are performance-critical and should be changed with downstream rendering/physics use in mind.

## Code Links

- `core/math/bvh.h`
- `core/math/bvh_tree.h`
- `core/math/bvh_*.inc`
- `core/math/dynamic_bvh.*`
- `core/math/face3.*`
- `core/math/disjoint_set.h`