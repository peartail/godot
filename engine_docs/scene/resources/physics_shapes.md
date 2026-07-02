# Physics Shapes

## Scope

2D and 3D shape resources, collision geometry data, and physics material resources.

## Entry Points

- `Shape2D`
- `Shape3D`
- `CircleShape2D`
- `RectangleShape2D`
- `ConvexPolygonShape2D`
- `BoxShape3D`
- `SphereShape3D`
- `ConvexPolygonShape3D`
- `PhysicsMaterial`

## Flow Notes

- Shape resources are referenced by collision nodes and registered with physics servers.
- Concave, convex, and height-map shapes have different runtime and editor use cases.
- Physics materials carry friction/bounce-style properties used by bodies and shapes.

## Code Links

- `scene/resources/*shape_2d*`
- `scene/resources/*shape_3d*`
- `scene/resources/physics_material.*`
- `servers/physics_server_2d.*`
- `servers/physics_server_3d.*`