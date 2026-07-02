# Tween And Easing

## Scope

Tween creation, tweeners, process/pause behavior, property/method interpolation, and easing equations.

## Entry Points

- `Tween`
- `Tweener`
- `PropertyTweener`
- `MethodTweener`
- `CallbackTweener`
- `Easing` functions

## Flow Notes

- Tweens are processed by SceneTree or bound nodes depending on creation mode.
- Tweeners define individual interpolation/callback steps.
- Easing equations are shared by tween and animation-like interpolation behavior.

## Code Links

- `scene/animation/tween.*`
- `scene/animation/easing_equations.h`
- `scene/main/scene_tree.*`