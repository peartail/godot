# Scene

Scene system, node hierarchy, runtime nodes, gameplay-facing resources, UI, animation, audio, theme, and scene debugging.

## Feature Categories

- [Main Scene System](main/README.md): `Node`, `SceneTree`, `Viewport`, `Window`, canvas items, multiplayer, timers, and scene ownership.
- [2D Nodes](nodes_2d/README.md): `Node2D`, sprites, cameras, 2D physics nodes, navigation, tile maps, particles, and 2D skeleton/path helpers.
- [3D Nodes](nodes_3d/README.md): `Node3D`, visual instances, meshes, lights, cameras, 3D physics nodes, navigation, skeletons, IK, particles, and GI helpers.
- [GUI](gui/README.md): `Control`, containers, buttons, text controls, popups, lists/trees, graph edit, dialogs, and UI media widgets.
- [Animation](animation/README.md): `AnimationPlayer`, `AnimationTree`, mixers, blend spaces, state machines, tweens, root motion, and easing.
- [Resources](resources/README.md): scene resources, packed scenes, animation resources, textures, materials, meshes, shapes, navigation, shaders, text, themes, and worlds.
- [Audio](audio/README.md): non-positional audio stream playback nodes and shared playback internals.
- [Theme](theme/README.md): default theme generation, `ThemeDB`, theme ownership, icons, and default font data.
- [Debugger](debugger/README.md): scene debugger, runtime node selection, debugger object views, and 3D debug camera/controller helpers.
- [Registration And Properties](registration_and_properties/README.md): scene type registration, scene string names, and property helper utilities.

## Global Entry Points

- `Node` is the base class for scene tree objects.
- `SceneTree` owns runtime node traversal, groups, timers, multiplayer polling, and root viewport coordination.
- `Viewport` bridges scene content to rendering/input and owns worlds/canvases.
- `PackedScene` serializes and instantiates node trees.

## Code Links

- `scene/main/`
- `scene/2d/`
- `scene/3d/`
- `scene/gui/`
- `scene/animation/`
- `scene/resources/`
- `scene/register_scene_types.*`