# OpenWorldTerrain Architecture

## 목적

`OpenWorldTerrain`은 기존 `SimpleTerrain`과 분리된 실험형 대형 지형 모듈이다.

핵심 방향은 CPU에서 모든 지형 메시를 직접 재생성하는 방식이 아니라, CPU에는 정규화된 height/layer 데이터를 보관하고 GPU shader에서 height texture를 샘플링해 vertex displacement로 렌더링하는 것이다.

## 모듈 위치

- 모듈: `modules/open_world_terrain`
- 노드: `OpenWorldTerrain3D`
- 데이터 리소스: `OpenWorldTerrainData`
- 레이어 리소스: `OpenWorldTerrainLayer`
- 에디터 플러그인: `modules/open_world_terrain/editor`
- 공식 class doc XML: `modules/open_world_terrain/doc_classes`

## 주요 구조

`OpenWorldTerrain3D`

- `MeshInstance3D` 기반 노드.
- tiled patch mesh를 생성한다.
- tile마다 height texture와 layer texture를 가진다.
- built-in shader material에서 height texture를 샘플링해 vertex displacement를 수행한다.
- height brush와 layer brush API를 제공한다.
- 에디터 toolbar와 gizmo에서 선택/편집된다.

`OpenWorldTerrainData`

- `height_data`: `0.0..1.0` 정규화 높이 배열.
- `layer_data`: RGBA 수동 splat layer 데이터.
- `world_size`, `height_scale`, `heightmap_resolution`을 저장한다.
- CPU 편집, undo/redo, picking, texture 갱신의 원본 데이터 역할을 한다.

`OpenWorldTerrainLayer`

- terrain material layer 하나를 표현하는 리소스.
- albedo, normal, roughness, AO, parallax texture를 묶는다.
- tint, texture scale, scalar roughness를 가진다.
- `generation_role`과 `terrain_feature`로 향후 자동 생성에서 높이 생성 레이어와 surface-only 레이어를 분리한다.

## 렌더링 흐름

1. `OpenWorldTerrainData.height_data`에 정규화 높이를 저장한다.
2. `OpenWorldTerrain3D`가 terrain을 tile patch mesh로 나눈다.
3. 각 tile은 자기 영역의 height image와 layer image를 생성한다.
4. tile별 `ImageTexture`가 GPU로 업로드된다.
5. vertex shader가 tile-local height texture를 샘플링한다.
6. `VERTEX.y += height * height_scale` 방식으로 표면 높이를 만든다.
7. fragment shader가 height/slope/painted layer weight를 이용해 material을 splatting한다.

## Texture Update 전략

초기 구현은 전체 height texture를 업로드했지만, 현재 구조는 tile + dirty region 기반이다.

- terrain 전체가 바뀌는 `Flat`, `Random`, `Rebuild`는 tile texture 전체를 갱신한다.
- brush stroke는 변경된 texel bounds만 계산한다.
- tile과 dirty bounds가 겹치는 영역만 patch image로 만든다.
- `ImageTexture.update_region()`을 통해 GPU sub-region upload를 수행한다.

이 구조는 전체 heightmap 업로드보다 작은 brush 입력 지연을 목표로 한다.

## Material 전략

현재 built-in material은 low/mid/high 3개 렌더링 슬롯을 사용한다.

- 자동 weight: height band + slope.
- 수동 weight: `OpenWorldTerrainData.layer_data`.
- 수동 paint alpha가 높을수록 자동 weight보다 painted layer weight가 우선된다.
- `terrain_layers` 배열의 첫 3개 enabled material layer가 low/mid/high 슬롯에 매핑된다.

장기 목표는 3채널 고정이 아니라 N-layer splat map 또는 texture array 구조로 확장하는 것이다.

## SimpleTerrain과의 차이

`SimpleTerrain`은 기존 편집 중심 terrain이다.

- CPU에서 chunk mesh vertex 자체를 재생성한다.
- 비교적 단순하고 안정적인 편집에 적합하다.

`OpenWorldTerrain`은 실험형 open-world terrain이다.

- mesh는 평면 patch grid에 가깝게 유지한다.
- height 변화는 texture와 shader displacement로 표현한다.
- tile, region upload, future LOD/streaming을 고려한다.

## 현재 제한

- 렌더링 material slot은 아직 low/mid/high 3개가 기본이다.
- collision proxy는 아직 없다.
- quadtree LOD, clipmap, streaming은 아직 구현되지 않았다.
- layer painting은 height와 독립된 surface mask로 시작했으며, N-layer painting은 다음 구조 개선이 필요하다.
- parallax는 재질 디테일용 offset이며 실제 terrain height나 picking에는 영향을 주지 않는다.

