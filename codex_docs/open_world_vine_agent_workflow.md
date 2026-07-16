# OpenWorld 덩굴 Agent-First 제작 지침

## 기본 원칙

덩굴 제작의 기본 사용자는 AI agent다. 마우스로 점을 배치하거나 Curve를 드래그하지 않아도 입력 작성, 경로 계산, 검증, Bake와 테스트를 완료할 수 있어야 한다.

GUI는 생성 결과를 확인하고 예외적으로 미세 조정하는 보조 수단이다. Inspector 버튼, Gizmo, viewport picking 또는 파일 대화상자만으로 가능한 기능은 완료된 기능으로 취급하지 않는다.

## Source of Truth

원본 데이터는 텍스트 diff와 재생성이 가능한 리소스로 유지한다.

```text
OpenWorldVineGenerationProfile.tres
OpenWorldVineGenerationRequest.tres
OpenWorldVinePathData.tres
             ↓ derived
OpenWorldVineVariant.tres + baked ArrayMesh
```

- Profile: 형태와 스타일 규칙
- Request: 어디에서 어떤 방식으로 생성할지 지정
- PathData: 계산된 anchor와 support binding
- Variant: 게임에서 사용하는 파생 결과

Binary mesh만 남기지 않는다. Agent가 Profile, Request와 seed로 결과를 다시 만들 수 있어야 한다.

## `OpenWorldVineGenerationRequest` 제안

최소 입력:

| Field | 역할 |
|---|---|
| `mode` | Creeping, Climbing, Hanging, TreeWrap |
| `seed` | 결정론적 변형 seed |
| `profile` | 생성 규칙 리소스 |
| `start_position` | 로컬 또는 월드 시작점 |
| `start_direction` | 초기 진행 방향 |
| `target_position` | 선택적 목표점 |
| `desired_length` | 목표 경로 길이 |
| `support_path` | 지면, MeshInstance, Tree node의 `NodePath` |
| `branch_budget` | 최대 보조 줄기 수 |
| `output_path` | Variant 저장 경로 |

정밀 제어가 필요하면 `PackedVector3Array explicit_anchors`를 제공한다. 이는 마우스 편집의 텍스트 대체 수단이며, 빈 배열이면 mode별 solver가 경로를 계산한다.

## Mode별 계산 입력

### Creeping

- 시작점, 전진 방향, 길이와 지면 `NodePath`
- 각 step에서 아래 방향 support query
- slope, surface offset, turn noise와 obstacle 정책

### Climbing

- 시작점, 목표 방향과 support object `NodePath`
- 표면 접선의 상승 성분과 seed noise 계산
- 최대 surface gap을 넘으면 종료하거나 Hanging으로 전환

### Hanging

- 시작점, 초기 속도 방향, 길이와 gravity bias
- 수치 기반 catenary 근사 또는 제어점 계산
- 시작 구간만 support binding을 유지

### TreeWrap

- Tree stable ID 또는 node path
- trunk/branch 중심선과 radius를 제공하는 support graph
- 감김 횟수, 진행 높이, 가지 전환 확률과 radial offset

## 필수 API 형태

정확한 이름은 구현 시 확정하되 다음 작업은 독립 호출 가능해야 한다.

```text
generate_path(request) -> OpenWorldVinePathData
validate_path(path_data) -> Dictionary
generate_mesh(profile, path_data, lod) -> ArrayMesh
bake_variant(request, save_path) -> OpenWorldVineVariant
get_generation_report() -> Dictionary
```

API는 에디터 플러그인에 종속되지 않아야 한다. Tool mode나 headless scene에서도 같은 계산 코드를 호출한다.

## Headless 작업 흐름

```text
1. 텍스트 Profile과 Request 생성
2. Headless Godot에서 request 로드
3. path 생성 및 validation
4. LOD mesh 생성과 Variant Bake
5. 저장 결과 재로드
6. hash, bounds, surface, triangle과 warning 출력
```

각 단계는 실패 원인을 구조화된 `Dictionary` 또는 JSON으로 반환한다. 성공 여부를 editor popup이나 화면 상태로만 전달하지 않는다.

## 결정론 규칙

- 난수는 명시적 seed와 고정된 stream 순서를 사용한다.
- support sample 간격과 iteration 상한을 Profile에 저장한다.
- 입력 노드 순회 순서에 의존하지 않는다.
- 동일 입력에서 anchor 수, 위치, mesh hash와 통계가 같아야 한다.
- 자동 보정이 발생하면 보정 내용과 횟수를 report에 기록한다.

## Agent가 다루기 쉬운 Validation

최소 report 항목:

- 요청 mode, seed와 profile 경로
- 생성/거부된 anchor 및 branch 수
- 전체 길이와 surface gap 최댓값
- self-intersection과 뒤집힌 frame 수
- LOD별 vertex/triangle/surface 수
- local AABB와 material 누락
- warning/error code와 관련 anchor index

오류 메시지는 해결 가능한 파라미터 이름과 허용 범위를 포함한다.

## 샘플과 테스트 규칙

- 샘플 씬은 스크립트나 테스트가 처음부터 재생성할 수 있어야 한다.
- Creeping, Climbing, Hanging, TreeWrap request를 텍스트 fixture로 제공한다.
- 정상, 급경사, surface gap, 잘못된 support와 zero-length 입력을 자동 테스트한다.
- 시각 검토용 screenshot은 agent가 실행할 수 있으나 필수 검증은 수치와 hash로 수행한다.
- 공개 ClassDB API에는 XML 문서와 GDScript/C# 예제를 함께 제공한다.

## 선택적 GUI

다음 기능은 편의를 위해 제공할 수 있다.

- 계산된 anchor/normal/path Gizmo
- 선택 anchor 이동과 재투영
- seed preview grid
- Generate, Validate와 Bake 버튼

모든 GUI 동작은 동일한 공개 API를 호출해야 하며 GUI에 별도 생성 로직을 두지 않는다.

## 완료 기준

- 빈 씬에서 agent가 텍스트 파일과 명령만으로 네 종류의 덩굴을 생성한다.
- 마우스 입력 없이 Variant 저장과 재로드 검증이 끝난다.
- 실패 request가 editor를 열지 않고 원인과 수정 방법을 출력한다.
- GUI와 headless 결과가 같은 seed에서 동일하다.
