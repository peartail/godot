# OpenWorld Tree 2차 수동 테스트

## 목적

2차 생성기의 6개 수형, seed 변형, 잎 표면, 뿌리, 가지, Bake 결과를 에디터에서 같은 기준으로 검증한다.

## 테스트 환경 기록

실행 전 아래 정보를 결과 기록에 남긴다.

```text
Engine build/date:
Renderer: Forward+ / Mobile / Compatibility
GPU / OS:
Test scene:
Trunk material:
Foliage material:
```

## 공통 준비

1. 빈 3D 씬에 `DirectionalLight3D`, `WorldEnvironment`, `Camera3D`를 추가한다.
2. `OpenWorldTreeGenerator3D`를 추가한다.
3. 새 `OpenWorldTreeGenerationProfile`을 생성해 generator에 연결한다.
4. trunk는 갈색 불투명 재질, foliage는 밝은 녹색 불투명 재질로 지정한다.
5. 첫 검증에서는 foliage 재질의 culling을 Disabled로 둔다.
6. 생성 후 정면·측면·상단과 근거리·중거리에서 확인한다.
7. 각 케이스의 스크린샷과 사용한 profile 값을 저장한다.

## 합격 기준

- 오류나 크래시 없이 생성·재생성·Bake된다.
- 동일 profile과 seed는 같은 정적 메시를 만든다.
- seed 변경 시 실루엣이 변하지만 수형의 정체성은 유지된다.
- 결과 메시에는 trunk/branch/root와 foliage의 두 surface가 존재한다.
- foliage surface가 비어 있지 않고 지정 재질로 보인다.
- 지면 아래로 과도하게 파고들거나 공중에 뜨는 부분이 없다.
- NaN, 뒤집힌 면, 긴 삼각형 폭발, 원점으로 뻗는 면이 없다.

## TC-01 기본 수형 6종

각 항목에서 `archetype`만 바꾸고 seed는 `1207`로 고정한다.

| Archetype | 기대 실루엣 | 중점 확인 |
|---|---|---|
| Temperate Broadleaf | 둥근 수관과 분산된 가지 | 기본형 회귀, foliage 분포 |
| Tropical Broadleaf | 크고 풍성한 상부 수관 | 넓은 수관과 줄기 비율 |
| Umbrella | 수평으로 넓은 우산형 수관 | 낮은 수관 두께와 측면 폭 |
| Conifer | 위가 좁은 원뿔형 | 층별 가지와 상단 수렴 |
| Palm | 긴 단일 줄기와 방사형 frond | foliage surface와 양면 표시 |
| Mangrove | 넓은 하부와 노출 뿌리 | prop root 접지와 겹침 |

판정: 여섯 결과를 실루엣만 보고 서로 구분할 수 있어야 한다.

## TC-02 Seed 결정성과 변형

1. 각 수형을 seed `1207`, `2402`, `8801`로 생성한다. 총 18개다.
2. `1207` 결과를 삭제 후 같은 값으로 다시 생성한다.
3. 처음 결과와 재생성 결과의 실루엣과 surface 수를 비교한다.

판정: 같은 seed는 동일하고, 다른 seed는 높이·회전·가지 배치가 달라야 한다. 나무가 붕괴하거나 전혀 다른 archetype처럼 바뀌면 실패다.

## TC-03 Palm 잎 표시

권장값:

```text
archetype = Palm
seed = 1207
palm_frond_count = 12
palm_frond_length = 3.2
palm_frond_width = 1.0
palm_frond_droop = 0.9
```

1. 상단에서 방사형 frond 수와 방향을 확인한다.
2. 측면에서 길이, 폭, 처짐을 확인한다.
3. foliage 재질을 제거했다 다시 지정해 surface 연결을 확인한다.
4. culling Disabled와 Back을 비교한다.

판정: 12개 안팎의 녹색 리본형 frond가 양면에서 보여야 한다. 현재 구현은 개별 소엽이 달린 야자잎이 아니라 곡면 ribbon이므로, 얇거나 단순해 보이는 것은 품질 한계다. surface 자체가 없거나 재질을 지정해도 전혀 안 보이면 버그다.

## TC-04 뿌리 스타일

같은 broadleaf profile에서 `root_style`을 `None`, `Flare`, `Buttress`, `Prop` 순서로 바꾼다.

- None: 추가 뿌리가 없어야 한다.
- Flare: 줄기 밑동이 자연스럽게 넓어져야 한다.
- Buttress: 판근이 줄기 주변에 분산되어야 한다.
- Prop: 지지 뿌리가 바깥쪽 지면까지 내려가야 한다.

판정: 뿌리가 줄기에서 분리돼 떠 있거나 지면 아래에 대부분 묻히면 실패다.

## TC-05 수관 형태

같은 seed에서 `crown_shape`을 `Round`, `Umbrella`, `Conical`, `Tiered`로 바꾼다.

판정: Round는 구형, Umbrella는 넓고 납작함, Conical은 위로 좁아짐, Tiered는 층 분리가 보여야 한다. `Auto`는 archetype 기본 형태와 일치해야 한다.

## TC-06 2차 가지와 처짐

1. `secondary_branch_count`를 `0`, `1`, `2`로 비교한다.
2. `secondary_branch_scale = 0.5`로 고정한다.
3. `branch_droop`을 `0.0`, `0.6`으로 비교한다.

판정: count 증가에 따라 가지 밀도가 증가하고, droop 증가 시 끝부분이 아래로 이동해야 한다. 자식 가지가 원점으로 연결되거나 부모와 분리되면 실패다.

## TC-07 Surface와 재질

1. 생성 메시의 surface 수를 확인한다.
2. surface 0에 trunk 재질, surface 1에 foliage 재질을 각각 지정한다.
3. 두 재질을 강한 대비색으로 바꾸어 영역이 섞이지 않는지 확인한다.

판정: surface 0은 줄기·가지·뿌리, surface 1은 canopy blob 또는 palm frond만 표시해야 한다.

## TC-08 Bake 및 재로드

1. 정상 생성된 나무에서 `Bake Variant`를 실행한다.
2. 생성된 `OpenWorldTreeVariant`를 저장한다.
3. 씬을 저장하고 에디터를 다시 연다.
4. variant 메시, 재질, bounds, 선택 가능 여부를 확인한다.

판정: 재생성 없이 같은 외형이 복구되고 씬 뷰에서 클릭할 수 있어야 한다.

## TC-09 Species/Placement 연결

1. Bake한 variant를 `OpenWorldTreeSpecies`에 등록한다.
2. 서로 다른 seed의 variant를 최소 3개 등록한다.
3. placement로 여러 나무를 배치한다.
4. 배치 후 개별 상호작용 대상과 렌더 메시의 연결을 확인한다.

판정: 등록된 variant가 섞여 배치되고, 나무별 게임플레이 객체를 식별할 수 있어야 한다.

## 권장 실행 순서

1. TC-01로 수형 전체 회귀를 확인한다.
2. TC-03 Palm을 먼저 검사해 foliage 문제를 분류한다.
3. TC-02로 18개 seed 샘플을 만든다.
4. TC-04~07로 개별 생성 기능을 검사한다.
5. TC-08~09로 저장 및 실제 사용 흐름을 검사한다.

버그는 [OpenWorld Tree 버그 리포트 양식](open_world_tree_bug_report_template.md)에 한 현상씩 기록한다.
