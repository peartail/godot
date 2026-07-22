---
name: godot-build
description: Builds this Godot Engine fork on Windows using scripts/build.ps1 presets instead of hand-written scons commands. Use when the user asks to build, verify, rebuild, compile, run mono glue, or check editor changes.
---

# Godot Build (this fork)

**Do not construct long `scons` commands.** Run `scripts/build.ps1` from the repo root.

## Presets

| Task | Command |
|------|---------|
| Default editor build | `.\scripts\build.ps1` |
| OpenWorldTerrain work | `.\scripts\build.ps1 -Preset terrain` |
| ClassDB / binding changes | `.\scripts\build.ps1 -MonoGlue` or `.\scripts\build.ps1 -Preset mono-glue` |
| C# assemblies only | `.\scripts\build.ps1 -Preset assemblies` |
| Public API + MonoGlue + agent-docs | `.\scripts\build.ps1 -Preset terrain -PublicApi -PublicApiClasses ClassA,ClassB` |
| agent-docs verify only | `.\scripts\build.ps1 -Preset public-api -PublicApiClasses ClassA,ClassB` |

## Rules

- Windows/MSVC: keep default `-Jobs 1` unless the user explicitly asks to parallelize.
- After `modules/simple_terrain` or `modules/open_world_terrain` C++ API changes, prefer `-Preset terrain -MonoGlue`.
- ClassDB / XML / rename work: follow `.cursor/skills/godot-public-api/SKILL.md` and use `-PublicApi -PublicApiClasses ...` (implies MonoGlue on `editor`/`terrain`).
- Extra SCons flags: `.\scripts\build.ps1 -ExtraArgs "module_simple_terrain_enabled=no"`
- Run builds in the background when they are expected to take a long time.

## Agent docs (after build)

Prefer build-script verification when classes are known:

```powershell
.\scripts\build.ps1 -Preset mono-glue -PublicApi -PublicApiClasses OpenWorldPlacement3D
```

Or query manually:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-search <term>
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-class <ClassName>
```

See `engine_docs/agent_tools/agent_docs_cli.md` for full reference.
