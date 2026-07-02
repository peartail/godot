# Main

Engine startup, command-line parsing, main loop setup, and application coordination.

## Global Flow

- `Main::setup()`: parse command-line options, load project settings, choose drivers, and initialize core systems.
- `Main::setup2()`: finish runtime/editor setup after project and server initialization.
- `Main::start()`: create the main loop, launch editor/project/runtime modes, and run command-line actions.
- `Main::iteration()`: advance physics, process, navigation, audio, rendering, and frame timing.
- `Main::cleanup()`: shut down main loop, servers, globals, and platform-facing services.

## Feature Categories

- [Boot And Setup](boot_and_setup.md): staged initialization, startup state, and shutdown ownership.
- [Command Line](command_line.md): CLI parsing, help output, mode selection, and command-line tools.
- [Project And Resources](project_and_resources.md): project discovery, settings, packed data, translation, and resource loading.
- [Runtime Loop](runtime_loop.md): frame iteration, physics/process timing, redraw, and quit handling.
- [Display And Window](display_and_window.md): display driver selection, window flags, resolution, scaling, and boot splash.
- [Server Startup](server_startup.md): rendering, audio, physics, text, XR, camera, accessibility, and theme server setup.
- [Editor And Tools](editor_and_tools.md): editor/project manager startup, import waiting, recovery mode, and editor-only CLI tools.
- [Debug And Profiling](debug_and_profiling.md): debug flags, profiler setup, FPS output, collision/path/navigation visualization, and movie writing.
- [Performance Monitors](performance_monitors.md): built-in and custom performance metrics exposed through `Performance`.
- [Frame Timing](frame_timing.md): physics tick scheduling, delta smoothing, fixed FPS, and low-processor timing.
- [Generated Assets](generated_assets.md): generated splash and application icon headers.
- [Steam Tracking](steam_tracking.md): optional Steam launch/install tracking helpers.

## Code Links

- `main/main.h`
- `main/main.cpp`
- `main/main_timer_sync.h`
- `main/main_timer_sync.cpp`
- `main/performance.h`
- `main/performance.cpp`
- `main/steam_tracker.h`
- `main/steam_tracker.cpp`
- `main/main_builders.py`
- `main/SCsub`
- `core/config/project_settings.*`