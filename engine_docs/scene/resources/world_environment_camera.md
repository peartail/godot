# World Environment Camera

## Scope

World resources, environment data, camera attributes, compositor resources, fog, sky, and rendering environment configuration.

## Entry Points

- `World2D`
- `World3D`
- `Environment`
- `CameraAttributes`
- `Compositor`
- `Sky`

## Flow Notes

- Viewports own or reference world resources for 2D and 3D rendering/physics contexts.
- Environment and camera attributes configure rendering behavior for a world or viewport.
- Sky and fog resources feed rendering server environment state.

## Code Links

- `scene/resources/world_2d.*`
- `scene/resources/world_3d*`
- `scene/resources/environment.*`
- `scene/resources/camera_attributes.*`
- `scene/resources/compositor.*`
- `scene/resources/sky.*`