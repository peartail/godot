# Engine Singleton

## Scope

Runtime singleton registry, engine metadata, frame/timing settings, movie/shader paths, and global debug flags.

## Entry Points

- `Engine`
- `Engine::add_singleton()`
- `Engine::get_singleton_object()`
- `Engine::set_physics_ticks_per_second()`
- `Engine::set_time_scale()`

## Flow Notes

- Engine singletons expose process-wide services to scripts and native systems.
- Timing settings are consumed by main loop iteration and physics scheduling.
- Some flags are configured from command-line options during `Main::setup()`.

## Code Links

- `core/config/engine.h`
- `core/config/engine.cpp`
- `main/main.cpp`