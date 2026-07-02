# 2D Physics Nodes

## Scope

2D physics bodies, areas, collision objects, collision shapes/polygons, ray/shape casts, joints, physical bones, and touch screen buttons.

## Entry Points

- `CollisionObject2D`
- `Area2D`
- `PhysicsBody2D`
- `CollisionShape2D`
- `CollisionPolygon2D`
- `RayCast2D`
- `ShapeCast2D`
- `Joint2D`

## Flow Notes

- Physics nodes synchronize scene transforms with PhysicsServer2D objects.
- Collision shape resources live under `scene/resources` and are owned/referenced by scene nodes.
- Cast nodes perform query-style physics checks during processing.

## Code Links

- `scene/2d/*physics*`
- `scene/2d/*collision*`
- `scene/2d/*ray_cast*`
- `scene/2d/*shape_cast*`
- `scene/2d/*joint*`
- `servers/physics_server_2d.*`