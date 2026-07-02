# 3D Skeleton And IK

## Scope

3D skeletons, bone attachments, IK solvers, transform modifiers, constraints, spring bones, and retargeting helpers.

## Entry Points

- `Skeleton3D`
- `BoneAttachment3D`
- `SkeletonIK3D`
- `SkeletonModifier3D`
- `BoneConstraint3D`
- `SpringBoneSimulator3D`
- `RetargetModifier3D`

## Flow Notes

- Skeleton nodes manage bone pose state and skinning-facing transforms.
- IK/modifier nodes alter bone poses through specialized solver pipelines.
- Spring bones and constraints often depend on update order and target transforms.

## Code Links

- `scene/3d/skeleton_3d.*`
- `scene/3d/bone_attachment_3d.*`
- `scene/3d/*ik_3d.*`
- `scene/3d/*modifier_3d.*`
- `scene/3d/*constraint_3d.*`
- `scene/3d/spring_bone*`