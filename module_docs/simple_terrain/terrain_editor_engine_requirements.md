# Godot Terrain Editor Engine Requirements

## 목적

Godot 엔진에 터레인 편집 기능을 C++ 모듈 형태로 추가한다.

초기 구현은 `SimpleTerrain`으로 유지하고, 대형 월드 지형 실험은 별도 `OpenWorldTerrain` 모듈에서 진행한다.

## 모듈 분리 원칙

`SimpleTerrain`

- 기존 CPU heightfield 기반 편집 터레인.
- 청크 단위 CPU mesh 생성.
- 에디터 브러시 편집.
- 기본 triplanar material.
- 작은/중간 규모 지형과 기능 검증용 기준 구현.

`OpenWorldTerrain`

- 새로 만들 대형 월드 지형 실험 모듈.
- height texture + GPU displacement 우선 검증.
- 장기적으로 tile streaming, clipmap, virtual texture 구조 검토.
- 기존 `SimpleTerrain` 기능을 깨지 않고 별도 개발.

## SimpleTerrain 요구사항

- 노드 이름은 `SimpleTerrain3D`로 한다.
- 데이터 리소스 이름은 `SimpleTerrainData`로 한다.
- 모듈 경로는 `modules/simple_terrain`으로 한다.
- 초기 지형은 랜덤이 아니라 평평한 상태여야 한다.
- 랜덤 지형 생성은 명시적 버튼/API로만 수행한다.
- 높이 데이터는 `(grid_size + 1) * (grid_size + 1)` 배열로 저장한다.
- 브러시 편집은 변경된 청크만 갱신해야 한다.
- 브러시 undo는 전체 height array가 아니라 변경 vertex delta를 우선 사용한다.
- 청크 경계는 에디터 gizmo로 볼 수 있어야 한다.
- ClassDB, Inspector, GDScript, C#에서 사용할 수 있어야 한다.

## 에디터 요구사항

- `SimpleTerrain3D` 선택 시 3D editor toolbar가 표시된다.
- Terrain mode toggle을 제공한다.
- Raise, Lower, Smooth, Flatten 브러시를 제공한다.
- Flat, Random 버튼을 제공한다.
- 한 번의 paint stroke는 하나의 undo action으로 기록한다.
- `show_chunk_gizmos`가 켜지면 청크 경계를 표시한다.

## OpenWorldTerrain 초기 요구사항

- 기존 `SimpleTerrain` 코드를 직접 변경하지 않고 새 모듈로 만든다.
- 첫 목표는 GPU displacement 렌더링 경로 검증이다.
- CPU height data와 GPU height texture를 동기화한다.
- 피킹/브러시 계산은 초기에는 CPU height data를 사용해도 된다.
- shader vertex 단계에서 height texture를 샘플링해 grid mesh를 변위한다.
- collision, streaming, LOD는 1차 실험 이후 단계로 둔다.

## 비범위

현재 단계에서 다음은 필수 구현이 아니다.

- production-grade terrain streaming.
- 완성형 quadtree LOD.
- terrain collision 자동 생성.
- splatmap painting.
- biome system.
- 서버/네트워크 동기화.
