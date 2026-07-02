# Variant Dispatch

## Scope

Dynamic method calls, operators, constructors, utility functions, and indexed/named set-get behavior.

## Entry Points

- `Variant::call()`
- `Variant::evaluate()`
- `Variant::set()` / `Variant::get()`
- `VariantUtilityFunctions`
- `VariantConstruct`

## Flow Notes

- Dispatch tables are generated or registered per Variant type.
- Operator behavior is shared by scripts, expressions, editor property operations, and serialized evaluations.
- Set/get paths cover indexed containers, object properties, named fields, and built-in value fields.

## Code Links

- `core/variant/variant_call.cpp`
- `core/variant/variant_op.*`
- `core/variant/variant_setget.*`
- `core/variant/variant_construct.*`
- `core/variant/variant_utility.*`
- `core/variant/method_ptrcall.h`