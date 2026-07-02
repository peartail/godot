# Navigation Resources

## Scope

Navigation mesh, navigation polygon, source geometry data, and polygon path finder resources.

## Entry Points

- `NavigationMesh`
- `NavigationPolygon`
- `NavigationMeshSourceGeometryData2D`
- `NavigationMeshSourceGeometryData3D`
- `PolygonPathFinder`

## Flow Notes

- Navigation resources are referenced by navigation region nodes.
- Source geometry data captures scene geometry for navigation baking.
- Runtime path queries go through navigation servers, not the resource alone.

## Code Links

- `scene/resources/navigation_mesh.*`
- `scene/resources/navigation_polygon*`
- `scene/resources/navigation_mesh_source_geometry_data_*`
- `scene/resources/polygon_path_finder*`
- `servers/navigation_server_2d.*`
- `servers/navigation_server_3d.*`