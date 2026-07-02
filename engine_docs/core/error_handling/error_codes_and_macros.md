# Error Codes And Macros

## Scope

Shared error enum values, error string helpers, failure macros, warning macros, and diagnostic reporting behavior.

## Entry Points

- `Error`
- `ERR_FAIL_*`
- `ERR_PRINT*`
- `WARN_PRINT*`
- `CRASH_*`

## Flow Notes

- `Error` values are part of many public APIs, so changes can have wide binding and documentation impact.
- Macros usually combine condition checks with logging and return behavior.
- Diagnostics must avoid evaluating unsafe expressions more than intended.

## Code Links

- `core/error/error_list.h`
- `core/error/error_list.cpp`
- `core/error/error_macros.h`
- `core/error/error_macros.cpp`