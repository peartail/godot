---
name: godot-public-api
description: >-
  Publishes and verifies Godot ClassDB public APIs for this fork — _bind_methods,
  module doc_classes XML, config.py get_doc_classes, MonoGlue/C# bindings, and
  headless agent-docs checks. Use when adding or renaming ClassDB classes,
  methods, properties, signals, or enums; when updating XML class docs; when the
  user mentions MonoGlue, agent-docs, public API, or godot-public-api; or after
  any open_world_terrain / simple_terrain binding change.
---

# Godot Public API (this fork)

Complete this checklist whenever a **public** ClassDB surface changes. Do not ship C++-only APIs that agents/scripts cannot call.

## When required

Trigger on any of:

- New `GDCLASS` / `GDREGISTER_*`
- New or renamed methods, properties, signals, enums in `_bind_methods`
- `doc_classes/*.xml` add/rename/edit
- Module `config.py` `get_doc_classes()` changes
- User asks for MonoGlue, agent-docs, or public API sync

Terrain height sculpt `apply_brush` and similar **non-placement** APIs are unrelated unless those bindings also change.

## Checklist (in order)

Copy and track:

```
Public API:
- [ ] 1. ClassDB bindings (_bind_methods / register_types)
- [ ] 2. doc_classes XML (+ rename files if class renamed)
- [ ] 3. module config.py get_doc_classes()
- [ ] 4. Compatibility aliases if renaming saved types (optional)
- [ ] 5. Build + MonoGlue + agent-docs verify via build.ps1
- [ ] 6. module_docs / ADR only if domain contract changed
```

### 1. ClassDB

- Implement `GDCLASS`, `_bind_methods`, `ADD_PROPERTY`, `BIND_ENUM_CONSTANT` as needed.
- Register in `modules/<module>/register_types.cpp`.
- Prefer scriptable/headless verbs (`preview_*`, `apply_*`, `validate_*`, `rebuild_*`, `clear_*`, `get_*_report`).

### 2. XML docs

- Path: `modules/<module>/doc_classes/<ClassName>.xml`
- Keep `class name`, method names, and `PROPERTY_HINT_*` / enum type strings in sync with ClassDB.
- On rename: `git mv` the XML file; update cross-type references in sibling XML.

### 3. config.py

- Add/rename the class string in `get_doc_classes()` for that module so editor Help / agent-docs embed it.

### 4. Compatibility (renames)

- Resource extensions (`RES_BASE_EXTENSION`) — keep stable unless intentionally migrating files.
- For renamed classes: `ClassDB::add_compatibility_class("OldName", "NewName")`.
- Method renames are breaking unless you keep thin wrappers.

### 5. Build + verify (required)

**Do not hand-write long scons commands.** Use `scripts/build.ps1`.

After ClassDB / XML / config changes:

```powershell
# OpenWorldTerrain / World Placement work
.\scripts\build.ps1 -Preset terrain -MonoGlue -PublicApi -PublicApiClasses OpenWorldPlacement3D,OpenWorldPlacementEntry,OpenWorldPlacementPreset,OpenWorldPlacementData

# Default editor module work
.\scripts\build.ps1 -Preset editor -MonoGlue -PublicApi -PublicApiClasses YourClassName
```

Notes:

- `-PublicApi` on `editor` / `terrain` implies `-MonoGlue` if not already set.
- `-PublicApiClasses` is required with `-PublicApi` (comma-separated class names).
- Verify-only after an existing binary:  
  `.\scripts\build.ps1 -Preset public-api -PublicApiClasses YourClassName`  
  or `.\scripts\build.ps1 -Preset mono-glue -PublicApi -PublicApiClasses YourClassName`

Manual agent-docs (optional extra checks):

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-class YourClassName
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-search YourTerm
```

See `engine_docs/agent_tools/agent_docs_cli.md`.

### 6. Domain docs

Update `module_docs/` only when authoring semantics or architecture change — not for pure renames already covered by XML.

## Done criteria

- Build exits 0
- MonoGlue/assemblies succeed when bindings changed
- Each `-PublicApiClasses` entry appears in `--agent-docs-class` output
- No stale generated C# types for removed class names (delete leftover `Generated/GodotObjects/OldName.cs` if glue left them)

## Related

- Build mechanics: `.cursor/skills/godot-build/SKILL.md`
- Agent-first rules: `AGENTS.md`
