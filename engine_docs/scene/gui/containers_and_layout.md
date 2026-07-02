# Containers And Layout

## Scope

UI layout containers, size negotiation, child sorting, split/tab layouts, margins, aspect ratio, and subviewport embedding.

## Entry Points

- `Container`
- `BoxContainer`
- `GridContainer`
- `FlowContainer`
- `SplitContainer`
- `TabContainer`
- `SubViewportContainer`

## Flow Notes

- Containers drive child size/position based on minimum sizes and size flags.
- Split and tab containers manage interactive state in addition to layout.
- SubViewport containers bridge UI layout and viewport rendering.

## Code Links

- `scene/gui/container.*`
- `scene/gui/box_container.*`
- `scene/gui/grid_container.*`
- `scene/gui/flow_container.*`
- `scene/gui/split_container.*`
- `scene/gui/tab_container.*`
- `scene/gui/subviewport_container.*`