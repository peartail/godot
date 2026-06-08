# SimpleTerrain Module Overview

## 목적

`modules/simple_terrain`은 Godot 엔진에 내장되는 심플 3D 터레인 모듈이다.

기존 `Terrain3D` 구현은 `SimpleTerrain3D`로 이름을 바꾸고, 앞으로 만들 대형 지형 실험 모듈인 `OpenWorldTerrain`과 분리한다.

## 모듈 구조

```text
modules/simple_terrain/
  SCsub
  config.py
  register_types.h
  register_types.cpp
  simple_terrain_data.h
  simple_terrain_data.cpp
  simple_terrain_3d.h
  simple_terrain_3d.cpp
  editor/
    simple_terrain_editor_plugin.h
    simple_terrain_editor_plugin.cpp
  doc_classes/
    SimpleTerrain3D.xml
    SimpleTerrainData.xml
```

## 런타임 클래스

`SimpleTerrainData`

- `Resource` 클래스다.
- 그리드 크기, 셀 크기, 높이 배열을 저장한다.
- 높이 배열은 `z * (grid_size + 1) + x` 순서로 저장한다.
- `.simpleterraindata` 리소스로 저장할 수 있다.

`SimpleTerrain3D`

- `MeshInstance3D` 기반 노드다.
- `SimpleTerrainData`를 직접 만들거나 외부 리소스로 할당받는다.
- 청크마다 별도의 `ArrayMesh`를 생성한다.
- 내부 `RenderingServer` 인스턴스로 청크를 렌더링한다.
- 브러시, 피킹, 지형 생성, 디버그, 머티리얼 API를 제공한다.

## 에디터 클래스

`SimpleTerrainEditorPlugin`

- 에디터 빌드에서만 등록된다.
- 3D 에디터 메뉴에 터레인 툴바를 추가한다.
- Raise, Lower, Smooth, Flatten, Flat, Random 기능을 제공한다.
- 한 번의 브러시 스트로크를 하나의 undo/redo 액션으로 묶는다.

`SimpleTerrain3DGizmoPlugin`

- 3D 뷰포트에 청크 경계를 그린다.
- `SimpleTerrain3D.get_chunk_debug_lines()`를 사용한다.

## 현재 구현 범위

구현됨:

- 높이필드 데이터 리소스.
- 평지 초기화와 랜덤 생성.
- 청크 기반 메시 렌더링.
- 브러시 편집 후 영향 청크만 재생성.
- DDA 기반 브러시 레이 피킹.
- 에디터 툴바와 청크 기즈모.
- 기본 높이 기반 triplanar 머티리얼.
- ClassDB와 doc XML 공개.

아직 미구현:

- 터레인 충돌 생성.
- 런타임 LOD.
- 청크 경계 노멀 보정.
- 편집 가능한 splatmap painting.
- OpenWorldTerrain GPU displacement.
