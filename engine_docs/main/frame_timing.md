# Frame Timing

## Scope

Physics tick scheduling, delta smoothing, refresh-rate estimation, fixed FPS, and frame delay.

## Main Entry Points

- `MainTimerSync::init()`
- `MainTimerSync::set_cpu_ticks_usec()`
- `MainTimerSync::set_fixed_fps()`
- `MainTimerSync::get_average_physics_steps()`
- `Main::iteration()`

## Code Links

- `main/main_timer_sync.h`
- `main/main_timer_sync.cpp`
- `main/main.cpp`