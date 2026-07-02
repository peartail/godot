# 3D Physics Nodes

## Scope

3D physics bodies, areas, collision objects, shapes, casts, joints, soft bodies, vehicles, and velocity tracking.

## Entry Points

- `CollisionObject3D`
- `Area3D`
- `PhysicsBody3D`
- `CollisionShape3D`
- `CollisionPolygon3D`
- `RayCast3D`
- `ShapeCast3D`
- `Joint3D`
- `SoftBody3D`
- `VehicleBody3D`

## Flow Notes

- Physics nodes synchronize scene transforms with PhysicsServer3D objects.
- Shape resources live in `scene/resources` and are referenced by collision nodes.
- Cast nodes perform query-style checks during physics or idle updates.

## Code Links

- `scene/3d/*physics*`
- `scene/3d/*collision*`
- `scene/3d/*ray_cast*`
- `scene/3d/*shape_cast*`
- `scene/3d/*joint*`
- `scene/3d/soft_body_3d*`
- `scene/3d/vehicle_body_3d*`
- `servers/physics_server_3d.*`