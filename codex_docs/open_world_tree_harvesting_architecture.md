# OpenWorld Tree 벌목 상호작용 구조

## 목적

대량 정적 렌더링 성능을 유지하면서 플레이어가 개별 나무를 선택하고 벌목할 수 있도록 정적 표현과 상호작용 표현을 분리한다.

## 권장 노드 구조

```text
HarvestableTree3D
├─ MeshInstance3D
│  └─ ArrayMesh
│     ├─ Trunk Surface
│     └─ Foliage Surface
├─ StaticBody3D 또는 AnimatableBody3D
│  └─ CollisionShape3D
└─ InteractionArea3D (Area3D)
   └─ CollisionShape3D
```

1차 interaction 구현에서는 뿌리, 자동 LOD, 바람 애니메이션, 완벽한 가지 접합, 개별 잎, 생물학적 성장 시뮬레이션을 요구하지 않는다. 먼저 seed가 다른 나무들이 안정적으로 다른 실루엣을 만들고 정적 메시로 Bake되는 현재 기반을 유지한다.

## 정적 표현과 상호작용 표현

평상시에는 `OpenWorldTree3D`가 나무를 `MultiMesh` instance로 렌더링한다. 모든 나무에 `StaticBody3D`와 `Area3D`를 만들지 않는다.

플레이어가 나무에 접근하거나 벌목을 시작할 때만 다음 순서로 전환한다.

```text
OpenWorldTree3D static instance
            ↓ stable instance ID 선택
set_tree_enabled(instance_id, false)
            ↓
HarvestableTree3D 생성
            ↓
체력 / 타격 / 낙하 / 자원 생성
            ↓
harvested 상태를 stable ID에 저장
```

상호작용이 취소되고 원본 나무가 유지되면 `HarvestableTree3D`를 제거하고 정적 instance를 다시 활성화할 수 있다.

## `HarvestableTree3D` 책임

- 원본 stable instance ID 보관
- `OpenWorldTreeVariant` 또는 baked mesh 참조
- 단순 trunk collision 구성
- interaction range 제공
- 체력과 damage 처리
- 낙하 방향과 상태 관리
- 벌목 완료 이벤트 발생
- log/resource drop 위치 제공

담당하지 않는 기능:

- 대량 배치 렌더링
- Species seed 선택
- runtime 절차 메시 재생성
- 임의 메시 절단
- 건축물 메시 직접 생성

## 상태 모델 제안

```text
STATIC
  → PROMOTED
  → DAMAGED
  → FALLING
  → HARVESTED

PROMOTED
  → STATIC  (상호작용 취소 및 손상 없음)
```

최소 저장 데이터:

- stable instance ID
- harvested 여부
- 남은 체력 또는 damage 단계
- respawn 시간 또는 영구 제거 여부

낙하 중의 매 프레임 물리 상태까지 영구 저장할 필요는 없다. 로드 시 `DAMAGED`, `HARVESTED` 같은 안정 상태로 복구하는 것이 단순하다.

## 충돌 전략

초기에는 `OpenWorldTreeVariant.collision_radius`와 `collision_height`로 capsule 또는 cylinder proxy를 만든다.

- 정적 렌더 단계: 주변 interaction query용 spatial index만 사용
- 승격 단계: trunk collision과 interaction area 생성
- 낙하 단계: 필요하면 `RigidBody3D` 또는 animation 기반 body로 교체
- foliage와 개별 가지에는 기본 collision을 만들지 않음

## 벌목 결과

메시 절단 대신 규격화된 결과물을 사용한다.

- 통나무 scene
- 가지 묶음 또는 목재 resource
- 나무 종류/등급/수분 같은 metadata
- 건축 시스템의 표준 부품 또는 inventory item

이 구조라면 나무 실루엣이 달라도 건축 시스템이 임의 절단 메시 형상에 의존하지 않는다.

## 완료 기준

- stable ID로 특정 정적 나무를 승격할 수 있다.
- 승격 중 원본과 interactive mesh가 동시에 보이지 않는다.
- 벌목을 취소하면 정적 표현으로 복귀할 수 있다.
- 벌목 완료 후 정적 instance가 다시 나타나지 않는다.
- 저장/로드 후 harvested 상태가 유지된다.
- interaction 범위 밖 나무는 개별 PhysicsBody 비용을 사용하지 않는다.

## 관련 문서

- [트리 시스템 문서 인덱스](open_world_tree_system_status_and_roadmap.md)
- [현재 구현 현황](open_world_tree_system_current_status.md)
- [생성 및 렌더링 로드맵](open_world_tree_generation_roadmap.md)
