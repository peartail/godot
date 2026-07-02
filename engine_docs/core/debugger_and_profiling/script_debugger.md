# Script Debugger

## Scope

Script-level debugger interface, break/step hooks, stack inspection, and language debugger integration.

## Entry Points

- `ScriptDebugger`
- `ScriptLanguage`
- `ScriptBacktrace`

## Flow Notes

- Script debugger interfaces are called by language backends.
- Stack and breakpoint behavior depends on each script language implementation.
- Core only defines the shared interface and routing points.

## Code Links

- `core/debugger/script_debugger.*`
- `core/object/script_language.*`
- `core/object/script_backtrace.*`