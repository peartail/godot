# Simple Terrain Generator Development Plan

## 목적

프로젝트용 `SimpleTerrainGenerator` 프로토타입을 엔진 모듈 `SimpleTerrain`으로 이관하기 위한 기준 계획이다.

## Prototype Phase

완료 기준:

- grid 기반 mesh 생성.
- flat/random terrain 생성.
- editor brush hit test.
- toolbar 표시/숨김.
- basic material 확인.

## Engine Migration Phase

이관 대상:

- runtime node -> `SimpleTerrain3D`
- data resource -> `SimpleTerrainData`
- editor toolbar -> `SimpleTerrainEditorPlugin`
- chunk debug drawing -> `SimpleTerrain3DGizmoPlugin`

## Stabilization Phase

작업:

- chunk rebuild 범위 최소화.
- brush release 시 undo delta 저장.
- Mono wrapper 생성 확인.
- doc XML 추가.
- module disable build 확인.

## OpenWorld 분리

대형 지형 실험은 기존 `SimpleTerrain`에 얹지 않는다.

새 모듈:

```text
modules/open_world_terrain
```

초기 실험:

- height texture.
- GPU displacement.
- CPU height data 피킹.
- shader normal 계산.

## 테스트

- editor build.
- headless editor start.
- terrain node 생성.
- brush edit.
- undo/redo.
- C# wrapper 확인.
