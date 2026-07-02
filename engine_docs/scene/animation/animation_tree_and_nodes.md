# Animation Tree And Nodes

## Scope

Animation tree graph evaluation, blend tree nodes, blend spaces, state machines, and extension animation nodes.

## Entry Points

- `AnimationTree`
- `AnimationNodeBlendTree`
- `AnimationNodeBlendSpace1D`
- `AnimationNodeBlendSpace2D`
- `AnimationNodeStateMachine`
- `AnimationNodeExtension`

## Flow Notes

- AnimationTree evaluates animation nodes and sends mixed output to AnimationMixer.
- Blend spaces and blend trees combine animation sources using parameter state.
- State machines own transitions, playback travel, and state graph behavior.

## Code Links

- `scene/animation/animation_tree.*`
- `scene/animation/animation_blend_tree.*`
- `scene/animation/animation_blend_space_1d.*`
- `scene/animation/animation_blend_space_2d.*`
- `scene/animation/animation_node_state_machine.*`
- `scene/animation/animation_node_extension.*`