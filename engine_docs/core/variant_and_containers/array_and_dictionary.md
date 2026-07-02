# Array And Dictionary

## Scope

Variant-backed collection types, shared storage, typed metadata, read-only behavior, and recursive equality.

## Entry Points

- `Array`
- `Dictionary`
- `Array::set_typed()`
- `Dictionary::set_typed()`
- `Array::make_read_only()`
- `Dictionary::make_read_only()`

## Flow Notes

- Both containers use shared internal storage and detach when modified through copy-on-write paths.
- Typed metadata is checked by container validation and reflected API calls.
- Recursive comparison uses depth guards to avoid infinite loops on self-referential structures.

## Code Links

- `core/variant/array.h`
- `core/variant/array.cpp`
- `core/variant/dictionary.h`
- `core/variant/dictionary.cpp`
- `core/variant/container_type_validate.h`