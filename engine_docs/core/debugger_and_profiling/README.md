# Debugger And Profiling

Core debugger routing, local and remote debugger implementations, script debugger hooks, profiler interfaces, and profiling metadata.

## Subtopics

- [Engine Debugger](engine_debugger.md): debugger registration, profiler registration, and message routing.
- [Local And Remote Debugger](local_and_remote_debugger.md): local command-line debugger and remote debugger peer flow.
- [Script Debugger](script_debugger.md): script debugging interface and script-facing debug hooks.
- [Profiling Metadata](profiling_metadata.md): profiling metadata generation and profiler data structures.

## Global Entry Points

- `EngineDebugger` coordinates debugger and profiler backends.
- `ScriptDebugger` provides language-facing debug hooks.
- Remote debugger peers bridge editor/debugger communication.

## Code Links

- `core/debugger/engine_debugger.*`
- `core/debugger/remote_debugger.*`
- `core/debugger/script_debugger.*`
- `core/profiling/profiling.*`