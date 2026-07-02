# Random

## Scope

Engine random number generation primitives and script-facing random generator resource.

## Entry Points

- `RandomPCG`
- `RandomNumberGenerator`
- `Math::random()` helpers

## Flow Notes

- `RandomPCG` is the low-level generator used by engine code.
- `RandomNumberGenerator` wraps random behavior for object/script APIs.
- Deterministic behavior depends on seed/state handling and should be preserved carefully.

## Code Links

- `core/math/random_pcg.h`
- `core/math/random_pcg.cpp`
- `core/math/random_number_generator.*`
- `core/math/math_funcs.*`