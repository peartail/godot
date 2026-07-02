# Library Loading And Manager

## Scope

Extension library loading, manager registry, function lookup, Godot instance bridge, and libgodot interface support.

## Entry Points

- `GDExtensionManager`
- `GDExtensionLibraryLoader`
- `GDExtensionFunctionLoader`
- `GodotInstance`

## Flow Notes

- Library loaders abstract platform-specific dynamic library loading.
- The manager owns loaded extension tracking and initialization/shutdown transitions.
- Function loader helpers connect named entry points to extension initialization code.

## Code Links

- `core/extension/gdextension_manager.*`
- `core/extension/gdextension_library_loader.*`
- `core/extension/gdextension_function_loader.*`
- `core/extension/godot_instance.*`
- `core/extension/libgodot.h`
- `core/os/shared_object.h`