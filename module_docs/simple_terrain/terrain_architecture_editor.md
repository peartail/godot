# SimpleTerrain Editor Architecture

## 에디터 분리

런타임 코드:

- `simple_terrain_data.*`
- `simple_terrain_3d.*`

에디터 전용 코드:

- `editor/simple_terrain_editor_plugin.*`

`TOOLS_ENABLED` 빌드에서만 에디터 플러그인을 등록한다.

## Toolbar

`SimpleTerrainEditorPlugin`은 3D editor menu에 toolbar를 추가한다.

Toolbar는 `SimpleTerrain3D`가 선택되었을 때만 보인다.

제공 기능:

- Terrain mode toggle.
- Brush operation selector.
- Radius slider.
- Strength slider.
- Flat button.
- Random button.

## Viewport Input

Terrain mode가 켜져 있을 때:

1. left mouse press에서 camera ray를 만든다.
2. `SimpleTerrain3D.get_brush_hit()`로 terrain hit를 찾는다.
3. hit가 있으면 painting 상태로 들어간다.
4. mouse motion 동안 brush를 적용한다.
5. mouse release에서 undo action을 기록한다.

## Undo/Redo

브러시 stroke는 전체 height array를 저장하지 않는다.

대신 다음 배열을 수집한다.

- changed indices.
- before values.
- after values.

release 시 `commit_action(false)`로 이미 적용된 brush 결과를 다시 실행하지 않고 undo stack에 기록만 한다.

## Chunk Gizmo

`SimpleTerrain3DGizmoPlugin`은 `show_chunk_gizmos`가 켜졌을 때 chunk boundary를 그린다.

debug line은 `SimpleTerrain3D.get_chunk_debug_lines()`에서 가져온다.

## 책임 경계

에디터 플러그인:

- selection.
- toolbar state.
- viewport input.
- undo/redo.
- gizmo drawing.

`SimpleTerrain3D`:

- height mutation.
- picking.
- mesh rebuild.
- debug line generation.
