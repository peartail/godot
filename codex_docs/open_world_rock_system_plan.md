# Agent-First OpenWorld 바위 시스템 구현 계획

## 목적

카툰 로우폴리 바위를 Godot에서 `Profile + Request + seed`로 결정론적으로 생성하고, 게임 실행 전에 정적 LOD·Collision 리소스로 Bake한다.

이 문서는 새 세션의 구현 기준이다. Tree/Vine 변경은 보존하며 Rock은 독립 시스템으로 추가한다.

## 핵심 원칙

- 마우스 조형 없이 `.tres`, GDScript, C#과 headless 명령으로 제작한다.
- 같은 Profile, Request와 seed는 같은 topology, vertex와 report를 생성한다.
- `validate`, `generate`, `bake`, `report`를 분리한다.
- 원본은 텍스트 리소스이며 mesh와 collision은 재생성 가능한 파생 결과다.
- 런타임 변형보다 사전 생성·Bake를 우선한다.
- Markdown이 180줄을 넘으면 설계/API/테스트 문서로 분리한다.

## 1차 지원 형태

| Mode | 특징 | 용도 |
|---|---|---|
| `BOULDER` | 둥글고 불규칙한 덩어리 | 필드·산악 바위 |
| `SLAB` | 넓고 낮으며 층리가 강함 | 판석·길·절벽 파편 |
| `SHARD` | 길고 방향성이 강한 쐐기 | 날카로운 암석·판타지 지형 |

후속 후보는 `CLIFF_CHUNK`, `PEBBLE_CLUSTER`, `CRYSTAL`이다.

## 공개 클래스

### `OpenWorldRockGenerationProfile`

- 크기 variation, point count와 radial distribution
- roughness, asymmetry, elongation과 방향성
- bottom flatten과 ground inset
- facet 크기, strata 강도·방향
- 선택적 multi-lobe 수와 겹침
- LOD 품질, collision 품질
- Vertex Color 재질 mask 규칙

### `OpenWorldRockGenerationRequest`

- `BOULDER`, `SLAB`, `SHARD`
- seed, local position 기준 크기, 회전축
- profile
- 선택적 `explicit_points`
- gameplay용 stable ID와 tag metadata

`explicit_points`가 있으면 자동 point solver를 우회한다.

### `OpenWorldRockTopologyData`

- 정규화된 source point cloud
- hull vertex·triangle과 face group
- base plane, local bounds와 source seed
- LOD가 공유할 형태 metadata

### `OpenWorldRockVariant`

- LOD0/LOD1/LOD2 mesh와 전환 거리
- convex collision shape 또는 collision point cloud
- source mode, seed, bounds
- stable ID와 gameplay metadata

### `OpenWorldRockGenerator3D : MeshInstance3D`

```text
validate_request() -> Dictionary
generate_topology() -> OpenWorldRockTopologyData
generate_rock() -> void
get_generated_lod_mesh(lod) -> ArrayMesh
get_generation_report() -> Dictionary
create_baked_variant() -> OpenWorldRockVariant
```

Inspector 액션은 이 공개 API만 호출한다.

## 메시 알고리즘

### 기본 파이프라인

```text
seeded sphere directions
        ↓
ellipsoid scaling
        ↓
low-frequency radial noise
        ↓
mode deformation / strata
        ↓
bottom flatten
        ↓
3D convex hull
        ↓
flat normals + material masks
```

- 방향점은 Fibonacci sphere 또는 subdivided icosphere에서 생성한다.
- 점마다 별도 RandomPCG stream으로 radial scale을 계산한다.
- 바닥 점을 동일 높이에 투영하고 안정적인 지지 polygon을 만든다.
- Convex Hull triangle winding과 vertex 순서는 결정론적으로 정렬한다.
- LOD는 같은 source point 집합의 결정론적 subset으로 다시 hull을 만든다.
- Collision은 LOD2 hull 또는 별도 저밀도 hull을 사용한다.

### 형태 규칙

- `BOULDER`: 축 variation과 저주파 noise를 균등하게 적용한다.
- `SLAB`: Y축을 압축하고 strata 방향으로 점을 양자화한다.
- `SHARD`: 주축으로 늘리고 한쪽 끝 반경을 줄인다.
- Concave 표현은 초기에는 2~4개 convex lobe 겹침으로 근사한다.
- Boolean union, voxel과 Marching Cubes는 1차 범위에서 제외한다.

## Surface와 셰이더 규약

- Surface 0: 전체 rock body
- UV0: 기본 triplanar 보조 좌표 또는 안정적인 box projection
- Normal: 기본 flat shading, 선택적으로 weighted normal
- Vertex R: upward/moss mask
- Vertex G: cavity 또는 downward mask
- Vertex B: strata/material variation
- Vertex A: seed phase

재질은 기본 카툰 rock preview를 제공하되 프로젝트 shader도 같은 채널을 사용한다.

## LOD와 Collision

- LOD0: 전체 point count와 facet detail
- LOD1: point count 약 50~65%, 작은 facet 제거
- LOD2: 8~12점 저밀도 hull
- 세 LOD의 pivot과 base plane은 동일해야 한다.
- LOD triangle 수는 반드시 단조 감소해야 한다.
- collision은 기본적으로 단일 convex shape를 생성한다.
- multi-lobe는 lobe별 convex collision을 선택적으로 허용한다.

## 구현 단계

### Phase 0 — Hull 기술 검증

- seed point cloud와 3D Convex Hull prototype
- bottom flatten, flat normal과 bounds
- 동일 seed의 point·index 결정론 테스트

### Phase 1 — 정적 제작 MVP

- Profile, Request, TopologyData, Generator와 Variant
- BOULDER/SLAB/SHARD
- LOD0/1/2, convex collision과 Bake
- structured validation/report
- XML 문서, GDScript/C# API와 headless 샘플

### Phase 2 — 표현 품질

- strata, directional facets와 multi-lobe
- Vertex Color mask와 카툰 preview material
- LOD silhouette 안정화와 visual TC

### Phase 3 — 배치와 게임플레이

- biome별 Variant library와 terrain placement
- 정적 바위 batching/cell streaming
- 채굴 가능 바위의 상태·stable ID·교체 mesh
- 파괴는 사전 제작된 intact/damaged/depleted Variant 교체를 우선한다.

Report 규약과 자동·수동 검증 항목은 [바위 테스트 계획](open_world_rock_test_plan.md)을 따른다.

## 제외 범위

- 런타임 voxel 파괴와 임의 절단
- 동굴·구멍이 있는 비볼록 단일 메시
- 고해상도 sculpt와 texture baking
- 실시간 erosion과 physics fracture
- 초기 단계의 대규모 MultiMesh 배치

## 완료 기준

- 에이전트가 텍스트 입력만으로 세 Mode 샘플을 생성·검증·저장·재로드한다.
- 동일 seed 결과가 빌드 간 결정론적이다.
- 표준 Mono editor와 tests-enabled 빌드가 성공한다.
- Mono glue·Debug/Release assemblies와 agent docs에 공개 API가 노출된다.
- 기존 Tree/Vine 테스트를 포함한 전체 회귀 테스트가 통과한다.
