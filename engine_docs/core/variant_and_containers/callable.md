# Callable

## Scope

Callable targets, bound arguments, custom callables, method pointers, and Variant-facing invocation.

## Entry Points

- `Callable`
- `CallableCustom`
- `CallableCustomBind`
- `CallableCustomMethodPointerBase`
- `Callable::call()`
- `Callable::bind()`

## Flow Notes

- `Callable` can target objects, lambdas/custom callables, or method pointer wrappers.
- Bound arguments are appended or unbound at call time depending on the callable wrapper.
- Signals, deferred calls, Array helpers, and script APIs all route through callable invocation paths.

## Code Links

- `core/variant/callable.h`
- `core/variant/callable.cpp`
- `core/variant/callable_bind.*`
- `core/variant/variant_callable.*`
- `core/object/callable_mp.*`