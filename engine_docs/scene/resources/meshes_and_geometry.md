# Meshes And Geometry

## Scope

Mesh resources, primitive/importer/immediate meshes, mesh data tools, surface tools, multimesh, mesh textures, and curve resources.

## Entry Points

- `Mesh`
- `ArrayMesh`
- `PrimitiveMesh`
- `ImporterMesh`
- `ImmediateMesh`
- `MultiMesh`
- `SurfaceTool`
- `MeshDataTool`
- `Curve`

## Flow Notes

- Mesh resources store renderable geometry and surface/material metadata.
- SurfaceTool and MeshDataTool are helper APIs for creating or inspecting mesh data.
- Curves are used by paths, animation, and curve texture resources.

## Code Links

- `scene/resources/mesh.*`
- `scene/resources/primitive_meshes*`
- `scene/resources/importer_mesh*`
- `scene/resources/immediate_mesh.*`
- `scene/resources/multimesh.*`
- `scene/resources/surface_tool.*`
- `scene/resources/mesh_data_tool.*`
- `scene/resources/curve.*`