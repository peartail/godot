# 2D Camera And Parallax

## Scope

2D camera control, parallax backgrounds/layers, canvas modulation, canvas grouping, and back-buffer copy nodes.

## Entry Points

- `Camera2D`
- `Parallax2D`
- `ParallaxBackground`
- `ParallaxLayer`
- `CanvasModulate`
- `CanvasGroup`
- `BackBufferCopy`

## Flow Notes

- `Camera2D` affects the viewport canvas transform and smoothing/limits behavior.
- Parallax nodes offset child content relative to viewport/camera movement.
- Back-buffer copy and canvas groups depend on rendering server canvas behavior.

## Code Links

- `scene/2d/camera_2d.*`
- `scene/2d/parallax_2d.*`
- `scene/2d/parallax_background.*`
- `scene/2d/parallax_layer.*`
- `scene/2d/canvas_modulate.*`
- `scene/2d/canvas_group.*`
- `scene/2d/back_buffer_copy.*`