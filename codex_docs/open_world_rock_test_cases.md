# OpenWorld Rock 샘플 테스트 케이스

## 사용 방법

이 문서는 게임 프로젝트 샘플의 자동 테스트와 수동 Visual TC 기준이다.
자동 판정은 report와 저장 리소스를 기준으로 하며 화면 캡처만으로 합격시키지 않는다.

## 공통 입력

| ID | Mode | Seed | Stable ID |
|---|---|---:|---|
| R-B01 | BOULDER | 8801 | `sample-boulder-8801` |
| R-S01 | SLAB | 8802 | `sample-slab-8802` |
| R-H01 | SHARD | 8803 | `sample-shard-8803` |

## 자동 테스트

### RT-001 Request 로드와 검증

- 세 `.tres`를 `CACHE_MODE_IGNORE`로 로드한다.
- Resource 타입, mode, seed, profile과 stable ID를 확인한다.
- `validate_request().success`가 `true`여야 한다.
- `error_codes`는 비어 있어야 한다.

### RT-002 동일 seed 결정론

- 각 request를 새 Generator 두 개에서 독립적으로 생성한다.
- source points, hull vertices와 hull indices를 비교한다.
- LOD0/1/2 surface arrays와 collision points를 비교한다.
- `topology_hash`가 같아야 한다.

### RT-003 다른 seed 변화

- Profile과 size는 유지하고 seed만 변경한다.
- topology hash 또는 hull vertex 배열이 달라야 한다.
- 두 결과 모두 request size의 허용 범위를 과도하게 벗어나면 안 된다.

### RT-004 바닥 접촉

- `base_contact_count >= 3`이어야 한다.
- 모든 base contact vertex의 Y가 `base_plane`과 epsilon 내에서 같아야 한다.
- 세 LOD report의 `base_plane`이 같아야 한다.
- Ground 기준으로 떠 있거나 profile의 inset 이상 묻히면 실패다.

### RT-005 Mode 형태

- BOULDER의 세 bounds 축은 극단적으로 납작하거나 길지 않아야 한다.
- SLAB은 `bounds.size.y < bounds.size.x`와 `bounds.size.y < bounds.size.z`여야 한다.
- SHARD는 primary axis 길이가 다른 두 축보다 길어야 한다.
- 세 mode의 topology hash는 서로 달라야 한다.

### RT-006 Mesh 배열

- 각 LOD에 surface 0이 정확히 하나 있어야 한다.
- vertex, normal, UV0와 color 배열 길이가 같아야 한다.
- 모든 vertex와 normal은 finite 값이어야 한다.
- 각 triangle의 세 vertex normal은 같아 flat facet을 유지해야 한다.
- triangle winding normal은 rock 중심에서 바깥쪽을 향해야 한다.

### RT-007 Vertex Color mask

- 모든 RGBA 채널은 0부터 1 사이여야 한다.
- R은 upward/moss, G는 downward, B는 strata 정렬도다.
- A는 같은 seed에서 같고 seed 변경 시 phase가 바뀌어야 한다.

### RT-008 LOD 단조 감소

- 세 mesh가 모두 유효해야 한다.
- `LOD0 triangles > LOD1 triangles > LOD2 triangles`여야 한다.
- LOD bounds의 base plane과 pivot은 같아야 한다.
- LOD2 collision point 수는 profile의 limit 이하여야 한다.

### RT-009 Collision

- `get_generated_collision_points().size() >= 4`여야 한다.
- 모든 collision point는 LOD0 visual bounds 안에 있어야 한다.
- PhysicsServer가 초기화된 실행에서는 collision shape도 유효해야 한다.
- PhysicsServer가 없는 unit-test 진입점에서는 point cloud를 기준으로 판정한다.

### RT-010 Bake와 재로드

- `create_baked_variant()` 결과를 `.tres`로 저장한다.
- 재로드 후 LOD0/1/2 mesh와 collision points가 유효해야 한다.
- mode, seed, bounds, stable ID와 tags가 원본 report와 같아야 한다.
- 재로드 mesh 배열은 저장 전 배열과 같아야 한다.

### RT-011 Variant library

- biome별 Variant를 두 개 이상 등록한다.
- 같은 biome과 seed는 항상 같은 Variant를 선택해야 한다.
- 없는 biome은 null을 반환해야 한다.
- 배열 길이 불일치는 `ENTRY_COUNT_MISMATCH`로 실패해야 한다.

### RT-012 Placement cell

- 두 개 이상의 셀에 placement를 기록한다.
- `get_placements_in_cell()`은 해당 셀 항목만 반환해야 한다.
- position, rotation, scale, Variant index와 stable ID가 유지돼야 한다.
- 빈 stable ID, non-finite transform과 0 이하 scale은 실패해야 한다.

### RT-013 Gameplay state

- intact Variant에 damaged와 depleted Variant를 연결한다.
- 각 state 요청은 대응하는 사전 Bake Variant를 반환해야 한다.
- 연결되지 않은 state는 intact Variant로 안전하게 fallback해야 한다.

### RT-014 오류 내구성

다음 입력은 crash 없이 `success=false`와 안정적인 error code를 반환해야 한다.

- request 누락: `REQUEST_MISSING`
- profile 누락: `PROFILE_MISSING`
- zero 또는 negative size: `INVALID_SIZE`
- zero primary axis: `INVALID_PRIMARY_AXIS`
- 12개 미만 explicit points: `EXPLICIT_POINTS_TOO_FEW`
- NaN 또는 Inf explicit point: `EXPLICIT_POINT_NON_FINITE`
- 공선·공면·중복점 hull 실패: `HULL_DEGENERATE`

## 저장 결과 검사

각 report JSON에는 다음 필드가 있어야 한다.

```text
success, error_codes, warning_codes, mode, seed,
source_point_count, hull_vertex_count, hull_face_count,
discarded_point_count, duplicate_point_count,
base_contact_count, base_plane, topology_hash,
lods, collision_point_count, collision_face_count
```

## 수동 Visual TC

### RV-001 형태 구분

- 같은 거리에서 BOULDER, SLAB, SHARD가 실루엣으로 구분돼야 한다.
- SLAB은 판상, SHARD는 primary axis 방향 쐐기로 보여야 한다.

### RV-002 지면 정렬

- 바위가 떠 있지 않아야 한다.
- ground inset보다 과도하게 파묻히지 않아야 한다.
- 바닥 polygon이 최소 세 점으로 안정적으로 보여야 한다.

### RV-003 Facet와 mask

- flat shading 경계가 로우폴리 형태를 명확히 보여야 한다.
- 위쪽 moss mask와 아래쪽 mask가 방향에 맞아야 한다.
- strata가 SLAB 표면에서 가장 분명해야 한다.

### RV-004 LOD 전환

- LOD 전환 시 바위가 이동하거나 바닥에서 튀면 실패다.
- 실루엣 변화는 허용하지만 큰 순간 팝은 기록하고 Profile을 조정한다.

### RV-005 Collision overlay

- debug collision이 visual bounds 밖으로 크게 돌출하지 않아야 한다.
- 충돌이 바위보다 과도하게 작아 명백한 관통을 만들면 실패다.

## 최종 실행 기록

샘플 프로젝트의 결과 문서에는 다음을 남긴다.

- 사용한 커스텀 엔진 commit과 실행 파일 경로
- headless 생성·검증 명령
- 세 mode의 topology hash와 LOD triangle 수
- Bake 리소스 경로
- 자동 테스트 pass/fail 수
- Visual TC 캡처와 발견된 silhouette 문제
