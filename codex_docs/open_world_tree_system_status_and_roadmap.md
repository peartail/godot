# OpenWorld Tree System 문서 인덱스

## 문서 구성

기존 통합 문서는 180줄 이하의 주제별 문서로 분리했다.

- [현재 구현 현황](open_world_tree_system_current_status.md): 구현된 클래스, 데이터 흐름, 1차 완료 범위, 테스트와 제한 사항
- [생성 및 렌더링 로드맵](open_world_tree_generation_roadmap.md): 열대 수형을 포함한 Phase 1.5~6 개발 계획
- [벌목 상호작용 구조](open_world_tree_harvesting_architecture.md): `HarvestableTree3D`, stable ID, 정적/interactive 전환
- [스타일라이즈드 생성기 1차 가이드](godot_stylized_tree_generator_mvp.md): 파라미터 설명과 에디터/API 사용법
- [스타일라이즈드 생성기 2차 가이드](godot_stylized_tree_generator_phase2.md): Archetype, 열대 수형, root, Palm frond
- [스타일라이즈드 생성기 3차 가이드](godot_stylized_tree_generator_phase3.md): LOD0/1/2 Bake, Wind 데이터와 카툰 preview
- [3차 테스트 케이스](open_world_tree_phase3_test_cases.md): LOD, Wind, Bake 수동 검증 항목
- [3차 테스트 샘플 구성](open_world_tree_phase3_test_sample_setup.md): 단일 나무, 거리 표식, 100개 군락 구성
- [덩굴 시스템 개발 플랜](open_world_vine_system_plan.md) / [Agent-First 제작 지침](open_world_vine_agent_workflow.md): 텍스트·Headless 중심 생성, 표면 부착과 Bake 계획
- [덩굴 현재 구현 현황](open_world_vine_system_current_status.md) / [Agent API](open_world_vine_agent_api.md) / [테스트 케이스](open_world_vine_test_cases.md)
- [Blender 나무 제작 가이드](blender_stylized_tree_asset_guidelines.md): Blender 보조 자산 제작 규칙
- [Blender GLB 익스포트 가이드](blender_tree_glb_export_guide.md): Godot 연결용 익스포트 설정

## 현재 방향 요약

```text
Profile + seed
      ↓
OpenWorldTreeGenerator3D
      ↓ Bake
OpenWorldTreeVariant
      ↓
OpenWorldTreeSpecies + OpenWorldTreePlacementData
      ↓
OpenWorldTree3D 정적 MultiMesh
      ↓ 상호작용 대상만 승격
HarvestableTree3D
```

- Godot에서 나무를 생성하고 정적 Variant로 Bake한다.
- 대량 나무는 `OpenWorldTree3D`가 렌더링한다.
- Stable instance ID가 배치와 벌목 상태를 연결한다.
- 상호작용 대상만 `HarvestableTree3D`로 전환한다.
- 메시 절단은 제외하고 규격화된 통나무/자원을 생성한다.

## 열대 나무 방향

열대 나무는 구현 가능하며 2차 수형 확장 범위에 포함한다.

- 현재 구조로 근사: Tropical Broadleaf, Umbrella/Savanna crown
- 전용 foliage module 필요: Palm frond
- 전용 root module 필요: Buttress root, Mangrove prop root, Banyan aerial root

상세 설계와 완료 기준은 [생성 및 렌더링 로드맵](open_world_tree_generation_roadmap.md)의 Phase 2를 참고한다.

## 현재 개발 단계

- Phase 1~3: 생성, 열대 수형, LOD, Wind와 정적 Bake 완료
- Phase 3.5: 프리셋, 실제 게임용 material, UV/normal 규약과 제작 도구 보강 예정
- Phase 4: `HarvestableTree3D`와 벌목 상태 전환 예정
- Phase 5: cell streaming, spatial index와 benchmark 예정
- Phase 6: biome/scatter 기반 월드 배치 도구 예정

다음 구현 주제인 덩굴은 나무 생성기에 포함하지 않는다. 공통 seed/Bake/LOD 규약은 재사용하되 별도 profile, generator, variant를 갖는다.

## 권장 다음 단계

1. Phase 3.5 프리셋과 게임용 material 마감
2. `HarvestableTree3D` 최소 기능과 stable ID 상태 저장
3. 실제 숲 benchmark 후 billboard/streaming 필요성 결정
4. 덩굴 시스템 Phase 0~1 프로토타입 병행

## 문서 길이 규칙

- Markdown 문서가 180줄을 넘으면 역할별 분리를 우선한다.
- 인덱스 문서는 짧은 요약과 링크만 유지한다.
- 공개 API 변경 시 XML 클래스 문서와 Mono glue를 함께 갱신한다.
- 결정론, Bake, stable ID 변경 시 단위 테스트를 갱신한다.
