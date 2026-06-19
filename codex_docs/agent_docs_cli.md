# Agent Docs CLI

Custom editor builds expose the embedded Godot class reference through command-line tools intended for AI agents.

These commands read the same class documentation that the editor Help system uses, including module docs from `modules/*/doc_classes`.

## Commands

List all embedded class docs:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-list
```

Search class docs:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-search OpenWorldTerrain
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-search apply_layer_brush
```

Print one class as Markdown:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-class OpenWorldTerrain3D
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-class SimpleTerrain3D
```

Dump all embedded class docs to a Markdown file:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-dump .\.agent_docs
```

The dump creates:

```text
.agent_docs/godot_agent_docs.md
```

## Separate Game Projects

For a game project that uses this custom engine, add this hint to the project's `AGENTS.md`:

````md
## Custom Godot Engine Docs

This project uses a custom Godot editor build with agent-readable engine docs.

Before editing terrain code or scenes, query the engine docs from the terminal:

```powershell
<path-to-custom-godot-editor> --headless --agent-docs-search OpenWorldTerrain
<path-to-custom-godot-editor> --headless --agent-docs-class OpenWorldTerrain3D
<path-to-custom-godot-editor> --headless --agent-docs-class SimpleTerrain3D
```

To create a local searchable copy:

```powershell
<path-to-custom-godot-editor> --headless --agent-docs-dump .\.agent_docs
```
````

Use `--agent-docs-class` for exact API details and `--agent-docs-search` when you do not know the class name.
