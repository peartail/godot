# 3D Particles And Audio

## Scope

3D CPU/GPU particles, particle collision helpers, audio listeners, and 3D positional audio players.

## Entry Points

- `CPUParticles3D`
- `GPUParticles3D`
- `GPUParticlesCollision3D`
- `AudioListener3D`
- `AudioStreamPlayer3D`

## Flow Notes

- CPU particles simulate on the CPU; GPU particles use rendering server particles.
- Particle collision nodes provide collision fields for GPU particle systems.
- 3D audio players mix spatial audio through the audio server.

## Code Links

- `scene/3d/cpu_particles_3d.*`
- `scene/3d/gpu_particles_3d.*`
- `scene/3d/gpu_particles_collision_3d.*`
- `scene/3d/audio_listener_3d.*`
- `scene/3d/audio_stream_player_3d.*`