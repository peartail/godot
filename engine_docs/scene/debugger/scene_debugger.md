# Scene Debugger

## Scope

Scene debugger message handling, object inspection snapshots, runtime node state reporting, and editor/debugger integration points.

## Entry Points

- `SceneDebugger`
- `SceneDebuggerObject`
- `SceneDebugger::initialize()`

## Flow Notes

- Scene debugger code bridges runtime SceneTree state to editor/debugger tools.
- Object snapshots expose selected properties and state through debugger messages.
- Debugger behavior depends on core debugger routing and remote debugger peers.

## Code Links

- `scene/debugger/scene_debugger.*`
- `scene/debugger/scene_debugger_object.*`
- `core/debugger/engine_debugger.*`