# Terrain Editor Engine Development Plan

## 현재 방향

기존 터레인 구현은 `SimpleTerrain`으로 이름을 정리한다.

대형 월드 지형 실험은 새 `OpenWorldTerrain` 모듈로 분리한다.

## Phase 1: SimpleTerrain Rename

목표:

- 기존 `Terrain3D`/`TerrainData` 구현을 `SimpleTerrain3D`/`SimpleTerrainData`로 변경한다.
- 모듈 경로를 `modules/simple_terrain`으로 변경한다.
- 기존 CPU chunk mesh 기능은 유지한다.

작업:

- `modules/terrain` -> `modules/simple_terrain`.
- `terrain_3d.*` -> `simple_terrain_3d.*`.
- `terrain_data.*` -> `simple_terrain_data.*`.
- `Terrain3D` -> `SimpleTerrain3D`.
- `TerrainData` -> `SimpleTerrainData`.
- `TerrainEditorPlugin` -> `SimpleTerrainEditorPlugin`.
- `Terrain3DGizmoPlugin` -> `SimpleTerrain3DGizmoPlugin`.
- doc XML 이름 변경.
- Mono glue/assembly 재생성.

검증:

- SCons editor build 통과.
- Mono assemblies build 통과.
- headless editor 실행 통과.
- stale `Terrain3D.cs`/`TerrainData.cs` wrapper 제거.

## Phase 2: SimpleTerrain Stabilization

목표:

- 기존 기능을 기준 구현으로 안정화한다.

작업:

- 브러시 성능 확인.
- Smooth 브러시 영역 snapshot 최적화 검토.
- doc XML 설명 정리.
- `module_simple_terrain_enabled=no` 빌드 옵션 확인.
- 기존 사용 예제 문서 갱신.

## Phase 3: OpenWorldTerrain Module Skeleton

목표:

- `SimpleTerrain`과 독립된 대형 지형 실험 모듈을 만든다.

예상 구조:

```text
modules/open_world_terrain/
  SCsub
  config.py
  register_types.h
  register_types.cpp
  open_world_terrain_3d.h
  open_world_terrain_3d.cpp
  open_world_terrain_data.h
  open_world_terrain_data.cpp
  doc_classes/
    OpenWorldTerrain3D.xml
    OpenWorldTerrainData.xml
```

초기 클래스:

- `OpenWorldTerrain3D`
- `OpenWorldTerrainData`

## Phase 4: GPU Height Texture Prototype

목표:

- height texture + GPU displacement가 Godot 모듈에서 안정적으로 동작하는지 검증한다.

작업:

- CPU height array 생성.
- height array를 `ImageTexture`로 변환.
- grid patch mesh 생성.
- shader vertex 단계에서 height texture sample.
- CPU 피킹은 기존 height data 방식으로 유지.
- 브러시 후 texture update 경로 검증.

## Phase 5: OpenWorld 확장 후보

실험 성공 후 검토:

- tile 기반 height data.
- clipmap ring grid.
- tile cache.
- 부분 texture upload.
- shader normal 계산.
- collision tile 별도 생성.
- virtual texture 또는 splatmap 연동.

## 빌드 명령

```powershell
scons platform=windows target=editor dev_build=yes tools=yes module_mono_enabled=yes -j4
```

ClassDB API 변경 후:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --generate-mono-glue modules\mono\glue
python modules\mono\build_scripts\build_assemblies.py --godot-output-dir bin --godot-platform windows --dev-debug
```
