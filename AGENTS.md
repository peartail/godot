# Codex Workspace Instructions

## Build Verification

- When verifying engine/editor changes in this workspace, use a Mono editor build by default.
- For OpenWorldTerrain work, prefer:

```powershell
scons platform=windows target=editor dev_build=yes module_open_world_terrain_enabled=yes module_mono_enabled=yes -j1
```

- Use `-j1` on Windows/MSVC unless there is a specific reason to parallelize, because parallel builds can fail with PDB write conflicts.
- If exposed ClassDB methods/properties changed, also regenerate Mono glue and rebuild assemblies:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --generate-mono-glue modules\mono\glue
python modules\mono\build_scripts\build_assemblies.py --godot-output-dir bin --godot-platform windows --dev-debug
```
