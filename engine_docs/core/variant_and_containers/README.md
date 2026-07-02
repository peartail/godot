# Variant And Containers

Universal value representation, dynamic calls, typed containers, operators, and parser glue.

## Subtopics

- [Variant Core](variant_core.md): storage, type tags, construction, destruction, and conversion.
- [Array And Dictionary](array_and_dictionary.md): dynamic containers, copy-on-write data, read-only state, and recursive comparison.
- [Callable](callable.md): callables, bound arguments, custom callables, and method pointer call glue.
- [Typed Containers](typed_containers.md): typed arrays, typed dictionaries, validation, and container type metadata.
- [Variant Dispatch](variant_dispatch.md): calls, operators, set/get paths, constructors, and utility functions.
- [Variant Parser And Serialization](variant_parser_and_serialization.md): text parsing, deep duplicate, and marshaling-facing behavior.

## Global Entry Points

- `Variant` is the dynamic value type used by properties, script APIs, serialization, and reflected calls.
- `Array` and `Dictionary` are Variant-backed containers with shared storage.
- `Callable` stores function targets for signals, deferred calls, and user callbacks.

## Code Links

- `core/variant/variant.*`
- `core/variant/array.*`
- `core/variant/dictionary.*`
- `core/variant/callable.*`
- `core/variant/variant_call.cpp`
- `core/variant/variant_op.*`
- `core/variant/variant_setget.*`
- `core/variant/variant_parser.*`