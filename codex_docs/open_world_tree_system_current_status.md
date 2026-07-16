# OpenWorld Tree System 현재 구현 현황

## 목적

OpenWorld Tree System은 카툰 스타일 나무를 Godot에서 절차적으로 제작하고, 정적 리소스로 Bake한 뒤, 넓은 월드에 효율적으로 배치하기 위한 시스템이다.

핵심 방향:

- Blender 완성 나무 변형에 의존하지 않는다.
- Godot에서 profile과 seed로 나무를 만든다.
- 생성 결과는 게임 실행 전에 정적 메시로 Bake한다.
- 멀리 있는 나무는 `MultiMesh`로 렌더링한다.
- 상호작용하는 나무만 별도 벌목 씬으로 전환한다.
- 메시 절단은 현재 범위에서 제외한다.

## 데이터 흐름

```text
OpenWorldTreeGenerationProfile + seed + materials
                         ↓
             OpenWorldTreeGenerator3D
                         ↓ Bake
              OpenWorldTreeVariant
                         ↓
              OpenWorldTreeSpecies
                         +
          OpenWorldTreePlacementData
                         ↓
               OpenWorldTree3D
```

## 구현된 클래스

### `OpenWorldTreeGenerationProfile`

나무 계열의 형태 규칙을 저장한다.

- Trunk: 높이, 굵기, taper, segment, radial sides, 굽힘
- Branch: 시작/끝 높이, 간격, phyllotaxy, 상승각, 길이, 굵기
- Canopy: blob 개수, 반지름, 세로 비율, 위치 편차, 거칠기

Profile은 수형 계열을, seed는 같은 계열 안의 개별 변형을 담당한다.

### `OpenWorldTreeGenerator3D`

Profile과 seed로 정적 `ArrayMesh`를 생성하는 제작 노드다.

- 굽은 tapered trunk
- interval/phyllotaxy 기반 가지
- tapered low-poly branch
- faceted canopy blob
- Temperate/Tropical/Umbrella/Conifer/Palm/Mangrove archetype
- crown envelope와 secondary branch
- trunk flare, buttress root, prop root
- Palm 양면 frond ribbon
- 결정론적 LOD0/LOD1/LOD2 생성과 일괄 Bake
- Wind vertex color와 기본 카툰 wind preview
- 강제 LOD preview와 triangle 통계
- 동일 profile/seed에 대한 결정론적 결과
- surface 0: trunk와 branch
- surface 1: foliage

Inspector 액션:

- `Generate Tree`
- `Randomize Seed`
- `Bake Variant...`

### `OpenWorldTreeVariant`

배치 가능한 나무 한 변형을 저장한다.

- 이름과 원본 seed
- LOD0/LOD1/LOD2 mesh
- LOD 거리와 최대 렌더 거리
- material override
- 충돌 반지름/높이 메타데이터

현재 Generator의 자동 Bake는 활성화된 LOD0/LOD1/LOD2와 전환 거리를 함께 저장한다.

### `OpenWorldTreeSpecies`

같은 종 또는 게임 플레이 분류의 Variant들을 묶는다. seed 기반 Variant 선택은 결정론적이다.

### `OpenWorldTreePlacementData`

대량 배치 데이터를 compact array로 저장한다.

- 위치, 회전, 크기
- seed, variant index
- stable instance ID
- enabled 상태
- color와 custom data

Stable ID는 배열 index와 별개이며 벌목 상태와 월드 저장 키로 사용할 수 있다.

### `OpenWorldTree3D`

Species와 PlacementData를 이용하는 대량 정적 렌더러다.

- Variant/LOD별 `MultiMesh` bucket
- 카메라 거리 기반 LOD 분류
- 최대 거리 culling
- 가까운 나무 ID 검색
- ID 기반 transform 조회
- 개별 나무 활성화/비활성화
- Scene View 선택 AABB
- `EXIT_TREE` 렌더 리소스 정리

## 완료된 1차 범위

- Godot 네이티브 나무 생성기
- 재사용 가능한 generation profile
- seed variation과 정적 Bake
- trunk/foliage 2-surface mesh
- Variant/Species/Placement/Renderer 리소스 구조
- MultiMesh 정적 렌더링
- stable ID와 enabled 상태
- Scene View 선택
- GDScript/C# API
- XML 클래스 문서와 agent docs CLI
- Blender 제작/GLB 문서

## 검증 결과

- OpenWorldTree 테스트 8개 통과
- Assertion 74개 통과
- Windows Mono editor 개발 빌드 성공
- Mono glue 및 Debug/Release C# assemblies 생성 성공
- Generator/Profile agent docs 조회 성공

검증 범위:

1. LOD 선택과 fallback
2. seed 기반 Variant 결정론
3. stable ID와 enabled 상태
4. Generator 정점 결정론과 두 surface Bake
5. LOD0/LOD1/LOD2 결정론과 triangle 감소
6. Wind vertex color 배열과 채널 범위
7. 세 LOD와 거리의 Variant Bake
8. Scene View selection mesh

## 현재 개발 판단

생성기 자체는 게임 프로젝트에서 Variant를 제작하고 배치해 볼 수 있는 단계다. 다음 핵심 목표는 생성 기능을 무한히 확장하는 것이 아니라 정적 나무를 벌목 가능한 개체로 승격하는 Phase 4다.

먼저 마감할 Phase 3.5 항목:

- 수종별 공식 `OpenWorldTreeGenerationProfile` preset과 sample scene
- texture와 Wind 규약을 함께 사용하는 게임용 trunk/foliage material
- bark/foliage UV와 smooth/faceted normal 규약 확정
- multi-seed preview와 선택 seed batch Bake
- 전체 archetype의 LOD 전환 visual QA

이후 우선 구현:

- `HarvestableTree3D`와 trunk collision proxy
- stable ID 기반 정적/interactive 전환
- 체력, damage, 낙하와 규격화된 resource drop
- harvested/respawn 상태 저장
- 실제 숲 benchmark 후 spatial index와 streaming

## 현재 제한

- 1단계 가지까지만 생성
- Banyan aerial root와 지형 적응 root 없음
- canopy가 blob 조합으로 제한됨
- 개별 leaf card와 Palm leaflet 분할 없음
- LOD2 billboard/impostor와 texture bake 없음
- 기본 wind shader는 단색 preview용이며 texture shader 확장 필요
- 자동 collision/벌목 물리 및 spatial index/cell streaming 없음

## 관련 문서

- [트리 시스템 문서 인덱스](open_world_tree_system_status_and_roadmap.md)
- [생성기 제작 가이드](godot_stylized_tree_generator_mvp.md)
- [3차 LOD/Wind 가이드](godot_stylized_tree_generator_phase3.md)
- [향후 생성/렌더링 로드맵](open_world_tree_generation_roadmap.md)
- [벌목 상호작용 구조](open_world_tree_harvesting_architecture.md)
- [덩굴 시스템 별도 개발 플랜](open_world_vine_system_plan.md)
