# Godot 스타일라이즈드 나무 생성기 3차

## 범위

3차는 동일 profile과 seed에서 LOD0/LOD1/LOD2를 생성하고, 공통 Wind vertex color를 기록해 정적 Variant로 일괄 Bake한다.

```text
Profile + Seed
      ↓
LOD0 / LOD1 / LOD2
      ↓
Wind vertex color + material
      ↓
OpenWorldTreeVariant
      ↓
OpenWorldTree3D 거리별 MultiMesh bucket
```

## 현재 구현

- LOD0/LOD1/LOD2 결정론적 생성
- LOD별 path segment와 radial side 감소
- 일정 간격의 branch, secondary branch, root 제거
- canopy blob과 Palm frond 수 감소
- LOD2 low-detail canopy blob
- 세 LOD와 전환 거리 일괄 Bake
- 강제 LOD editor preview
- LOD별 vertex/triangle 통계
- trunk/branch/foliage 공통 Wind vertex color
- 기본 카툰 Wind ShaderMaterial

Billboard/impostor와 자동 텍스처 Bake는 이번 범위에 포함하지 않는다.

## LOD 생성 규칙

LOD는 별도 seed로 다시 변형하지 않는다. 동일 seed의 수형을 생성하고 표시 디테일만 제거한다.

### LOD0

- profile의 전체 trunk와 branch segment 사용
- 모든 primary/secondary branch와 root 사용
- 전체 canopy blob 또는 Palm frond 사용
- 세밀한 blob topology 사용

### LOD1

- `lod1_quality`에 따라 segment와 radial side 감소
- primary branch와 root를 일정 stride로 선택
- foliage 개수는 유지하고 blob topology 또는 frond segment 감소
- secondary branch 생략
- 기본값 `0.55`

### LOD2

- `lod2_quality`에 따라 추가 단순화
- 주요 primary branch만 유지하고 foliage topology를 추가 단순화
- canopy blob은 subdivision 없는 저해상도 topology 사용
- 기본값 `0.18`

LOD가 달라도 다음은 유지한다.

- 지면 중심 pivot
- 전체 나무 높이
- trunk/foliage surface 순서
- seed 기반 방향과 주요 실루엣
- UV와 Wind vertex color 채널

## Generator LOD 설정

| Property | 기본값 | 역할 |
|---|---:|---|
| `generate_lod1` | true | LOD1 생성 및 Bake |
| `generate_lod2` | true | LOD2 생성 및 Bake |
| `lod1_quality` | 0.55 | LOD1 디테일 비율 |
| `lod2_quality` | 0.18 | LOD2 디테일 비율 |
| `lod1_distance` | 25 m | LOD1 전환 거리 |
| `lod2_distance` | 55 m | LOD2 전환 거리 |
| `max_distance` | 120 m | 최대 렌더 거리 |
| `preview_lod` | LOD0 | 에디터 강제 표시 |

`lod1_distance <= lod2_distance <= max_distance`가 setter에서 유지된다.

## Wind 데이터 규약

모든 LOD의 vertex color는 시각 색상이 아니라 다음 변형 가중치다.

| Channel | 의미 |
|---|---|
| R | 높이 기반 전체 굽힘 |
| G | branch/frond 보조 굽힘 |
| B | foliage flutter |
| A | profile seed 기반 안정적 phase |

root는 고정되므로 RGB가 0이다. trunk는 높이에 따라 R이 증가하고, branch 끝과 Palm frond 끝은 G/B가 증가한다.

## 기본 카툰 Wind

`wind_enabled = true`이면 생성기의 surface 재질에 기본 Wind ShaderMaterial이 연결된다.

| Property | 역할 |
|---|---|
| `wind_strength` | 전체 변위 크기 |
| `wind_speed` | 시간 배율 |
| `wind_gust_strength` | 저주파 돌풍 크기 |
| `wind_direction` | 로컬 XZ 바람 방향 |
| `wind_trunk_color` | 기본 줄기색 |
| `wind_foliage_color` | 기본 잎색 |

기본 shader는 위치 hash를 사용해 MultiMesh instance마다 phase 차이를 준다. 프로젝트 전용 shader를 사용할 때도 같은 RGBA 규약을 소비하면 된다.

## 에디터 사용

1. `OpenWorldTreeGenerator3D`에 profile과 material을 설정한다.
2. LOD quality와 거리를 설정한다.
3. `Generate Tree`를 누른다.
4. `preview_lod`를 LOD0, LOD1, LOD2로 바꿔 실루엣을 비교한다.
5. Inspector 하단의 vertex/triangle 통계를 확인한다.
6. 필요하면 `wind_enabled`를 켜 변형을 확인한다.
7. `Bake Variant...`로 세 LOD를 저장한다.

통계는 Inspector가 다시 구성될 때 갱신된다.

프로그램에서 `OpenWorldTreeGenerator3D.new()`로 생성할 때는 `generation_profile`을 명시적으로 지정한다. 에디터에서 추가한 노드는 새 profile이 자동 생성된다.

## GDScript 예시

```gdscript
var generator := OpenWorldTreeGenerator3D.new()
generator.generation_profile = profile
generator.seed = 8801
generator.lod1_quality = 0.55
generator.lod2_quality = 0.18
generator.lod1_distance = 25.0
generator.lod2_distance = 55.0
generator.wind_enabled = true
generator.generate_tree()

var lod2: ArrayMesh = generator.get_generated_lod_mesh(2)
var stats: Dictionary = generator.get_lod_statistics(2)
var variant: OpenWorldTreeVariant = generator.create_baked_variant()
```

## Bake 결과

`create_baked_variant()`는 다음을 복사한다.

- `lod0_mesh`, `lod1_mesh`, `lod2_mesh`
- `lod1_distance`, `lod2_distance`, `max_distance`
- `source_seed`
- collision radius/height 힌트
- 각 surface에 연결된 material

LOD 생성이 꺼진 슬롯은 비어 있으며, `OpenWorldTreeVariant`의 기존 fallback 규칙이 더 상세한 메시를 선택한다.

## 검증 기준

- 같은 profile/seed의 세 LOD가 반복 생성 시 동일하다.
- LOD1 triangle 수는 LOD0보다 작다.
- LOD2 triangle 수는 LOD1보다 작다.
- 세 LOD의 높이와 pivot이 유지된다.
- 모든 LOD가 trunk/foliage 두 surface를 유지한다.
- Wind color 배열이 모든 surface에 존재한다.
- Bake 후 씬 재로드에서도 세 LOD와 거리가 유지된다.
- 거리 전환 시 크기 변화나 큰 실루엣 popping이 없다.

## 알려진 제한

- LOD는 범용 mesh decimator가 아니라 생성 규칙 기반 단순화다.
- LOD2 billboard/impostor는 아직 없다.
- 기본 Wind material은 단색 카툰 preview용이며 texture shader는 프로젝트에서 확장해야 한다.
- Palm은 ribbon frond이며 개별 leaflet은 생성하지 않는다.
- 지형 적응 root와 벌목 물리는 3차 범위가 아니다.
