# OpenWorld 덩굴 Agent API 가이드

## 기본 흐름

모든 제작 작업은 마우스 없이 다음 API 순서로 실행할 수 있다.

```text
Profile + Request
      ↓ validate_request
      ↓ generate_path
      ↓ generate_vine
      ↓ create_baked_variant
      ↓ ResourceSaver.save
```

입력 좌표와 explicit anchor는 `OpenWorldVineGenerator3D` 로컬 좌표다. `support_path`는 Generator 기준 상대 `NodePath`다.

## GDScript Headless 예제

```gdscript
extends SceneTree

func _initialize() -> void:
    var profile := OpenWorldVineGenerationProfile.new()
    profile.stem_radius = 0.055
    profile.segment_length = 0.35
    profile.leaf_spacing = 0.42

    var request := OpenWorldVineGenerationRequest.new()
    request.profile = profile
    request.mode = OpenWorldVineGenerationRequest.MODE_HANGING
    request.seed = 1207
    request.start_position = Vector3(0, 4, 0)
    request.target_enabled = true
    request.target_position = Vector3(5, 3, 0)
    request.desired_length = 6.0

    var generator := OpenWorldVineGenerator3D.new()
    generator.auto_generate = false
    generator.generation_request = request

    var validation := generator.validate_request()
    if not validation.success:
        print(JSON.stringify(validation))
        quit(1)
        return

    generator.generate_vine()
    var report := generator.get_generation_report()
    print(JSON.stringify(report, "  "))

    var variant := generator.create_baked_variant()
    var error := ResourceSaver.save(variant, "res://generated/hanging_1207.owvinevariant")
    quit(0 if error == OK else 2)
```

실행 예:

```powershell
godot.windows.editor.dev.x86_64.mono.console.exe `
  --headless --path C:\MyProject `
  --script res://tools/generate_vine.gd
```

## C# 예제

```csharp
var profile = new OpenWorldVineGenerationProfile {
    StemRadius = 0.055f,
    SegmentLength = 0.35f,
    LeafSpacing = 0.42f,
};

var request = new OpenWorldVineGenerationRequest {
    Profile = profile,
    Mode = OpenWorldVineGenerationRequest.VineMode.Hanging,
    Seed = 1207,
    StartPosition = new Vector3(0, 4, 0),
    TargetEnabled = true,
    TargetPosition = new Vector3(5, 3, 0),
    DesiredLength = 6.0f,
};

var generator = new OpenWorldVineGenerator3D {
    AutoGenerate = false,
    GenerationRequest = request,
};

Godot.Collections.Dictionary validation = generator.ValidateRequest();
if ((bool)validation["success"]) {
    generator.GenerateVine();
    OpenWorldVineVariant variant = generator.CreateBakedVariant();
    ResourceSaver.Save(variant, "res://generated/hanging_1207.owvinevariant");
}
```

## Mode별 입력

### Creeping

- `support_path`: `OpenWorldTerrain3D` 또는 `MeshInstance3D`
- `start_position`: 표면 probe 범위 안
- `start_direction`: 바닥 진행 방향
- `desired_length`: 목표 길이

### Climbing

- `support_path`: `MeshInstance3D`
- `start_position`: 메시 표면 근처
- `start_direction`: 초기 surface 탐색과 접선 힌트

### Hanging

- support가 필요 없다.
- `target_enabled`가 true면 두 점 사이 sag 경로를 만든다.
- false면 방향·길이·중력 편향으로 끝점을 계산한다.

### TreeWrap

- `support_path`: 생성 완료된 `OpenWorldTreeGenerator3D`
- 트리의 `get_generated_support_graph()`를 사용한다.
- 트리 Variant에도 graph가 함께 Bake된다.

### Bramble

- support 없이 둥글게 얽힌 독립형 가시덤불을 생성한다.
- `start_position`은 덤불 밑면 중심이다.
- `desired_length`는 줄기 하나의 목표 길이다.
- 실제 줄기 수는 `min(bramble_stem_count, branch_budget + 1)`이다.
- `bramble_radius`, `bramble_height`, `bramble_tangle_strength`로 실루엣을 제어한다.
- 상세 샘플은 [BRAMBLE 가이드](open_world_vine_bramble_guide.md)를 참고한다.

## 명시 Anchor

`explicit_anchors`에 두 점 이상을 넣으면 mode solver와 support query를 건너뛴다.

```gdscript
request.explicit_anchors = PackedVector3Array([
    Vector3(0, 0, 0),
    Vector3(1, 0.1, 0.2),
    Vector3(2, 0.4, 0),
])
```

`explicit_normals`는 생략하거나 anchor와 같은 개수로 제공한다. 생략 시 `Vector3.UP`을 사용한다.

## Report 확인

주요 필드:

- `success`, `errors`, `error_codes`, `warnings`, `warning_codes`
- `mode`, `seed`
- `anchor_count`, `branch_count`, `total_length`
- `surface_gap_anchor`, `max_surface_gap`, `frame_error_count`
- `lod0`, `lod1`, `lod2`

각 LOD Dictionary는 surface, vertex, triangle과 AABB 통계를 제공한다.

## 품질·Decoration 파라미터

- `leaf_cluster_card_count`: LOD0 클러스터 카드 수
- `leaf_cluster_spread`: 카드 사이 방사형 간격
- `leaf_size_variation`: seed 기반 카드 크기 편차
- `thorn_density`: 가시 후보 유지 확률, 0이면 비활성
- `thorn_spacing`, `thorn_size`: 가시 간격과 길이
- `branch_junction_scale`: 가지 collar 최대 굵기
- `branch_junction_length`: collar 길이, 0이면 비활성
- `bramble_surface_bias`: 내부 밀도와 외곽 실루엣 사이의 편향
- `bramble_ground_anchor_ratio`: 바닥에 고정되어 Wind R값을 낮게 갖는 줄기 비율

가시와 collar는 Surface 0, 잎 클러스터는 Surface 1을 유지한다. LOD1은 카드와 가시를 줄이고 LOD2는 단일 잎 카드와 가시 없는 ribbon을 사용한다. LOD report에는 `leaf_clusters`, `leaf_cards`, `thorns`, `junctions`가 포함된다.
