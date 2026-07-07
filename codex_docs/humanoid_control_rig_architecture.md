# HumanoidControlRig3D 구조와 개발 방향

## 배경

`HumanoidControlRig3D`는 glTF/FBX 등으로 이미 Godot에 import된 캐릭터를 대상으로, `Skeleton3D`의 bone hierarchy를 읽어 기본적인 control rig를 자동 구성하는 1차 MVP 기능이다.

이 기능은 MetaHuman `.dna`, RigLogic, Unreal Control Rig, Epic proprietary runtime을 사용하지 않는다. 현재 방향은 Godot의 MIT-safe 런타임 안에서 `Skeleton3D`, `Marker3D`, `TwoBoneIK3D`, `LookAtModifier3D` 같은 기존 노드를 조합해 캐릭터 조작 레이어를 만드는 것이다.

## 현재 구현 위치

- 런타임 노드: `scene/3d/humanoid_control_rig_3d.h`
- 구현: `scene/3d/humanoid_control_rig_3d.cpp`
- ClassDB 등록: `scene/register_scene_types.cpp`
- Class reference 문서: `doc/classes/HumanoidControlRig3D.xml`

`scene/3d/SCsub`는 `*.cpp`를 자동 포함하므로 새 source 파일을 별도로 추가하지 않아도 빌드된다.

## 현재 공개 API

`HumanoidControlRig3D`는 `Node3D`를 상속하며, `Skeleton3D`의 자식으로 사용하는 것을 전제로 한다.

Properties:

- `auto_setup_on_ready: bool`
- `create_hand_ik: bool`
- `create_foot_ik: bool`
- `create_head_look_at: bool`
- `control_distance_scale: float`

Methods:

- `setup_from_skeleton()`
- `clear_generated_rig()`
- `get_control_node(control_name: StringName) -> Node`
- `get_missing_bones() -> PackedStringArray`

## 현재 생성 구조

`setup_from_skeleton()`을 호출하면 기존 생성물을 먼저 제거한 뒤, 가능한 control과 modifier를 다시 만든다.

생성 노드는 모두 `ControlRig_` prefix를 사용한다. 이 prefix는 `clear_generated_rig()`가 사용자가 직접 만든 노드와 자동 생성 노드를 구분하기 위한 최소한의 소유권 경계다.

Generated control nodes:

- `ControlRig_hand_l`
- `ControlRig_hand_r`
- `ControlRig_foot_l`
- `ControlRig_foot_r`
- `ControlRig_elbow_l`
- `ControlRig_elbow_r`
- `ControlRig_knee_l`
- `ControlRig_knee_r`
- `ControlRig_head_look`

Generated modifier nodes:

- `ControlRig_ArmIK`
- `ControlRig_LegIK`
- `ControlRig_HeadLookAt`

Control nodes는 `HumanoidControlRig3D`의 자식으로 생성된다. Modifier nodes는 parent `Skeleton3D`의 자식으로 생성된다. 이는 기존 `SkeletonModifier3D` 처리 모델이 `Skeleton3D`의 child modifier를 순서대로 처리하는 구조와 맞추기 위한 것이다.

## 현재 데이터 흐름

```text
Imported glTF/FBX character
        ↓
Skeleton3D bone hierarchy
        ↓
HumanoidControlRig3D.setup_from_skeleton()
        ↓
bone alias lookup
        ↓
Marker3D control targets
        ↓
TwoBoneIK3D / LookAtModifier3D
        ↓
Skeleton3D pose modification
```

`HumanoidControlRig3D`는 bone pose를 직접 수정하지 않는다. 현재 책임은 skeleton 분석, control target 생성, 기존 modifier 노드 연결이다.

## Bone 매핑 전략

현재 구현은 bone 이름을 비교할 때 대소문자와 `_`, `-`, `.`, 공백을 무시한다. 예를 들어 `upper_arm_l`, `UpperArm.L`, `upperarm-l` 같은 이름은 같은 계열로 비교될 수 있다.

현재 alias 범위는 다음 계열을 포함한다.

- Godot/일반 humanoid: `upper_arm_l`, `lower_arm_l`, `hand_l`, `thigh_l`, `calf_l`, `foot_l`, `head`
- UE 스타일: `upperarm_l`, `lowerarm_l`, `thigh_l`, `calf_l`, `foot_l`, `head`
- Mixamo 스타일: `mixamorig:LeftArm`, `mixamorig:LeftForeArm`, `mixamorig:LeftHand`, `mixamorig:LeftUpLeg`, `mixamorig:LeftLeg`, `mixamorig:LeftFoot`

체인 단위로 필요한 bone이 모두 발견된 경우에만 해당 IK가 생성된다. 예를 들어 왼팔의 `upper_arm_l`, `lower_arm_l`, `hand_l` 중 하나라도 없으면 왼팔 IK는 생성하지 않고 `missing_bones`에 `left arm IK chain`을 기록한다.

## 현재 검증 상태

확인된 검증:

- Windows Mono editor build 성공
- `--generate-mono-glue` 성공
- C# assemblies rebuild 성공
- 전체 humanoid smoke test 성공: modifier 3개, control 9개 생성
- partial skeleton smoke test 성공: head만 있을 때 head look-at만 생성하고 누락 chain 보고

현재 알려진 비기능성 경고:

- ANGLE dependency 설치 경고
- 기존 `SplineMesh3D` C# member hiding warning

이 경고들은 현재 control rig 구현 실패와 직접 관련이 없다.

## 설계 방향

1차 방향은 "Godot-native control rig assembly"다.

즉, 외부 proprietary rig 데이터를 직접 실행하는 것이 아니라, import된 skeleton 위에 Godot 노드 기반 control layer를 구성한다. 이 접근은 다음 장점이 있다.

- glTF/FBX 등 일반 asset pipeline과 호환된다.
- Epic `.dna`/RigLogic 라이선스 문제를 피한다.
- 기존 Godot modifier 생태계와 자연스럽게 통합된다.
- editor에서 생성된 control/IK 노드를 직접 확인하고 조정할 수 있다.

반대로 현재 한계도 명확하다.

- bone alias가 코드에 하드코딩되어 있다.
- pole/control 초기 위치가 heuristic offset이다.
- head look-at의 forward axis는 `+Z`로 고정되어 있다.
- FK/IK switch, space switching, constraint stack, baking은 없다.
- face morph/ARKit curve 매핑은 아직 포함하지 않는다.

## 다음 개발 단계

### 1. Rig Profile 리소스화

하드코딩된 bone alias와 axis 기본값을 `Resource`로 분리한다.

예상 리소스:

- `HumanoidControlRigProfile`
- bone alias map
- default control list
- chain definitions
- forward axis / pole direction defaults
- naming convention presets

이를 통해 MetaHuman-style, Mixamo, VRM, Blender Rigify 같은 preset을 엔진 코드 변경 없이 추가할 수 있다.

### 2. Editor 생성 워크플로

현재는 script/API 호출 중심이다. 다음 단계에서는 editor action을 추가하는 것이 좋다.

예상 UX:

- `Skeleton3D` 선택
- context menu 또는 toolbar에서 "Create Humanoid Control Rig"
- 생성 전 preview: found bones / missing bones / controls to create
- 생성 후 control node selection과 gizmo editing

### 3. Control 저장과 재생성 정책

현재 `setup_from_skeleton()`은 생성물을 지우고 다시 만든다. 앞으로는 두 모드가 필요하다.

- Rebuild: generated nodes 재생성
- Update Links: 사용자가 옮긴 control transform은 유지하고 modifier NodePath만 갱신

이 정책이 생기면 사용자가 control 위치를 수동 조정한 뒤에도 rig를 안정적으로 유지할 수 있다.

### 4. Axis 자동 추정

현재 pole direction과 look-at forward axis는 보수적인 기본값이다. 다음 단계에서는 rest pose를 기준으로 bone 방향을 추정해야 한다.

필요 기능:

- child bone 위치 기반 local forward axis 추정
- left/right mirror detection
- knee/elbow plane 추정
- model scale 기반 control offset 계산

### 5. Face Control 2차 MVP

Body IK 이후에는 face morph driver를 별도 MVP로 추가한다.

예상 방향:

- `MeshInstance3D`의 blend shape 목록 탐색
- ARKit-style curve 이름과 blend shape 이름 매핑
- `apply_face_curves(Dictionary curves)` 형태의 runtime API
- jaw, blink, smile, brow 계열부터 시작

이 단계도 `.dna`/RigLogic 없이 glTF/FBX에 포함된 morph target을 대상으로 한다.

## 장기 방향

장기적으로는 `HumanoidControlRig3D`를 단순 자동 생성기에서 "rig profile + generated modifier graph" 시스템으로 발전시키는 것이 좋다.

최종 목표는 다음과 같다.

- import된 humanoid skeleton 자동 인식
- profile 기반 control rig 생성
- body IK, look-at, twist, retarget, face morph를 하나의 authoring workflow로 연결
- generated rig를 editor에서 조정 가능
- animation 위에 additive로 적용
- 필요 시 baked animation으로 export 가능

이 방향은 MetaHuman 같은 고품질 캐릭터뿐 아니라 일반 humanoid asset에도 적용 가능하다. 핵심은 특정 vendor 포맷을 직접 의존하지 않고, Godot 내부의 skeleton/modifier/render pipeline 위에 재사용 가능한 control layer를 만드는 것이다.
