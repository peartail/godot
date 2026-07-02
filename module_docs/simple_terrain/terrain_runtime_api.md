# SimpleTerrain Runtime API

## Main Classes

`SimpleTerrainData`

- 저장 가능한 heightfield resource.
- grid 설정과 height data를 저장한다.

`SimpleTerrain3D`

- 지형을 렌더링하고 편집하는 scene node.
- 내부에서 `SimpleTerrainData`를 만들거나 외부 리소스를 할당받는다.

## Create Terrain

```gdscript
var terrain := SimpleTerrain3D.new()
terrain.grid_size = 128
terrain.cell_size = 1.0
terrain.chunk_size = 32
terrain.reset_flat_terrain()
add_child(terrain)
```

## Use Data Resource

```gdscript
var data := SimpleTerrainData.new()
data.resize(128, 1.0, true)
data.set_height(64, 64, 5.0)

var terrain := SimpleTerrain3D.new()
terrain.simple_terrain_data = data
add_child(terrain)
```

## Random Terrain

```gdscript
terrain.random_height_scale = 8.0
terrain.random_frequency = 0.035
terrain.random_octaves = 4
terrain.randomize_seed()
```

초기 terrain은 flat이다.

## Brush API

```gdscript
terrain.apply_brush(
    Vector3(0.0, 0.0, 0.0),
    5.0,
    0.25,
    SimpleTerrain3D.BRUSH_RAISE
)
```

## Brush Delta API

```gdscript
var delta := terrain.apply_brush_with_delta(
    world_position,
    4.0,
    0.2,
    SimpleTerrain3D.BRUSH_SMOOTH
)
```

반환 dictionary:

- `indices`
- `before`
- `after`

## Height Patch

```gdscript
terrain.apply_height_patch(indices, heights)
```

undo/redo 또는 부분 높이 복원에 사용한다.

## Picking

```gdscript
var hit := terrain.get_brush_hit(ray_origin, ray_direction)
if not hit.is_empty():
    terrain.apply_brush(hit.position, 4.0, 0.2, SimpleTerrain3D.BRUSH_SMOOTH)
```

## Debug Lines

```gdscript
terrain.show_chunk_gizmos = true
var lines := terrain.get_chunk_debug_lines()
```

## Triplanar Material

```gdscript
terrain.use_builtin_triplanar_material = true
terrain.triplanar_low_height = 1.5
terrain.triplanar_high_height = 8.0
terrain.triplanar_blend_width = 2.0
terrain.triplanar_texture_scale = 0.08
```

## Custom Material

```gdscript
terrain.terrain_material = preload("res://terrain_material.tres")
```
