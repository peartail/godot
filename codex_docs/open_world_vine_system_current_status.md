# OpenWorld 덩굴 시스템 현재 구현 현황

## 목표

AI agent가 마우스 없이 텍스트 리소스와 공개 API로 카툰 스타일 덩굴을 생성·검증·Bake하도록 구성한다.

```text
Profile + Request + Support
            ↓
       VinePathData
            ↓
  LOD0 / LOD1 / LOD2 Mesh
            ↓
        VineVariant
            ↓
      OpenWorldVine3D
```

## 구현 클래스

### `OpenWorldVineGenerationProfile`

- low-poly stem과 taper
- side shoot와 leaf card
- LOD별 다중 카드 leaf cluster
- 선택형 stem thorn decoration
- branch collar 기반 표면 접합 보강
- support projection 규칙
- gap, slope, Hanging, TreeWrap과 Bramble 설정
- LOD와 Wind 설정

### `OpenWorldVineGenerationRequest`

- Creeping, Climbing, Hanging, TreeWrap, Bramble mode
- profile, seed, 시작·목표·길이
- 상대 support `NodePath`와 stable ID
- 마우스를 대체하는 explicit anchor/normal

에디터에서 생성한 Request는 Profile을 자동 생성한다. 코드에서 직접 생성할 때는 Profile을 명시적으로 지정한다.

### `OpenWorldVinePathData`

- primary와 side path
- anchor별 normal과 attached flag
- parent path와 tree support segment
- 전체 polyline 길이

### `OpenWorldVineGenerator3D`

- `validate_request()` 구조화된 검증
- mode별 결정론적 path solver
- MeshInstance TriangleMesh projection
- OpenWorldTerrain height-field projection
- Tree support graph 기반 TreeWrap
- support 없는 타원형 자가 얽힘 Bramble
- parallel-transport low-poly stem
- 양면 leaf card
- LOD0/1/2와 Wind vertex color
- Variant Bake와 machine-readable report

### `OpenWorldVineVariant`

- 세 LOD mesh와 거리
- 원본 mode와 seed
- support stable ID
- KEEP/HIDE/DETACH 정책

### `OpenWorldVine3D`

- 단일 고유 덩굴의 camera distance LOD
- max distance culling
- support-loss 정책 처리
- DETACH gameplay signal

### `OpenWorldTreeSupportGraph`

- trunk와 primary branch 중심선
- 구간별 radius
- parent와 attachment ratio
- Tree Variant Bake 연동

## 메시 규약

| 항목 | 규약 |
|---|---|
| Surface 0 | Stem과 side shoot |
| Surface 1 | Leaf card |
| Stem UV | 둘레 U, 누적 길이 V |
| Leaf UV | 0~1 |
| Vertex R | 고정점부터 전체 굽힘 |
| Vertex G | side shoot 굽힘 |
| Vertex B | leaf flutter |
| Vertex A | seed phase |

LOD2 stem은 양면 ribbon이며 leaf 간격을 확대한다.

## Agent-First 작업 흐름

1. `.tres` 또는 코드로 Profile과 Request 작성
2. Headless에서 `validate_request()` 실행
3. `generate_vine()` 실행
4. `get_generation_report()`를 JSON으로 확인
5. `create_baked_variant()` 실행
6. `ResourceSaver.save()`와 재로드 검증

Inspector 버튼은 같은 공개 API를 호출하는 선택 기능이다.

## 구현된 편의 기능

- Validate Vine Request
- Generate Vine
- Randomize Seed
- Bake Vine Variant
- Inspector JSON report
- XML class reference와 agent docs 노출
- GDScript/C# Headless 예제
- 자동·수동 테스트 케이스

## 현재 제한

- runtime 성장과 surface 재탐색 없음
- 임의 imported tree stem 분석 없음
- TreeWrap은 생성 트리 support graph만 사용
- branch collar는 겹침 기반이며 watertight boolean 접합은 아님
- 꽃과 열매 decoration 없음
- cell batching과 streaming 없음
- DETACH 실제 물리는 gameplay 시스템 책임
- runtime 절단과 잎별 collision 없음

## 검증 상태

- Windows Mono editor, tests enabled, `-j1` 빌드 성공
- 엔진 전체 1,373 test cases 성공
- 전체 427,055 assertions 성공
- Mono glue 및 Debug/Release API assemblies 재생성 성공
- Headless 초기화와 agent class docs 조회 성공
- ANGLE dependency 안내와 기존 draw-context 메시 외 덩굴 실패 없음

## 관련 문서

- [개발 플랜](open_world_vine_system_plan.md)
- [Agent-First 제작 지침](open_world_vine_agent_workflow.md)
- [Agent API 가이드](open_world_vine_agent_api.md)
- [테스트 케이스](open_world_vine_test_cases.md)
- [테스트 샘플 생성](open_world_vine_test_sample_setup.md)
- [BRAMBLE 생성 가이드](open_world_vine_bramble_guide.md)
