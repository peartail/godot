# 2D Visuals

## Scope

2D drawing nodes for sprites, polygons, meshes, lines, canvas effects, lights, occluders, and multimesh instances.

## Entry Points

- `Sprite2D`
- `Polygon2D`
- `Line2D`
- `MeshInstance2D`
- `MultiMeshInstance2D`
- `Light2D`
- `LightOccluder2D`

## Flow Notes

- Most visual nodes submit draw commands through `CanvasItem`.
- Line and polygon helpers generate geometry before canvas submission.
- Lights and occluders interact with canvas lighting in the rendering server.

## Code Links

- `scene/2d/sprite_2d.*`
- `scene/2d/polygon_2d.*`
- `scene/2d/line_2d.*`
- `scene/2d/line_builder.*`
- `scene/2d/mesh_instance_2d.*`
- `scene/2d/multimesh_instance_2d.*`
- `scene/2d/light_2d.*`
- `scene/2d/light_occluder_2d.*`