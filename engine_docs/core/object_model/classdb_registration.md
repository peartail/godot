# ClassDB Registration

## Scope

Native class metadata registration for construction, inheritance, binding, constants, properties, signals, enums, and documentation.

## Entry Points

- `ClassDB`
- `GDCLASS`
- `ClassDB::register_class()`
- `ClassDB::bind_method()`
- `ClassDB::add_property()`
- `ClassDB::add_signal()`
- `ClassDB::bind_integer_constant()`

## Flow Notes

- Native types register metadata during engine/module registration.
- `ClassDB` records inheritance and API surface used by scripts, editor inspection, docs, and serialization.
- `_bind_methods()` is the usual local class hook for method/property/signal metadata.
- Class construction through `ClassDB` is only available for registered constructible classes.

## Code Links

- `core/object/class_db.h`
- `core/object/class_db.cpp`
- `core/object/object.h`
- `core/register_core_types.*`