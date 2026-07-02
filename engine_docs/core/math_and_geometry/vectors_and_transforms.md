# Vectors And Transforms

## Scope

Fundamental math value types for coordinates, transforms, rotations, colors, rectangles, planes, and projections.

## Entry Points

- `Vector2`, `Vector3`, `Vector4`
- `Vector2i`, `Vector3i`, `Vector4i`
- `Transform2D`, `Transform3D`
- `Basis`, `Quaternion`, `Projection`
- `Rect2`, `AABB`, `Plane`, `Color`

## Flow Notes

- These types are Variant values and are exposed directly to scripts.
- Approximate comparison and finite checks are important for editor and physics robustness.
- Transform and Basis changes can affect rendering, physics, animation, and import behavior.

## Code Links

- `core/math/vector*.h`
- `core/math/transform_2d.*`
- `core/math/transform_3d.*`
- `core/math/basis.*`
- `core/math/quaternion.*`
- `core/math/projection.*`
- `core/math/color.*`
- `core/math/aabb.*`
- `core/math/rect2.*`
- `core/math/plane.*`