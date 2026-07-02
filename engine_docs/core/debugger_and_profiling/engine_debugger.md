# Engine Debugger

## Scope

Debugger singleton behavior, debugger/profiler registration, breakpoints, capture callbacks, and message routing.

## Entry Points

- `EngineDebugger`
- `EngineProfiler`
- `EngineDebugger::register_profiler()`
- `EngineDebugger::send_message()`

## Flow Notes

- Engine debugger code is core-level so scripts, servers, and editor debugging can share it.
- Profilers can be registered and activated by name.
- Message routing is used by remote debugging and editor integrations.

## Code Links

- `core/debugger/engine_debugger.h`
- `core/debugger/engine_debugger.cpp`
- `core/debugger/engine_profiler.*`