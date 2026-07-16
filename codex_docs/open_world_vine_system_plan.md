# OpenWorld 덩굴 시스템 개발 플랜

## 목적

카툰 스타일 덩굴을 Godot에서 seed 기반으로 구성하고 정적 메시로 Bake한다. 벽, 바위, 나무와 지면을 타는 덩굴 및 아래로 늘어지는 덩굴을 같은 데이터 흐름으로 제작하는 것이 목표다.

나무 시스템과 공통으로 유지할 원칙:

- 텍스트 리소스/API로 생성하고 게임 실행 전 정적 리소스로 Bake한다.
- profile은 형태 계열, seed는 같은 계열 안의 변형을 담당한다.
- stem과 foliage를 별도 surface로 유지한다.
- 런타임 절차 성장과 임의 메시 절단은 초기 범위에서 제외한다.
- 마우스 Curve 편집과 Gizmo는 필수가 아닌 선택적 보조 기능이다.

에이전트 중심 입력·명령·검증 규약은 [덩굴 Agent-First 제작 지침](open_world_vine_agent_workflow.md)을 따른다.

## 나무 시스템과의 경계

덩굴은 나무의 branch module로 구현하지 않는다. 지지 표면을 따라가는 경로와 부착 법선이 핵심이므로 별도 시스템으로 구성한다.

재사용 대상:

- seed 기반 결정론
- LOD와 Wind vertex color 규약
- Variant Bake 패턴
- editor preview와 mesh 통계 방식

분리 대상:

- generation profile과 generator
- 표면 탐색 및 경로 데이터
- stem/leaf 메시 생성 규칙
- 배치와 부착 상태

## 제안 데이터 흐름

```text
OpenWorldVineGenerationProfile + seed
                    +
 VineGenerationRequest + support reference
                    ↓
           OpenWorldVinePathData
                    ↓
          OpenWorldVineGenerator3D
                    ↓ Bake
            OpenWorldVineVariant
                    ↓
              OpenWorldVine3D
```

### `OpenWorldVineGenerationProfile`

- stem 반지름, taper, radial sides와 segment 간격
- branch 빈도, 길이, 분기각과 중력 방향성
- leaf 간격, 크기, 회전, 군집 밀도
- surface offset과 부착 강도
- climbing, creeping, hanging 형태 preset
- Wind 강도와 LOD 품질

### `OpenWorldVineGenerator3D`

- 요청과 support query로 중심 경로와 anchor를 계산
- 필요하면 텍스트 anchor 또는 `Curve3D`를 입력 경로로 사용
- 경로를 따라 low-poly tube/ribbon stem 생성
- 결정론적 보조 줄기와 잎 배치
- stem/foliage 두 surface 생성
- LOD, Wind channel, collision hint 생성
- Variant Bake와 editor preview 제공

### `OpenWorldVineVariant`

- LOD0/LOD1/LOD2 mesh
- LOD 거리와 material override
- 원본 seed와 profile 식별 정보
- local bounds와 간단한 interaction/collision metadata

## 덩굴 형태

초기 preset은 다음 세 가지로 제한한다.

| Preset | 경로 특성 | 주요 사용처 |
|---|---|---|
| Climbing | 표면을 따라 위로 성장 | 벽, 나무, 절벽 |
| Creeping | 지면과 낮은 표면을 따라 확장 | 숲 바닥, 폐허 |
| Hanging | 시작점에서 중력 방향으로 처짐 | 가지, 천장, 동굴 |

열대 대형 잎, 꽃, 가시와 판타지 발광체는 leaf/decoration 모듈의 후속 preset으로 취급한다.

## Phase 0 — 기술 검증

- 텍스트 anchor 배열을 입력으로 사용하는 tube/ribbon stem prototype
- 평행 이동 프레임으로 급격한 비틀림 방지
- 단순 leaf card 배치
- stem/foliage 두 surface와 UV 확인
- 동일 profile/seed/request의 결정론 검증

완료 기준:

- 수직벽, 곡면, 늘어진 곡선에서 stem이 뒤집히지 않는다.
- pivot, bounds, material surface가 Bake 후 유지된다.

## Phase 1 — 정적 제작 MVP

- profile, generator, variant와 XML/API 문서
- Climbing/Creeping/Hanging preset
- primary stem과 1단계 side shoot
- leaf card/cluster, 간격과 크기 variation
- editor Generate, Randomize Seed, Bake 액션
- ClassDB generate/validate/bake/report API와 headless sample command
- mesh 통계, parameter validation과 기본 sample scene
- 단위 테스트와 Mono API 노출

완료 기준:

- 에이전트가 텍스트 request로 여러 seed를 생성·비교하고 Variant를 저장할 수 있다.
- 세 preset의 실루엣이 명확히 구분된다.

## Phase 2 — 표면 부착

- anchor 위치와 surface normal 저장
- authoring-time support query로 벽, 바위, 지형과 나무 표면에 projection
- surface offset, 최대 탐색 거리와 경사 제한
- 장애물 또는 표면 이탈 시 경로 중단 규칙
- 원본 지지 오브젝트가 변경됐을 때 재투영 경고

초기 구현은 GUI 유무와 관계없이 제작 시 projection 결과를 저장한다. 런타임마다 물리 raycast로 덩굴을 재생성하지 않는다.

## Phase 3 — 표현 품질과 LOD

- bark/leaf UV와 texture material
- stem, side shoot, leaf용 Wind channel
- LOD1 줄기/잎 간격 축소와 LOD2 ribbon 또는 card
- leaf cluster, flower와 fruit decoration socket
- surface conform 품질과 self-intersection 보정
- 전체 preset visual QA와 benchmark

## Phase 4 — 배치와 선택적 게임플레이

- 반복 배치용 Variant library와 biome palette
- 정적 덩굴 batching 또는 cell streaming
- 선택적 채집, 제거, 재생 상태
- 지지 나무가 벌목될 때 숨김/분리/낙하 정책
- Stable ID와 저장 데이터 연동

상호작용이 필요 없는 덩굴에는 개별 PhysicsBody를 만들지 않는다.

## 초기 제외 범위

- 실시간 성장 시뮬레이션
- 런타임 표면 길찾기
- 덩굴 로프 물리와 완전한 절단
- 잎별 개별 collision
- 임의 메시를 감싸는 자동 UV unwrap

## 권장 구현 순서

1. 텍스트 anchor/request 기반 Phase 0로 메시와 프레임 안정성 검증
2. Phase 1의 독립 리소스/API와 정적 Bake 완성
3. Headless 실행 가능한 제작 시점 surface projection 추가
4. Wind/LOD와 표현 품질 확장
5. 실제 월드 밀도 측정 후 batching과 gameplay 범위 결정

## 테스트 핵심 항목

- 동일 request의 path/mesh hash와 leaf 배치 결정론
- 수직/수평/곡면/급회전 anchor 경로의 프레임 안정성
- stem과 foliage surface 순서, UV와 Wind channel
- Bake/재로드 후 mesh, bounds와 material 유지
- 표면 offset과 normal 방향 일관성
- LOD 감소율과 전환 실루엣
- 대량 배치 시 draw call, triangle과 CPU 비용

## 문서 규칙

- 구현 시 클래스별 XML 문서와 GDScript/C# API 예제를 함께 작성한다.
- 수동 테스트 케이스와 sample scene 구성은 별도 문서로 분리한다.
- 한 Markdown 문서가 180줄을 넘으면 설계, API, 테스트 문서로 나눈다.
