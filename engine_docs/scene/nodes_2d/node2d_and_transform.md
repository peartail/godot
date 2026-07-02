# Node2D And Transform

## Scope

2D transform base class, transform propagation, editor markers, remote transform sync, and visibility notifications.

## Entry Points

- `Node2D`
- `Marker2D`
- `RemoteTransform2D`
- `VisibleOnScreenNotifier2D`

## Flow Notes

- `Node2D` builds on `CanvasItem` and provides position, rotation, scale, skew, and global transform behavior.
- Remote transforms copy transform state to target nodes.
- Visibility notifiers depend on viewport/render visibility state.

## Code Links

- `scene/2d/node_2d.*`
- `scene/2d/marker_2d.*`
- `scene/2d/remote_transform_2d.*`
- `scene/2d/visible_on_screen_notifier_2d.*`