# Scene Tree Lock + Copy Scene Path 기능 구현 요약

## 개요

이 문서는 Godot 에디터의 씬 트리(SceneTree)에서 구현한 두 가지 기능을 정리합니다:

1. 씬 트리 노드 항목에서 `Lock` 버튼을 표시하고, 잠긴 노드는 씬 뷰 클릭으로 선택/편집되지 않도록 처리
2. 씬 도킹 패널의 우상단 `[:]` 메뉴에 `Copy Scene Path` 항목 추가

## 수정된 파일

- `editor/docks/scene_tree_dock.h`
- `editor/docks/scene_tree_dock.cpp`
- `editor/scene/scene_tree_editor.cpp`

## 구현 상세

### 1. `Copy Scene Path` 메뉴 항목 추가

#### `editor/docks/scene_tree_dock.h`

- `SceneTreeDock::Tool` 열거형에 `TOOL_COPY_SCENE_PATH`를 추가했습니다.

#### `editor/docks/scene_tree_dock.cpp`

- `_update_tree_menu()`에서 씬 트리 도킹 메뉴에 `Copy Scene Path` 항목을 추가했습니다.
- 메뉴 항목 위에 구분선(separator)을 추가하여 기존 항목과 시각적으로 구분했습니다.
- 현재 편집 중인 씬이 없으면 해당 항목을 비활성화하도록 설정했습니다.
- `_tool_selected()`에서 `TOOL_COPY_SCENE_PATH` 처리 로직을 추가하여 현재 편집 중인 씬의 경로를 복사해 클립보드에 넣습니다.
- 클립보드 저장은 `DisplayServer::get_singleton()->clipboard_set(scene_path)`로 수행합니다.

### 2. 씬 트리 노드 잠금 버튼 구현

#### `editor/scene/scene_tree_editor.cpp`

- 씬 트리 항목 상단에 `Lock` 버튼을 표시하도록 수정했습니다. 2D/3D 항목에 대해 항상 보이도록 구성되어 있습니다.
- `BUTTON_LOCK` 클릭 시:
  - `Node`의 메타데이터 `_edit_lock_`을 설정/해제하여 잠금 상태를 토글합니다.
  - 잠금 해제 시 `remove_meta("_edit_lock_")`, 잠금 시 `set_meta("_edit_lock_", true)`를 수행합니다.
  - `EditorUndoRedoManager`를 사용해 잠금/잠금 해제 작업을 되돌릴 수 있도록 처리했습니다.
  - `node_changed` 신호와 `CanvasItemEditor::get_singleton()->emit_signal("item_lock_status_changed")`를 함께 발생시켜 에디터 상태를 갱신합니다.

## 사용자 동작 흐름

- 씬 도킹 패널 우상단 `[:]` 메뉴를 열면 `Copy Scene Path` 항목이 표시됩니다.
- 편집 중인 씬이 없을 경우 해당 메뉴 항목은 비활성화됩니다.
- 씬 트리 노드 항목에서 `Lock` 버튼을 클릭하면 해당 노드가 잠기고, 에디터 상호작용 시 잠긴 노드는 무시됩니다.

## 빌드

다음 명령으로 에디터를 빌드했습니다:

```powershell
python -m SCons platform=windows target=editor dev_build=yes module_mono_enabled=yes -j1
```

빌드 결과물:

- `bin\godot.windows.editor.dev.x86_64.mono.exe`

## 참고

- `Copy Scene Path` 메뉴가 사용자에게 보이지 않는 문제를 발견하여 메뉴 항목을 항상 추가하고 구분선을 넣어 시각적으로 확실히 표시하도록 수정했습니다.
- 잠금 기능은 `CanvasItemEditor`와 연동되어 잠금 상태 변경을 알리는 신호를 발생시키도록 구현되었습니다.
