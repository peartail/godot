# 3D Navigation And XR

## Scope

3D navigation agents, links, obstacles, regions, and XR scene nodes.

## Entry Points

- `NavigationAgent3D`
- `NavigationLink3D`
- `NavigationObstacle3D`
- `NavigationRegion3D`
- XR node classes

## Flow Notes

- Navigation nodes register with NavigationServer3D and reference navigation mesh resources.
- Agents query path and avoidance state through server-backed maps.
- XR nodes bridge scene transforms with XR server tracking spaces.

## Code Links

- `scene/3d/*navigation*`
- `scene/3d/xr_nodes*`
- `scene/resources/navigation_mesh.*`
- `servers/navigation_server_3d.*`
- `servers/xr/`