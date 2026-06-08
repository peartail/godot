# Simple Terrain Generator Requirements

## 목적

`SimpleTerrainGenerator`는 엔진 내장 터레인 개발 전에 기능을 빠르게 검증하기 위한 Godot 프로젝트용 프로토타입이다.

현재 엔진 구현은 이 프로토타입의 결과를 바탕으로 `SimpleTerrain3D`와 `SimpleTerrainData` 모듈로 정리되었다.

## 주요 기능

- height data 기반 terrain 생성.
- grid size와 cell size 설정.
- flat terrain 초기화.
- 명시적 random terrain 생성.
- 3D editor에서 브러시 편집.
- mesh rebuild와 material 확인.

## 데이터 요구사항

- height data가 단일 source of truth다.
- 크기는 `(grid_size + 1) * (grid_size + 1)`이다.
- terrain은 origin 중심으로 배치한다.
- scene/resource 저장 가능성을 검증한다.

## 에디터 요구사항

- terrain 선택 시 toolbar를 표시한다.
- terrain이 아닌 노드 선택 시 toolbar를 숨긴다.
- Raise, Lower, Smooth, Flatten 브러시를 제공한다.
- brush hit test는 editor camera ray를 사용한다.

## 엔진 이관 기준

프로토타입 개념은 다음 엔진 클래스에 대응한다.

- `SimpleTerrainGenerator` -> `SimpleTerrain3D`
- `SimpleTerrainData` -> `SimpleTerrainData`
- `height_data` -> `SimpleTerrainData.height_data`
- editor plugin -> `SimpleTerrainEditorPlugin`

## 비범위

- 대형 terrain streaming.
- OpenWorldTerrain GPU displacement.
- production terrain file format.
- terrain collision 완성.
- LOD renderer 완성.
