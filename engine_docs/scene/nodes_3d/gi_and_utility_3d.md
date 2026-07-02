# 3D GI And Utility

## Scope

Voxel GI, voxelization, lightmapping helpers, spring arms, spline meshes, velocity tracking, and other 3D utility nodes.

## Entry Points

- `VoxelGI`
- `Voxelizer`
- `Lightmapper`
- `SpringArm3D`
- `SplineMesh3D`
- `VelocityTracker3D`

## Flow Notes

- GI and lightmapping nodes bridge scene data with rendering/light bake systems.
- Spline mesh and spring arm nodes are specialized utility nodes layered on Node3D behavior.
- Velocity trackers derive motion data from transform history.

## Code Links

- `scene/3d/voxel_gi.*`
- `scene/3d/voxelizer.*`
- `scene/3d/lightmapper.*`
- `scene/3d/spring_arm_3d*`
- `scene/3d/spline_mesh_3d.*`
- `scene/3d/velocity_tracker_3d.*`