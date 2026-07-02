# Scene Instancing

## Scope

Scene placeholders, missing nodes, resource preloading, packed scene interaction, and scene instancing support nodes.

## Entry Points

- `InstancePlaceholder`
- `MissingNode`
- `ResourcePreloader`
- `PackedScene`

## Flow Notes

- Placeholders defer full instancing for editor/runtime workflows.
- Missing nodes preserve data when a class is unavailable.
- Resource preloader stores named resource references inside scenes.

## Code Links

- `scene/main/instance_placeholder.*`
- `scene/main/missing_node.*`
- `scene/main/resource_preloader.*`
- `scene/resources/packed_scene.*`