# Engine Docs

Godot engine documentation for Codex agents and engine contributors.

## Organization Rules

- Keep each Markdown file under 180 lines.
- Split a topic when it grows past the line limit.
- Structure docs as `global -> category -> subtopic`.
- Link docs to relevant engine code paths whenever possible.
- Keep module-specific docs in `module_docs/`.

## Categories

- [Agent Tools](agent_tools/README.md): agent-facing CLI helpers and embedded docs access.
- [Core](core/README.md): low-level Object, Variant, resources, math, IO, and OS abstractions.
- [Scene](scene/README.md): Node, SceneTree, 2D/3D nodes, animation, physics nodes, and UI controls.
- [Servers](servers/README.md): rendering, physics, audio, text, navigation, XR, and server APIs.
- [Editor](editor/README.md): editor UI, inspectors, docks, import flow, scene editing, and tools.
- [Drivers](drivers/README.md): graphics, audio, input, file access, and platform service backends.
- [Platform](platform/README.md): platform ports and OS-specific entry points.
- [Main](main/README.md): startup, project settings, command-line flow, and engine loop coordination.
- [Documentation](documentation/README.md): class reference, docs generation, and documentation tooling.
- [Tests](tests/README.md): engine tests, test organization, and verification notes.

## Related Docs

- [Module Docs](../module_docs/README.md)
