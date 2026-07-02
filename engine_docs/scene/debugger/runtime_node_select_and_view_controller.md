# Runtime Node Select And View Controller

## Scope

Runtime node picking/select helpers and 3D debug view movement/controller behavior.

## Entry Points

- `RuntimeNodeSelect`
- `View3DController`

## Flow Notes

- Runtime node selection helps editor/debugger tools identify scene nodes from runtime views.
- View3DController provides camera-like controls for scene debug views.
- These helpers cross scene, input, and debugger/editor-facing behavior.

## Code Links

- `scene/debugger/runtime_node_select.*`
- `scene/debugger/view_3d_controller.*`
- `scene/main/viewport.*`
- `core/input/input_event.*`