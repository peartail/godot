# Core Registration

## Scope

Core class registration, singleton setup, global initialization, and unregister cleanup.

## Entry Points

- `register_core_types()`
- `unregister_core_types()`
- `register_core_settings()`

## Flow Notes

- Core registration happens before higher-level engine systems register their types.
- Registration order matters because later systems depend on Object, Variant, IO, and config classes.
- Unregister must release singleton and static state in a compatible reverse order.

## Code Links

- `core/register_core_types.h`
- `core/register_core_types.cpp`
- `main/main.cpp`