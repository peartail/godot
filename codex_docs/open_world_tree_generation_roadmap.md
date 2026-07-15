# OpenWorld Tree 생성 및 렌더링 로드맵

## 열대지방 나무도 가능한가?

가능하다. 다만 모든 열대 나무를 현재 trunk + phyllotaxy branch + canopy blob 조합만으로 표현하기는 어렵다.

현재 생성기로 바로 근사하기 쉬운 유형:

- 열대 우산형 활엽수
- 높은 줄기와 넓고 납작한 수관을 가진 정글 교목
- 굵은 줄기와 적은 수의 큰 가지를 가진 stylized tropical broadleaf

현재 구조 확장이 필요한 유형:

- 야자나무: 일반 가지 대신 줄기 꼭대기의 방사형 frond가 필요
- 맹그로브: prop root와 낮은 분기 구조가 필요
- 반얀나무: 넓은 crown과 aerial root가 필요
- 판근이 발달한 열대 교목: buttress root 생성기가 필요

따라서 열대 나무는 2차 생성 품질 범위에 명시적으로 포함한다.

## 2차 수형 Archetype

하나의 파라미터 집합을 무리하게 늘리기보다 profile에 수형 archetype을 두는 방향을 권장한다.

```text
Temperate Broadleaf
Tropical Broadleaf
Umbrella / Savanna
Conifer
Palm
Mangrove / Prop-root
```

Archetype은 메시 생성기를 완전히 분리하는 타입이 아니라, trunk·branch·root·foliage 모듈의 조합 preset으로 설계한다.

예시:

| Archetype | Trunk | Branch | Root | Foliage |
|---|---|---|---|---|
| Tropical Broadleaf | 굵고 높은 줄기 | 상부 대형 가지 | buttress 선택 | 넓은 blob crown |
| Umbrella | 중간 줄기 | 수평 확장 | 기본 root flare | 납작한 envelope |
| Palm | 단일 taper 줄기 | 없음 | 작은 flare | 방사형 frond |
| Mangrove | 짧은 줄기 | 낮은 분기 | prop root | 작은 dense blob |

## Phase 1.5 — 제작 안정화

개발 항목:

- sample scene과 기본 카툰 material
- broadleaf/umbrella/conifer profile preset
- 여러 seed를 비교하는 preview grid
- 선택 seed batch bake
- 파라미터 유효성 경고
- vertex/triangle 통계 표시

완료 기준:

- 외부 모델 없이 빠르게 Species용 Variant 세트를 만들 수 있다.
- 한 profile에서 여러 seed를 비교하고 선택 저장할 수 있다.

## Phase 2 구현 상태

현재 완료:

- 6개 archetype API와 결정론적 생성 경로
- Round/Umbrella/Conical/Tiered crown
- Secondary branch와 branch droop
- Flare/Buttress/Prop root
- Palm 전용 양면 frond ribbon

후속 품질 작업은 아래 공통/열대 항목 중 leaf card, leaflet, aerial root, normal/UV 규약이다.
## Phase 2 — 수형과 카툰 생성 품질

공통 개발 항목:

- root flare와 stylized root
- 2단계 자식 가지
- branch droop, tropism, crown bias
- round/umbrella/tiered/conical crown envelope
- blob 크기 gradient
- smooth/faceted normal 모드
- bark/foliage UV와 vertex color 규약

열대 수형 개발 항목:

- `Tropical Broadleaf` crown preset
- 판근용 buttress root module
- `Palm` 단일 줄기 모드
- 방사형 frond generator와 frond droop
- `Mangrove` prop-root module
- `Banyan` aerial-root 실험 옵션
- 큰 잎을 위한 leaf cluster 또는 low-poly leaf card

2차 완료 기준:

- 온대 활엽수, 열대 활엽수, 우산형, 침엽수, 야자수 실루엣이 명확히 구분된다.
- 같은 archetype의 seed variation은 다양하지만 미술 방향은 유지된다.
- 열대 활엽수와 야자수는 서로 다른 foliage 생성 방식을 사용한다.
- 생물학적 성장 시뮬레이션 없이 정적 Bake가 가능하다.

## Phase 3 — LOD와 Wind

개발 항목:

- LOD1 branch/canopy 단순화
- LOD2 trunk 축약과 canopy 병합 또는 billboard
- LOD 거리 preview
- trunk stiffness와 branch/foliage wind weight
- 카툰 wind shader
- LOD0/LOD1/LOD2 일괄 Bake

완료 기준:

- 한 번의 Bake로 Variant의 모든 LOD가 생성된다.
- LOD 전환 시 주요 실루엣이 유지된다.
- Archetype이 달라도 동일한 wind channel 규약을 사용한다.

## Phase 4 — Interactive Tree와 벌목

개발 항목:

- `HarvestableTree3D` 제작
- 정적 instance와 interactive scene 전환
- collision proxy, 체력, damage, 낙하
- 규격화된 log/resource drop
- harvested/respawn 상태 저장

상세 구조는 [벌목 상호작용 구조](open_world_tree_harvesting_architecture.md)를 따른다.

## Phase 5 — 대규모 월드 최적화

개발 항목:

- placement cell/chunk
- 카메라 주변 streaming
- spatial grid 또는 BVH
- LOD 갱신 프레임 분산
- interactive tree 최대 개수 제한
- profiler 통계와 benchmark scene

목표 플랫폼 확정 후 visible instance, draw call, LOD CPU 시간, placement memory를 수치화한다.

## Phase 6 — 월드 제작 도구

개발 항목:

- Terrain 수동/브러시/scatter 배치
- slope, height, layer, exclusion 규칙
- biome별 Species palette
- stable ID 유지형 regenerate
- Variant thumbnail과 asset audit

열대 biome에서는 Tropical Broadleaf, Palm, Mangrove Species를 밀도와 지형 조건에 따라 혼합할 수 있어야 한다.

## 권장 다음 작업

1. Phase 1.5 sample scene과 preview grid
2. `archetype`과 crown envelope 데이터 모델 설계
3. Tropical Broadleaf/우산형 preset으로 기존 blob 방식 검증
4. Palm용 frond generator를 별도 foliage module로 구현
5. Buttress/prop root module 추가
6. 수형이 안정된 후 LOD와 wind 규약 확정

초기 열대 나무 구현은 `Tropical Broadleaf`와 `Palm` 두 종류가 적당하다. 전자는 현재 시스템의 확장성을 검증하고, 후자는 일반 branch/canopy와 다른 생성 모듈 조합을 검증할 수 있다.
