# Node3D And Transform

## Scope

3D transform hierarchy, global/local transform state, markers, remote transforms, paths, and visibility notification helpers.

## Entry Points

- `Node3D`
- `Marker3D`
- `RemoteTransform3D`
- `Path3D`
- `VisibleOnScreenNotifier3D`

## Flow Notes

- `Node3D` owns transform propagation and notification behavior for 3D scene nodes.
- Remote transform nodes copy transform state to target nodes.
- Path nodes wrap `Curve3D` resources for spatial path workflows.

## Code Links

- `scene/3d/node_3d.*`
- `scene/3d/marker_3d.*`
- `scene/3d/remote_transform_3d.*`
- `scene/3d/path_3d.*`
- `scene/3d/visible_on_screen_notifier_3d.*`