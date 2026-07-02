# SimpleTerrain Runtime Architecture

## 소유 구조

`SimpleTerrainData`는 persistent height data를 소유한다.

`SimpleTerrain3D`는 렌더링과 편집 동작을 소유한다.

`SimpleTerrain3D`가 담당하는 것:

- mesh generation.
- chunk lifetime.
- `RenderingServer` instance.
- brush editing.
- ray picking.
- runtime material selection.

## Height Data

`SimpleTerrainData.height_data`는 vertex마다 하나의 높이를 저장한다.

`grid_size = N`일 때:

- quad 수: `N`
- vertex 수: `N + 1`
- height 개수: `(N + 1) * (N + 1)`

인덱스:

```text
index = z * (grid_size + 1) + x
```

## 초기 상태

`SimpleTerrain3D`는 데이터가 없으면 지연 생성한다.

초기 terrain은 평평하다.

랜덤 terrain은 다음 API나 에디터 버튼을 통해서만 생성된다.

- `generate_random_terrain()`
- `randomize_seed()`

## Chunked Rendering

지형은 `chunk_size` 기준으로 여러 chunk로 나뉜다.

각 chunk는 다음을 가진다.

- terrain quad origin.
- quad width/depth.
- `ArrayMesh`.
- internal `RenderingServer` instance RID.

상속된 `MeshInstance3D.mesh`는 비워 둔다.

실제 renderable은 내부 chunk instance다.

## Partial Rebuild

브러시 편집은 영향을 받은 vertex rectangle을 계산한다.

해당 영역과 겹치는 chunk만 mesh를 다시 만든다.

이 방식으로 전체 terrain rebuild 비용을 줄인다.

## Brush Undo

브러시 중에는 변경된 vertex index와 before/after height만 수집한다.

마우스를 놓을 때 전체 height array를 저장하지 않고 delta patch만 undo action에 기록한다.

Undo/redo는 `apply_height_patch()`를 사용해 변경 vertex만 되돌린다.

## Ray Picking

`get_brush_hit()`은 2D DDA 방식으로 ray가 지나가는 grid cell만 검사한다.

전체 triangle을 모두 검사하지 않기 때문에 큰 grid에서 더 안정적이다.
