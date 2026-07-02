# Variant Core

## Scope

Variant storage, type tagging, lifetime management, conversion, comparison, and native pointer helpers.

## Entry Points

- `Variant`
- `Variant::Type`
- `Variant::construct()`
- `Variant::clear()`
- `VariantInternal`
- `VariantCaster`

## Flow Notes

- `Variant` owns or references data depending on the stored type.
- Construction and destruction paths are centralized so reflected APIs can create values generically.
- Type conversion behavior affects script calls, property assignment, serialization, and editor inspectors.

## Code Links

- `core/variant/variant.h`
- `core/variant/variant.cpp`
- `core/variant/variant_internal.h`
- `core/variant/variant_construct.*`
- `core/variant/variant_destruct.*`
- `core/variant/variant_caster.h`
- `core/variant/native_ptr.h`