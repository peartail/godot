# MainLoop And Time

## Scope

Base main-loop lifecycle and time/date APIs.

## Entry Points

- `MainLoop`
- `MainLoop::initialize()`
- `MainLoop::process()`
- `MainLoop::physics_process()`
- `MainLoop::finalize()`
- `Time`

## Flow Notes

- `MainLoop` is driven by `Main::iteration()`.
- SceneTree is the most common MainLoop implementation.
- Time APIs expose ticks, Unix time, date dictionaries, and time-zone helpers.

## Code Links

- `core/os/main_loop.*`
- `core/os/time.*`
- `core/os/time_enums.h`
- `main/main.cpp`