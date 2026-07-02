# 2D Navigation

## Scope

2D navigation agents, links, obstacles, regions, and navigation mesh/polygon resource interaction.

## Entry Points

- `NavigationAgent2D`
- `NavigationLink2D`
- `NavigationObstacle2D`
- `NavigationRegion2D`

## Flow Notes

- Navigation nodes register with NavigationServer2D.
- Regions reference navigation polygon resources from `scene/resources`.
- Agents query paths and avoidance state through server-backed navigation maps.

## Code Links

- `scene/2d/*navigation*`
- `scene/resources/navigation_polygon*`
- `servers/navigation_server_2d.*`