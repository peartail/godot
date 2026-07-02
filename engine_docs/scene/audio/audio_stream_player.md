# Audio Stream Player

## Scope

Non-positional audio stream playback, bus routing, polyphony, playback state, and shared audio player internals.

## Entry Points

- `AudioStreamPlayer`
- `AudioStreamPlayerInternal`
- `AudioStreamPlayback`

## Flow Notes

- `AudioStreamPlayer` is the base non-2D/non-3D playback node.
- 2D and 3D stream players share internal playback logic with positional additions.
- Audio mixing routes through AudioServer and configured audio buses.

## Code Links

- `scene/audio/audio_stream_player.h`
- `scene/audio/audio_stream_player.cpp`
- `scene/audio/audio_stream_player_internal.*`
- `scene/2d/audio_stream_player_2d.*`
- `scene/3d/audio_stream_player_3d.*`
- `servers/audio/`