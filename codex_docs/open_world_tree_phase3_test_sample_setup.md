# OpenWorld Tree 3차 테스트 샘플 구성

## Scene Tree

```text
TreePhase3Test
├─ WorldEnvironment
├─ DirectionalLight3D
├─ CameraRig
│  └─ Camera3D
├─ Ground
├─ SingleTreeRoot
│  └─ OpenWorldTreeGenerator3D
├─ DistanceMarkers
│  ├─ Marker15m
│  ├─ Marker30m
│  └─ Marker50m
└─ ForestRoot
   └─ OpenWorldTree3D
```

## 기본 환경

- Ground 크기: 최소 `120m × 120m`
- 나무 원점: `(0, 0, 0)`
- 카메라 높이: `2~4m`
- DirectionalLight는 나무의 정면과 측면이 모두 읽히도록 배치한다.
- 첫 검사는 Forward+에서 진행하고 필요하면 Mobile/Compatibility를 비교한다.
- Ground에 5m 또는 10m 간격 눈금을 두면 거리와 흔들림을 보기 쉽다.

## 단일 나무 Generator

`SingleTreeRoot/OpenWorldTreeGenerator3D`에 다음 값을 사용한다.

```text
seed = 1207
auto_generate = true
generate_lod1 = true
generate_lod2 = true
lod1_quality = 0.55
lod2_quality = 0.18
lod1_distance = 15.0
lod2_distance = 30.0
max_distance = 50.0
preview_lod = LOD0
wind_enabled = false
```

재질:

- trunk: 불투명 갈색
- foliage: 불투명 녹색
- foliage culling: 첫 검사에서는 Disabled
- Wind 기본 material 검사는 `wind_enabled = true`로 전환

## 기본 Temperate Profile

```text
archetype = Temperate Broadleaf
tree_height = 7.0
trunk_segments = 9
trunk_radial_sides = 7
trunk_base_radius = 0.38
trunk_tip_radius = 0.08
branch_start_ratio = 0.30
branch_end_ratio = 0.90
branch_interval = 0.55
branch_segments = 5
secondary_branch_count = 1
secondary_branch_scale = 0.50
canopy_blob_count = 14
canopy_radius_min = 0.70
canopy_radius_max = 1.20
```

이 profile을 먼저 사용하면 자동화 테스트와 가까운 조건에서 LOD 감소율을 확인할 수 있다.

## Archetype 비교 샘플

동일한 기본 profile을 복제한 뒤 archetype과 핵심값만 조정한다.

| 이름 | Archetype | Seed | 핵심 조정 |
|---|---|---:|---|
| Temperate_A | Temperate Broadleaf | 1207 | 기본값 |
| Tropical_A | Tropical Broadleaf | 1207 | root Auto, 수관 폭 확인 |
| Umbrella_A | Umbrella | 1207 | crown Auto |
| Conifer_A | Conifer | 1207 | crown Auto |
| Palm_A | Palm | 1207 | frond 12, length 3.2 |
| Mangrove_A | Mangrove | 1207 | root Auto, root count 5 |

각 샘플은 먼저 Generator에서 비교한 뒤 정상 결과만 Variant로 Bake한다.

## Seed 비교 세트

모든 archetype에서 다음 seed를 사용하면 총 18개 샘플이 된다.

```text
1207
2402
8801
```

파일 이름 예시:

```text
temperate_seed1207.owtreevariant
palm_seed2402.owtreevariant
mangrove_seed8801.owtreevariant
```

## 거리 표식

SingleTree의 원점에서 카메라 이동축 방향으로 표식을 둔다.

| Marker | 거리 | 예상 상태 |
|---|---:|---|
| Marker15m | 15m | LOD0 → LOD1 |
| Marker30m | 30m | LOD1 → LOD2 |
| Marker50m | 50m | LOD2 → culling |

경계를 앞뒤로 천천히 통과하고, 빠르게 왕복하는 검사도 한 번 수행한다.

## ForestRoot 구성

1. Bake한 Variant 3개 이상을 하나의 `OpenWorldTreeSpecies`에 등록한다.
2. 새 `OpenWorldTreePlacementData`를 만든다.
3. 10×10 grid로 100개를 배치한다.
4. 간격은 `6~10m`, scale은 `0.85~1.15` 범위로 둔다.
5. 각 placement에 stable seed와 ID를 사용한다.
6. `OpenWorldTree3D.camera_path`를 `CameraRig/Camera3D`에 연결한다.

권장 배치 영역:

```text
X = -45m ~ 45m
Z = -45m ~ 45m
Y = 0m
```

## Wind 비교 프리셋

| 이름 | Strength | Speed | Gust | 용도 |
|---|---:|---:|---:|---|
| Calm | 0.08 | 0.6 | 0.10 | 약한 상시 바람 |
| Default | 0.22 | 1.0 | 0.35 | 기본 검증 |
| Strong | 0.65 | 1.5 | 0.80 | 변형 가독성 |
| Stress | 2.00 | 8.0 | 2.00 | 안정성 검사 |

Stress는 품질 기준이 아니라 NaN, 폭발, 크래시 검사용이다.

## 캡처 위치

각 샘플에서 다음 이미지를 저장한다.

- 정면 LOD0/1/2
- 측면 LOD0/1/2
- 상단 LOD0/1/2
- 15m, 30m 전환 직전과 직후
- Wind Default에서 좌우 최대 굽힘
- Bake 후 재로드 결과
- 100개 군락의 근거리와 원거리

## 결과 기록표

| Sample | LOD0 tri | LOD1 tri | LOD2 tri | Height 유지 | Wind | Bake | 판정 |
|---|---:|---:|---:|---|---|---|---|
| Temperate_A | | | | | | | |
| Tropical_A | | | | | | | |
| Umbrella_A | | | | | | | |
| Conifer_A | | | | | | | |
| Palm_A | | | | | | | |
| Mangrove_A | | | | | | | |

## 실행 순서

1. Temperate_A로 LOD 통계와 강제 preview를 확인한다.
2. 6개 archetype의 LOD 실루엣을 비교한다.
3. Wind Default와 Stress를 검사한다.
4. Variant를 Bake하고 재로드한다.
5. DistanceMarkers에서 실제 전환을 확인한다.
6. ForestRoot의 100개 군락을 이동하며 검사한다.
7. 발견한 현상을 TC 번호와 함께 별도 리포트로 기록한다.
