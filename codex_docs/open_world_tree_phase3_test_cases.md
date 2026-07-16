# OpenWorld Tree 3차 테스트 케이스

## 목적

LOD0/LOD1/LOD2 생성, 전환, Wind 데이터, 카툰 Wind 재질, 일괄 Bake를 엔진 에디터와 실행 화면에서 검증한다.

샘플 씬 구성은 [3차 테스트 샘플 구성](open_world_tree_phase3_test_sample_setup.md)을 따른다.

## 환경 기록

```text
Engine build/commit:
Renderer: Forward+ / Mobile / Compatibility
GPU / OS:
Scene:
Archetype / Seed:
Camera FOV:
Wind enabled:
```

## 공통 합격 기준

- 생성·LOD 전환·Bake·재로드 중 오류나 크래시가 없다.
- 모든 LOD에 trunk/foliage 두 surface가 존재한다.
- LOD1 triangle 수는 LOD0보다 작다.
- LOD2 triangle 수는 LOD1보다 작다.
- LOD 전환 시 pivot, 높이, 주요 수관 폭이 유지된다.
- Wind 적용 중 밑동과 root가 지면에서 크게 움직이지 않는다.
- 같은 profile과 seed의 결과는 재생성 후 동일하다.

## TC-01 세 LOD 생성

1. `generate_lod1`, `generate_lod2`를 켠다.
2. `Generate Tree`를 실행한다.
3. `preview_lod`를 LOD0, LOD1, LOD2로 바꾼다.
4. Inspector 하단 통계를 기록한다.

판정:

- 세 LOD가 모두 표시된다.
- LOD가 낮아질수록 branch와 topology가 단순해진다.
- 나무 높이와 지면 중심 pivot은 유지된다.

## TC-02 LOD 비활성화와 fallback

1. `generate_lod2 = false`로 생성한다.
2. `preview_lod = LOD2`를 선택한다.
3. `generate_lod1 = false`도 적용하고 다시 확인한다.

판정: 없는 preview LOD는 더 상세한 LOD 방향으로 fallback하며 빈 화면이나 오류가 발생하지 않는다.

## TC-03 Quality 경계값

다음 조합을 각각 생성한다.

| 조합 | lod1_quality | lod2_quality |
|---|---:|---:|
| 기본 | 0.55 | 0.18 |
| 고품질 | 0.80 | 0.50 |
| 최저 | 0.10 | 0.05 |

판정: 최저값에서도 삼각형 폭발, 빈 surface, 원점으로 뻗는 면이 없어야 한다. 품질값 증가 시 triangle 수가 대체로 증가해야 한다.

## TC-04 Archetype LOD 회귀

seed `1207`로 다음 6종을 검사한다.

| Archetype | LOD 중점 확인 |
|---|---|
| Temperate Broadleaf | 둥근 수관 부피 |
| Tropical Broadleaf | 넓은 상부 수관과 판근 |
| Umbrella | 수평 폭과 납작한 수관 |
| Conifer | 원뿔형과 층별 가지 |
| Palm | frond 방향, 길이, 처짐 |
| Mangrove | prop root와 낮은 수관 |

판정: LOD2에서도 수형만 보고 archetype을 구분할 수 있어야 한다.

## TC-05 Seed 결정성

1. 각 LOD의 vertex/triangle 통계를 기록한다.
2. 같은 seed로 두 번 다시 생성한다.
3. `1207`, `2402`, `8801`을 비교한다.

판정: 같은 seed의 통계와 형상은 같고, 다른 seed는 실루엣이 달라야 한다. 한 나무의 LOD마다 별개의 변형처럼 보여서는 안 된다.

## TC-06 수동 거리 전환

1. `lod1_distance = 15`, `lod2_distance = 30`, `max_distance = 50`으로 Bake한다.
2. `OpenWorldTree3D`에 카메라를 연결한다.
3. 카메라를 0~60m 구간에서 천천히 이동한다.

판정:

- 0~15m: LOD0
- 15~30m: LOD1
- 30~50m: LOD2
- 50m 이후: 렌더 제외
- 경계에서 크기 점프, 순간 회전, 한 프레임 중복 표시가 없어야 한다.

## TC-07 대량 LOD 전환

1. 같은 Species로 100개 이상 배치한다.
2. 카메라를 군락 안팎으로 이동한다.
3. `lod_update_interval`을 기본값과 `0.05`, `0.5`로 비교한다.

판정: 거리별로 여러 LOD bucket이 함께 표시되고, 이동 중 누락·복제·잔상이 없어야 한다. 에디터 종료 시 크래시가 없어야 한다.

## TC-08 Wind 기본 동작

권장값:

```text
wind_enabled = true
wind_strength = 0.22
wind_speed = 1.0
wind_gust_strength = 0.35
wind_direction = (1.0, 0.35)
```

판정:

- trunk 상단이 밑동보다 많이 움직인다.
- branch/frond 끝이 시작점보다 많이 움직인다.
- foliage에 작은 flutter가 보인다.
- root와 밑동은 안정적으로 고정된다.

## TC-09 Wind 극단값

1. `wind_strength`를 `0`, `0.22`, `1.0`, `2.0`으로 비교한다.
2. `wind_speed`를 `0`, `1`, `8`로 비교한다.
3. `wind_gust_strength`를 `0`, `0.35`, `2.0`으로 비교한다.
4. `wind_direction`을 X, Z, 대각선으로 바꾼다.

판정: 0에서는 해당 효과가 멈추고 방향 변경이 즉시 반영된다. 최대값에서도 NaN, 메시 폭발, 화면 전체 번쩍임이 없어야 한다.

## TC-10 LOD 간 Wind 연속성

Wind를 켜고 TC-06 거리 경계를 반복 통과한다.

판정: 전환 순간 나무가 반대 방향으로 튀거나 바람 phase가 크게 재시작하지 않아야 한다. 디테일 감소는 허용하지만 전체 굽힘 방향은 유지돼야 한다.

## TC-11 재질 전환

1. `wind_enabled = false`에서 사용자 trunk/foliage 재질을 확인한다.
2. Wind를 켜 기본 단색 재질을 확인한다.
3. 다시 끄고 사용자 재질로 복귀하는지 확인한다.

판정: surface 0/1 재질이 뒤바뀌지 않고 토글 후 즉시 정상 표시된다.

## TC-12 Bake와 재로드

1. Wind와 세 LOD가 생성된 상태에서 `Bake Variant...`를 실행한다.
2. Variant와 씬을 저장하고 에디터를 재시작한다.
3. 세 mesh, 거리, seed, material, collision hint를 확인한다.

판정: 재생성 없이 동일 외형과 거리 전환이 복구돼야 한다.

## TC-13 기존 API 회귀

1. `get_generated_mesh()`를 호출한다.
2. 기존 1·2차 코드로 LOD0을 사용한다.
3. `clear_generated_tree()` 후 세 LOD를 확인한다.

판정: `get_generated_mesh()`는 LOD0을 반환하고 Clear는 모든 LOD와 preview를 제거한다.

## 버그 기록

한 현상씩 [Tree 버그 리포트 양식](open_world_tree_bug_report_template.md)에 기록한다.

추가 분류 코드:

- `LOD-GEN`: LOD 생성 또는 triangle 감소 오류
- `LOD-POP`: 거리 전환 실루엣 popping
- `LOD-BUCKET`: MultiMesh LOD 배정 오류
- `WIND-DATA`: vertex color 가중치 오류
- `WIND-SHD`: shader 표시 또는 변형 오류
- `WIND-LOD`: LOD 전환 시 wind 불연속
