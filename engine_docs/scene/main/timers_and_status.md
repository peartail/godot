# Timers And Status

## Scope

Scene timers, status indicator nodes, and scene-tree frame-time interpolation helpers.

## Entry Points

- `Timer`
- `StatusIndicator`
- `SceneTreeFTI`

## Flow Notes

- `Timer` emits timeout signals through SceneTree processing.
- Status indicator nodes expose platform/status UI hooks where available.
- Frame-time interpolation helpers smooth scene updates between physics ticks.

## Code Links

- `scene/main/timer.*`
- `scene/main/status_indicator.*`
- `scene/main/scene_tree_fti.*`
- `scene/main/scene_tree_fti_tests.*`