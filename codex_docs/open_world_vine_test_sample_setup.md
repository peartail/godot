# OpenWorld 덩굴 테스트 샘플 생성

## 목적

마우스 조작 없이 Creeping, Climbing, Hanging, TreeWrap, Explicit Anchor 샘플을 생성한다.

생성 결과:

```text
res://generated/vine_tc/
├─ explicit_anchor.tres
├─ creeping.tres
├─ climbing.tres
├─ hanging.tres
├─ tree_wrap.tres
└─ vine_test_samples.tscn
```

## 샘플 배치

| 샘플 | 지지체 | 주요 검증 |
|---|---|---|
| ExplicitAnchor | 없음 | 급회전 frame, 결정론 |
| Creeping | PlaneMesh | 바닥 투영, surface offset |
| Climbing | BoxMesh | 수직 상승, 부착 normal |
| Hanging | 없음 | endpoint와 sag |
| TreeWrap | 생성 트리 | support graph와 radius |

## Headless 생성 스크립트

프로젝트에 `res://tools/create_vine_test_samples.gd`로 저장한다.

```gdscript
extends SceneTree

const OUT := "res://generated/vine_tc"
var sample_root := Node3D.new()
var profile := OpenWorldVineGenerationProfile.new()

func _initialize() -> void:
    DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(OUT))
    sample_root.name = "VineTestSamples"
    get_root().add_child(sample_root)
    _configure_profile()
    _add_plane()
    _add_box()
    _add_tree()
    _bake_explicit()
    _bake_surface("Creeping", OpenWorldVineGenerationRequest.MODE_CREEPING,
        NodePath("../SupportPlane"), Vector3(0, 0.4, 0), Vector3.RIGHT)
    _bake_surface("Climbing", OpenWorldVineGenerationRequest.MODE_CLIMBING,
        NodePath("../ClimbBox"), Vector3(4.7, 0.2, 0), Vector3.UP)
    _bake_hanging()
    _bake_tree_wrap()
    var packed := PackedScene.new()
    assert(packed.pack(sample_root) == OK)
    assert(ResourceSaver.save(packed, OUT + "/vine_test_samples.tscn") == OK)
    quit()

func _configure_profile() -> void:
    profile.stem_radius = 0.055
    profile.tip_radius_scale = 0.2
    profile.radial_sides = 5
    profile.segment_length = 0.25
    profile.side_branch_count = 2
    profile.leaf_spacing = 0.35
    profile.leaf_density = 1.0
    profile.surface_offset = 0.04
    profile.support_probe_distance = 0.8
    profile.surface_gap_policy = OpenWorldVineGenerationProfile.SURFACE_GAP_SWITCH_TO_HANGING

func _own(node: Node) -> void:
    sample_root.add_child(node)
    node.owner = sample_root

func _add_plane() -> void:
    var support := MeshInstance3D.new()
    support.name = "SupportPlane"
    var plane := PlaneMesh.new()
    plane.size = Vector2(5, 5)
    support.mesh = plane
    _own(support)

func _add_box() -> void:
    var support := MeshInstance3D.new()
    support.name = "ClimbBox"
    support.position = Vector3(4, 1, 0)
    var box := BoxMesh.new()
    box.size = Vector3(1, 2, 1)
    support.mesh = box
    _own(support)

func _add_tree() -> void:
    var tree := OpenWorldTreeGenerator3D.new()
    tree.name = "SupportTree"
    tree.position = Vector3(-4, 0, 0)
    tree.auto_generate = false
    tree.generation_profile = OpenWorldTreeGenerationProfile.new()
    tree.seed = 1207
    _own(tree)
    tree.generate_tree()

func _request(mode: int, seed: int) -> OpenWorldVineGenerationRequest:
    var request := OpenWorldVineGenerationRequest.new()
    request.profile = profile
    request.mode = mode
    request.seed = seed
    request.desired_length = 4.0
    request.branch_budget = 2
    return request

func _bake(label: String, request: OpenWorldVineGenerationRequest) -> void:
    var generator := OpenWorldVineGenerator3D.new()
    generator.auto_generate = false
    sample_root.add_child(generator)
    generator.generation_request = request
    var validation := generator.validate_request()
    assert(validation.success, JSON.stringify(validation))
    generator.generate_vine()
    var report := generator.get_generation_report()
    assert(report.success, JSON.stringify(report))
    var variant := generator.create_baked_variant()
    assert(ResourceSaver.save(variant, OUT + "/" + label.to_snake_case() + ".tres") == OK)
    var runtime := OpenWorldVine3D.new()
    runtime.name = label
    runtime.variant = variant
    _own(runtime)
    sample_root.remove_child(generator)
    generator.free()

func _bake_explicit() -> void:
    var request := _request(OpenWorldVineGenerationRequest.MODE_HANGING, 1101)
    request.explicit_anchors = PackedVector3Array([
        Vector3(-3, 0.1, 3), Vector3(-2.5, 0.3, 3.2),
        Vector3(-2, 0.8, 3), Vector3(-1.5, 0.4, 2.8), Vector3(-1, 1, 3)])
    _bake("ExplicitAnchor", request)

func _bake_surface(label: String, mode: int, support: NodePath,
        start: Vector3, direction: Vector3) -> void:
    var request := _request(mode, 2202 if mode == OpenWorldVineGenerationRequest.MODE_CREEPING else 3303)
    request.support_path = support
    request.start_position = start
    request.start_direction = direction
    _bake(label, request)

func _bake_hanging() -> void:
    var request := _request(OpenWorldVineGenerationRequest.MODE_HANGING, 4404)
    request.start_position = Vector3(-3, 4, -2)
    request.target_enabled = true
    request.target_position = Vector3(2, 3, -2)
    request.desired_length = 6.0
    _bake("Hanging", request)

func _bake_tree_wrap() -> void:
    var request := _request(OpenWorldVineGenerationRequest.MODE_TREE_WRAP, 5505)
    request.support_path = NodePath("../SupportTree")
    _bake("TreeWrap", request)
```

## 실행

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe `
  --headless --path C:\MyProject `
  --script res://tools/create_vine_test_samples.gd
```

완료 후 `vine_test_samples.tscn`을 열고 [테스트 케이스](open_world_vine_test_cases.md)의 TC-01~08을 확인한다.
