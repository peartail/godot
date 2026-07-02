# Object And ObjectDB

## Scope

Core object identity, property access, metadata, notifications, signal surface, script attachment points, and live instance lookup.

## Entry Points

- `Object`
- `ObjectID`
- `ObjectDB`
- `Object::set()` / `Object::get()`
- `Object::notification()`
- `Object::connect()` / `Object::emit_signal()`
- `Object::set_script()` / `Object::get_script_instance()`

## Flow Notes

- `Object` exposes dynamic `_set`, `_get`, `_get_property_list`, and notification hooks.
- `ObjectID` is the stable handle used when direct pointers are unsafe to store.
- `ObjectDB` owns live object registration and resolves `ObjectID` back to objects.
- Script instances extend `Object` behavior without changing native inheritance.

## Code Links

- `core/object/object.h`
- `core/object/object.cpp`
- `core/object/object_id.h`
- `core/object/object.compat.inc`