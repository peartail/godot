# StringName

## Scope

Interned immutable names for fast comparison and stable identifier reuse.

## Entry Points

- `StringName`
- `StaticCString`
- `CoreStringNames`

## Flow Notes

- `StringName` interns text so comparisons are cheap and repeated identifiers share storage.
- Class names, method names, property names, signal names, and many server identifiers use `StringName`.
- Static string names are collected to avoid repeated runtime construction of common identifiers.

## Code Links

- `core/string/string_name.h`
- `core/string/string_name.cpp`
- `core/core_string_names.h`