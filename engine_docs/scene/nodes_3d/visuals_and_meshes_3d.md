# 3D Visuals And Meshes

## Scope

Visual instances, mesh instances, multimesh instances, importer mesh instances, labels, sprites, decals, and occluders.

## Entry Points

- `VisualInstance3D`
- `GeometryInstance3D`
- `MeshInstance3D`
- `MultiMeshInstance3D`
- `ImporterMeshInstance3D`
- `Label3D`
- `Sprite3D`
- `Decal`
- `OccluderInstance3D`

## Flow Notes

- Visual instances register renderable objects with RenderingServer.
- Mesh instances reference `Mesh` resources and material overrides.
- Occluders, decals, sprites, and labels all depend on rendering-server instance state.

## Code Links

- `scene/3d/visual_instance_3d.*`
- `scene/3d/mesh_instance_3d.*`
- `scene/3d/multimesh_instance_3d.*`
- `scene/3d/importer_mesh_instance_3d.*`
- `scene/3d/label_3d.*`
- `scene/3d/sprite_3d.*`
- `scene/3d/decal.*`
- `scene/3d/occluder_instance_3d.*`