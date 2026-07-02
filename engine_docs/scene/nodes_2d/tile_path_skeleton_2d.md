# 2D Tile Path Skeleton

## Scope

Tile maps, paths, path follow behavior, skeletons, 2D bones/modifications, and animated sprite helpers.

## Entry Points

- `TileMap`
- `TileMapLayer`
- `Path2D`
- `Skeleton2D`
- `AnimatedSprite2D`

## Flow Notes

- Tile map runtime behavior depends heavily on `TileSet` resources.
- Path nodes wrap `Curve2D` resources and expose path-following data.
- Skeleton2D uses resources and modification stacks from `scene/resources`.

## Code Links

- `scene/2d/tile_map.*`
- `scene/2d/tile_map_layer.*`
- `scene/2d/path_2d.*`
- `scene/2d/skeleton_2d.*`
- `scene/2d/animated_sprite_2d.*`
- `scene/resources/tile_set*`
- `scene/resources/skeleton_modification_2d*`