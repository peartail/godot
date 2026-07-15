# Godot Stylized Tree Generator 2차 가이드

## 추가된 기능

2차 생성기는 하나의 broadleaf 수식만 사용하지 않고 `archetype`, `crown_shape`, `root_style` 조합으로 수형을 선택한다. 동일한 profile과 seed는 1차와 마찬가지로 결정론적인 정적 `ArrayMesh`를 만든다.

지원 archetype:

- `Temperate Broadleaf`
- `Tropical Broadleaf`
- `Umbrella`
- `Conifer`
- `Palm`
- `Mangrove`

모든 결과는 기존 계약을 유지한다.

- Surface 0: trunk, primary/secondary branch, root
- Surface 1: canopy blob 또는 palm frond
- `Bake Variant...`: `OpenWorldTreeVariant.lod0_mesh`로 저장

## Archetype 동작

### Temperate Broadleaf

1차 생성 방식과 호환되는 기본 모드다. `root_style = Auto`일 때 별도 root를 생성하지 않고 round crown을 사용한다.

### Tropical Broadleaf

- 높은 위치에서 시작하는 굵고 긴 가지
- 일반 활엽수보다 넓은 canopy blob
- Auto root에서 buttress root 생성

판근이 지나치게 크면 `root_length`, `root_height`, `root_flare_scale`을 줄인다.

### Umbrella

- 상부에 집중된 가지
- 낮은 branch elevation
- 수평으로 넓어지는 branch length envelope
- 세로로 압축된 canopy
- Auto root에서 trunk flare 사용

### Conifer

- 낮은 높이부터 가지 생성
- 위로 갈수록 짧아지는 conical envelope
- 아래로 처지는 branch bias
- canopy 크기가 높이에 따라 감소

현재 foliage는 needle mesh가 아니라 작은 blob crown이다.

### Palm

- 일반 branch를 생성하지 않음
- 끝까지 굵기를 유지하는 단일 trunk
- trunk 꼭대기에 방사형 frond ribbon 생성
- frond는 양면 triangle mesh
- Auto root에서 trunk flare 사용

Palm에서는 `canopy_blob_count` 대신 다음 값을 사용한다.

- `palm_frond_count`
- `palm_frond_length`
- `palm_frond_width`
- `palm_frond_droop`

### Mangrove

- 낮은 branch 시작/종료 범위
- tiered crown
- Auto root에서 elevated prop root 생성

현재 물이나 지면 높이를 자동으로 샘플링하지 않으므로 root 끝은 generator local Y=0을 기준으로 한다.

## Crown Shape

`crown_shape = Auto`는 archetype에 따라 다음처럼 해석된다.

| Archetype | Auto crown |
|---|---|
| Temperate/Tropical | Round |
| Umbrella | Umbrella |
| Conifer | Conical |
| Palm | Frond 전용 |
| Mangrove | Tiered |

수동으로 `Round`, `Umbrella`, `Conical`, `Tiered`를 지정해 archetype 기본값을 덮어쓸 수 있다.

## Root Style

`root_style = Auto` 매핑:

| Archetype | Auto root |
|---|---|
| Temperate/Conifer | None |
| Tropical | Buttress |
| Umbrella/Palm | Flare |
| Mangrove | Prop |

Root 설정:

- `root_flare_scale`: 지면에서 trunk 반지름 배율
- `root_height`: flare 또는 root attachment 높이
- `root_count`: buttress/prop root 개수
- `root_length`: 지면 방향 길이

## Secondary Branch

`secondary_branch_count`를 1 이상으로 지정하면 각 primary branch 중후반부에서 자식 가지가 생성된다.

- `secondary_branch_count`: primary branch당 자식 수
- `secondary_branch_scale`: primary 대비 길이 비율
- `branch_droop`: 모든 branch 끝의 추가 하강량

카툰 실루엣에는 0~2개가 적당하다. 높은 값은 정점 수와 canopy anchor 수를 빠르게 증가시킨다.

## 권장 시작값

### Tropical Broadleaf

```text
archetype = Tropical Broadleaf
tree_height = 7~10 m
branch_start_ratio = 0.45~0.6
branch_length = 2.5~4.5 m
secondary_branch_count = 1
root_count = 4~7
root_length = 1.2~2.5 m
```

### Palm

```text
archetype = Palm
tree_height = 6~12 m
trunk_radial_sides = 6~8
trunk_bend = 0.2~0.7 m
palm_frond_count = 8~14
palm_frond_length = 2~4 m
palm_frond_droop = 0.5~1.4 m
```

## 스크립트 예시

```gdscript
var profile := OpenWorldTreeGenerationProfile.new()
profile.archetype = OpenWorldTreeGenerationProfile.ARCHETYPE_PALM
profile.tree_height = 8.0
profile.palm_frond_count = 12
profile.palm_frond_length = 3.2

var generator := OpenWorldTreeGenerator3D.new()
generator.auto_generate = false
generator.generation_profile = profile
generator.seed = 2402
generator.generate_tree()
```

## 남은 2차 품질 작업

- 개별 leaf card/leaf cluster
- Palm leaflet 분할
- Banyan aerial root
- 지형 높이를 따르는 root
- smooth/faceted normal 선택
- 확정된 bark/foliage UV와 vertex color 규약
- biome별 공식 profile preset과 sample scene

이 항목들은 현재 archetype API를 유지하면서 후속 메시 모듈로 확장한다.
