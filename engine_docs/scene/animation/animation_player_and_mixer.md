# Animation Player And Mixer

## Scope

Animation playback, mixing, libraries, track application, method/audio callbacks, and cache update behavior.

## Entry Points

- `AnimationPlayer`
- `AnimationMixer`
- `AnimationLibrary`
- `Animation`

## Flow Notes

- `AnimationPlayer` manages named animations and playback state.
- `AnimationMixer` applies sampled track data to nodes/resources.
- Animation resources and libraries live under `scene/resources`.

## Code Links

- `scene/animation/animation_player.*`
- `scene/animation/animation_mixer.*`
- `scene/resources/animation.*`
- `scene/resources/animation_library.*`