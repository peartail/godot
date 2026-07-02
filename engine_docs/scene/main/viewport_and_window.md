# Viewport And Window

## Scope

Viewport rendering/input target behavior, window lifecycle, root viewport, subviewports, world/canvas ownership, and embedding.

## Entry Points

- `Viewport`
- `Window`
- `SubViewport`
- `Viewport::push_input()`
- `Window::popup()`

## Flow Notes

- Viewports own render targets, input dispatch state, worlds, cameras, and canvas transforms.
- Windows connect SceneTree state to DisplayServer windows.
- Subviewports are used by UI containers, editor previews, tools, and offscreen rendering.

## Code Links

- `scene/main/viewport.h`
- `scene/main/viewport.cpp`
- `scene/main/window.h`
- `scene/main/window.cpp`
- `scene/gui/subviewport_container.*`
- `servers/display_server.*`