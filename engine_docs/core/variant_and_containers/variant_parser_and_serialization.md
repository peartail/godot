# Variant Parser And Serialization

## Scope

Text parsing of Variant data, resource-aware parser hooks, deep duplicate helpers, and pool allocation helpers.

## Entry Points

- `VariantParser`
- `VariantWriter`
- `VariantDeepDuplicate`
- `VariantPools`

## Flow Notes

- `VariantParser` is used by text resources, config-like formats, and test fixtures.
- Resource parser callbacks allow external code to resolve resource references while reading Variant data.
- Deep duplicate behavior is important for Arrays, Dictionaries, Resources, and object-bearing data.

## Code Links

- `core/variant/variant_parser.h`
- `core/variant/variant_parser.cpp`
- `core/variant/variant_deep_duplicate.h`
- `core/variant/variant_pools.*`
- `core/io/marshalls.*`