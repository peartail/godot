# Visual UI Widgets

## Scope

Panels, texture widgets, color picker, video player, separators, reference rects, foldable containers, virtual joystick, and view panning helpers.

## Entry Points

- `Panel`
- `PanelContainer`
- `TextureRect`
- `TextureButton`
- `TextureProgressBar`
- `ColorPicker`
- `VideoStreamPlayer`
- `ViewPanner`
- `VirtualJoystick`

## Flow Notes

- Texture widgets render resource-backed images through Control drawing.
- Color picker combines color models, shapes, controls, and popup interactions.
- Video stream player bridges UI nodes with video stream resources and audio/video playback.

## Code Links

- `scene/gui/panel*`
- `scene/gui/texture_*`
- `scene/gui/color_picker*`
- `scene/gui/video_stream_player.*`
- `scene/gui/view_panner.*`
- `scene/gui/virtual_joystick.*`