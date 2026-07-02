# OS Abstraction

## Scope

Platform-facing process, environment, filesystem, window-independent, execution, and system service APIs.

## Entry Points

- `OS`
- `OS::get_singleton()`
- `OS::execute()`
- `OS::create_process()`
- `OS::get_environment()`

## Flow Notes

- Platform implementations provide concrete behavior behind `OS`.
- Engine code should prefer `OS` and core abstractions over platform-specific calls.
- Some APIs are unavailable or behave differently per platform/export target.

## Code Links

- `core/os/os.h`
- `core/os/os.cpp`
- `platform/`