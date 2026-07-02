# Script Language Hooks

## Scope

Script instance attachment, language backend integration, script debugging hooks, extension script languages, and script backtrace capture.

## Entry Points

- `ScriptLanguage`
- `ScriptInstance`
- `ScriptLanguageExtension`
- `ScriptBacktrace`
- `Object::set_script()`

## Flow Notes

- `ScriptLanguage` defines the backend interface implemented by GDScript, CSharp, and extension languages.
- `ScriptInstance` is attached to an `Object` to provide script-defined behavior and properties.
- `ScriptLanguageExtension` exposes the language backend interface to GDExtension.
- Script backtraces are collected through language-specific implementations.

## Code Links

- `core/object/script_language.h`
- `core/object/script_language.cpp`
- `core/object/script_instance.h`
- `core/object/script_instance.cpp`
- `core/object/script_language_extension.*`
- `core/object/script_backtrace.*`