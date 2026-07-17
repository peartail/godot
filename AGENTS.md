# Codex Workspace Instructions

## Agent-First Development

- Assume this project is developed and operated primarily through AI agents. The user should rarely need mouse dragging, repeated clicking, Inspector-only editing, or modal file dialogs.
- Every new authoring feature must expose a text- or code-driven path through ClassDB APIs, GDScript/C#, a headless command, or a deterministic generation script. A Gizmo or Inspector button may be added as an optional convenience, but must not be the only workflow.
- Prefer human-readable source inputs such as `.tres`, `.tscn`, JSON, and documented numeric parameters. Treat binary meshes and baked resources as derived outputs that agents can regenerate.
- Generation must be deterministic when the same source data and seed are supplied. Separate `generate`, `validate`, `bake`, and `report` operations so agents can diagnose failures without GUI interaction.
- Use semantic references such as `NodePath`, stable IDs, profile names, support-object names, and numeric anchors instead of requiring viewport picking.
- Provide validation errors and generation statistics as structured return values or machine-readable text. Do not rely only on visual notifications or editor popups.
- Add XML class documentation, GDScript/C# examples, headless test coverage, and text-based sample configurations for public systems.
- Ensure an agent can create a sample scene, run generation, save outputs, reload them, and verify essential properties entirely from the terminal.
- Visual inspection remains useful, but it should verify computed results rather than be required to author them.

## Build Verification

Use `scripts/build.ps1` instead of hand-written `scons` commands (see `.cursor/skills/godot-build/SKILL.md`).

```powershell
.\scripts\build.ps1
.\scripts\build.ps1 -Preset terrain
.\scripts\build.ps1 -MonoGlue
.\scripts\build.ps1 -Preset mono-glue
```

- Default: Mono editor dev build (`-Jobs 1` on Windows/MSVC to avoid PDB conflicts).
- OpenWorldTerrain work: `-Preset terrain`.
- ClassDB / binding changes: `-MonoGlue` after build, or `-Preset mono-glue` alone.

## Agent-Readable Engine Docs

- Custom editor builds expose embedded class reference docs through CLI commands for AI agents:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-search OpenWorldTerrain
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-class OpenWorldTerrain3D
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-dump .\.agent_docs
```

- In a separate game project that uses this custom engine, add the same commands to that project's `AGENTS.md` so agents know they can query engine docs from the terminal.
- See `engine_docs/agent_tools/agent_docs_cli.md` for the full command reference.
