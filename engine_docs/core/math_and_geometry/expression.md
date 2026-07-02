# Expression

## Scope

Expression parsing and evaluation for user-provided math-like expressions.

## Entry Points

- `Expression`
- `Expression::parse()`
- `Expression::execute()`

## Flow Notes

- Expressions parse text into an executable representation using Variant values.
- Execution can call built-in functions and access supplied inputs.
- Error reporting needs to remain useful for editor and script users.

## Code Links

- `core/math/expression.h`
- `core/math/expression.cpp`
- `core/variant/variant.*`