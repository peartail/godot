# Typed Containers

## Scope

Typed Array and Dictionary metadata, validation helpers, script API type checks, and container type propagation.

## Entry Points

- `TypedArray<T>`
- `TypedDictionary<K, V>`
- `ContainerType`
- `container_type_validate.h`
- `TypeInfo`

## Flow Notes

- Typed containers store expected Variant type, class name, and script constraints.
- Validation affects property assignment, method arguments, and script-exposed API contracts.
- Typed metadata must be preserved when containers are duplicated or passed through Variant calls.

## Code Links

- `core/variant/typed_array.h`
- `core/variant/typed_dictionary.h`
- `core/variant/container_type_validate.h`
- `core/variant/type_info.h`
- `core/variant/binder_common.h`