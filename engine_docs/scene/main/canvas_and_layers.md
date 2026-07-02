# Canvas And Layers

## Scope

2D draw command submission, canvas item visibility, material/shader state, canvas layers, and shader global overrides.

## Entry Points

- `CanvasItem`
- `CanvasLayer`
- `ShaderGlobalsOverride`
- `CanvasItem::_draw()`

## Flow Notes

- `CanvasItem` is the base for 2D and GUI drawing.
- Draw commands are cached and submitted through rendering server canvas APIs.
- Canvas layers alter draw order and transform relative to the viewport.

## Code Links

- `scene/main/canvas_item.*`
- `scene/main/canvas_layer.*`
- `scene/main/shader_globals_override.*`
- `servers/rendering_server.*`