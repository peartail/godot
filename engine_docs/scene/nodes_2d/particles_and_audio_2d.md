# 2D Particles And Audio

## Scope

2D CPU/GPU particles, particle materials, audio listeners, and positional 2D audio stream players.

## Entry Points

- `CPUParticles2D`
- `GPUParticles2D`
- `AudioListener2D`
- `AudioStreamPlayer2D`

## Flow Notes

- CPU particles simulate on the CPU, while GPU particles use rendering server particle systems.
- Particle process materials are shared resources under `scene/resources`.
- 2D audio players mix through audio server with viewport/world-position context.

## Code Links

- `scene/2d/cpu_particles_2d.*`
- `scene/2d/gpu_particles_2d.*`
- `scene/2d/audio_listener_2d.*`
- `scene/2d/audio_stream_player_2d.*`
- `scene/resources/particle_process_material.*`