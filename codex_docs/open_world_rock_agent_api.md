# OpenWorld Rock Agent API

## 제작 흐름

Rock은 원본 `.tres`와 파생 Bake를 분리한다.

1. `OpenWorldRockGenerationProfile`에 재사용할 형태 규칙을 기록한다.
2. `OpenWorldRockGenerationRequest`에 mode, seed, size, stable ID를 기록한다.
3. `validate_request()` 결과의 `success`와 `error_codes`를 확인한다.
4. 필요하면 `generate_topology()` 결과와 `topology_hash`만 검사한다.
5. `generate_rock()`으로 LOD0/1/2와 convex collision을 만든다.
6. `get_generation_report()`를 JSON으로 저장하거나 비교한다.
7. `create_baked_variant()`를 `.tres`로 저장하고 재로드해 검증한다.

이 흐름은 Inspector나 viewport 선택을 요구하지 않는다.

## 결정론 규약

- 같은 Profile, Request와 seed는 같은 source point, canonical hull index와 hash를 만든다.
- `explicit_points`가 비어 있지 않으면 자동 sphere sampling을 우회한다.
- 바닥점은 공통 `base_plane`에 투영되고 모든 LOD가 같은 원점을 쓴다.
- LOD는 LOD0 source point의 결정론적 subset으로 다시 hull을 만든다.
- report 실패는 `REQUEST_MISSING`, `PROFILE_MISSING`, `INVALID_SIZE`,
  `INVALID_PRIMARY_AXIS`, `EXPLICIT_POINTS_TOO_FEW`,
  `EXPLICIT_POINT_NON_FINITE`, `HULL_DEGENERATE` 코드로 판별한다.

## Report 핵심 필드

| 필드 | 의미 |
|---|---|
| `success`, `error_codes`, `warning_codes` | 구조화된 상태 |
| `mode`, `seed` | 원본 요청 |
| `source_point_count` | flatten 이후 입력점 수 |
| `hull_vertex_count`, `hull_face_count` | LOD0 hull 통계 |
| `base_contact_count`, `base_plane` | 지면 접촉 정보 |
| `topology_hash` | canonical topology 비교 키 |
| `lods` | LOD별 vertex, triangle, AABB |
| `collision_point_count`, `collision_face_count` | convex collision 통계 |

## 재질 채널

Surface 0은 flat triangle vertex를 사용한다. UV0는 안정적인 XZ 보조 좌표다.
Vertex Color R은 upward/moss, G는 downward, B는 strata 정렬도,
A는 seed phase다. 프로젝트 shader도 같은 채널을 사용한다.

## 배치와 상태

`OpenWorldRockVariantLibrary.select_variant(biome, seed)`는 전역 RNG 없이
Variant를 고른다. `OpenWorldRockPlacementData`는 transform, Variant index와
stable ID를 평행 배열로 저장하고 `get_placements_in_cell()`로 셀 단위 조회한다.

채굴 상태는 runtime fracture 대신 `OpenWorldRockVariant.get_variant_for_state()`로
미리 Bake한 intact, damaged, depleted Variant를 교체한다.

## Headless 실행

GDScript 샘플은 별도 Godot 프로젝트에서 다음처럼 실행한다.

```powershell
godot --headless --path <project> --script res://open_world_rock_headless_sample.gd
```

샘플은 `validate → generate → report → save → reload`를 모두 수행하며 실패 시
0이 아닌 종료 코드를 반환한다. C# 샘플은 같은 API 순서를 보여 준다.
