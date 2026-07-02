# 3D Camera Light Environment

## Scope

3D camera projection, lights, reflection/lightmap probes, fog, world environment, and environment-affecting nodes.

## Entry Points

- `Camera3D`
- `Light3D`
- `ReflectionProbe`
- `LightmapGI`
- `LightmapProbe`
- `FogVolume`
- `WorldEnvironment`

## Flow Notes

- Cameras drive viewport 3D rendering and projection state.
- Lights and probes register rendering-server instances and reference resource data.
- Environment nodes modify viewport/world rendering environment.

## Code Links

- `scene/3d/camera_3d.*`
- `scene/3d/light_3d.*`
- `scene/3d/reflection_probe.*`
- `scene/3d/lightmap_gi.*`
- `scene/3d/lightmap_probe.*`
- `scene/3d/fog_volume.*`
- `scene/3d/world_environment.*`