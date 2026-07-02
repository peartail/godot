# Control Core

## Scope

Base UI node behavior, focus, mouse/filter input, anchors, offsets, minimum size, theme lookup, and canvas drawing integration.

## Entry Points

- `Control`
- `Control::_gui_input()`
- `Control::set_anchors_preset()`
- `Control::get_theme_*()`

## Flow Notes

- `Control` inherits from `CanvasItem` and is the root of GUI behavior.
- Layout depends on anchors, offsets, size flags, minimum size, and parent containers.
- Theme lookup falls back through node/theme/default theme chains.

## Code Links

- `scene/gui/control.h`
- `scene/gui/control.cpp`
- `scene/main/canvas_item.*`
- `scene/theme/theme_owner.*`