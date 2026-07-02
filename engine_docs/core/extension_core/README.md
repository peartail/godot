# Extension Core

GDExtension loading, interface exposure, generated wrappers, compatibility hashes, API dumps, and external library lifecycle.

## Subtopics

- [GDExtension Core](gdextension_core.md): extension objects, lifecycle, initialization levels, and resource format glue.
- [Interface And API Dump](interface_and_api_dump.md): public C interface, JSON/interface dumps, and header generation.
- [Library Loading And Manager](library_loading_and_manager.md): extension library loaders, manager registry, and Godot instance bridge.
- [Generated Wrappers And Compatibility](generated_wrappers_and_compatibility.md): generated wrappers, compatibility hashes, and special compatibility data.

## Global Entry Points

- `GDExtension` represents a loaded extension resource.
- `GDExtensionManager` tracks loaded extensions and initialization state.
- `gdextension_interface.*` exposes the C ABI to extension code.

## Code Links

- `core/extension/gdextension.*`
- `core/extension/gdextension_manager.*`
- `core/extension/gdextension_interface.*`
- `core/extension/extension_api_dump.*`