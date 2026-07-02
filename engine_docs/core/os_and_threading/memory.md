# Memory

## Scope

Memory allocation helpers, allocation macros, pool allocation, and memory-related diagnostics.

## Entry Points

- `Memory`
- `memnew`
- `memdelete`
- `memalloc`
- `memfree`

## Flow Notes

- Engine allocation macros centralize construction, destruction, and tracking behavior.
- Object allocation and ref-counted lifetime sit above these lower-level memory helpers.
- Memory code is foundational and should avoid dependencies on higher engine layers.

## Code Links

- `core/os/memory.h`
- `core/os/memory.cpp`
- `core/typedefs.h`