# Profiling Metadata

## Scope

Core profiling metadata structures, generated profiling header, and profiling builder script.

## Entry Points

- `profiling.h`
- `profiling.gen.h`
- `profiling_builders.py`

## Flow Notes

- Profiling metadata is generated for engine profiling hooks.
- Runtime performance counters exposed through `Performance` live under `main`, not this core profiling folder.
- Generated profiling files should be updated through their builder script.

## Code Links

- `core/profiling/profiling.h`
- `core/profiling/profiling.cpp`
- `core/profiling/profiling.gen.h`
- `core/profiling/profiling_builders.py`
- `main/performance.*`