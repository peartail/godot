# Pathfinding

## Scope

Graph and grid pathfinding helpers exposed to scripts and used by tools.

## Entry Points

- `AStar`
- `AStarGrid2D`
- `AStar::get_point_path()`
- `AStarGrid2D::get_point_path()`

## Flow Notes

- `AStar` manages explicit graph points and weighted connections.
- `AStarGrid2D` builds path queries over a configured grid region.
- Compatibility include files preserve behavior for older API shapes.

## Code Links

- `core/math/a_star.h`
- `core/math/a_star.cpp`
- `core/math/a_star.compat.inc`
- `core/math/a_star_grid_2d.*`
- `core/math/a_star_grid_2d.compat.inc`