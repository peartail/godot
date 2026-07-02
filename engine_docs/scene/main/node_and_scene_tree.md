# Node And SceneTree

## Scope

Node hierarchy, ownership, lifecycle, groups, process modes, notifications, path lookup, and SceneTree orchestration.

## Entry Points

- `Node`
- `SceneTree`
- `Node::add_child()`
- `Node::_notification()`
- `SceneTree::process()`
- `SceneTree::physics_process()`

## Flow Notes

- `Node` lifecycle depends on enter/exit tree notifications and ownership rules.
- `SceneTree` coordinates processing, groups, timers, multiplayer polling, and root viewport state.
- Scene paths and edited-scene ownership affect editor and runtime behavior differently.

## Code Links

- `scene/main/node.h`
- `scene/main/node.cpp`
- `scene/main/scene_tree.h`
- `scene/main/scene_tree.cpp`
- `core/string/node_path.*`