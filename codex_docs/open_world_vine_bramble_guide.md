# OpenWorld BRAMBLE 생성 가이드

## 용도

`MODE_BRAMBLE`은 벽·지형·나무 support 없이 독립적으로 서 있는 둥근 덩굴 덤불을 생성한다.
생성 결과는 다른 덩굴처럼 LOD0/1/2와 `OpenWorldVineVariant`로 Bake할 수 있다.

## 좌표와 입력 규약

- `start_position`: 덤불 타원체의 밑면 중심
- `desired_length`: 각 줄기의 목표 polyline 길이
- `branch_budget + 1`: 요청이 허용하는 최대 줄기 수
- `bramble_stem_count`: Profile이 허용하는 최대 줄기 수
- `support_path`: 사용하지 않으며 지정하면 `BRAMBLE_SUPPORT_IGNORED` warning을 반환

실제 줄기 수는 다음과 같다.

```text
min(profile.bramble_stem_count, request.branch_budget + 1)
```

## Headless 생성 샘플

```gdscript
var profile := OpenWorldVineGenerationProfile.new()
profile.segment_length = 0.18
profile.stem_radius = 0.045
profile.bramble_radius = 1.4
profile.bramble_height = 1.0
profile.bramble_stem_count = 9
profile.bramble_tangle_strength = 0.72
profile.bramble_surface_bias = 0.58
profile.bramble_ground_anchor_ratio = 0.5
profile.thorn_density = 0.8
profile.thorn_spacing = 0.22
profile.leaf_cluster_card_count = 3

var request := OpenWorldVineGenerationRequest.new()
request.profile = profile
request.mode = OpenWorldVineGenerationRequest.MODE_BRAMBLE
request.seed = 4417
request.start_position = Vector3.ZERO
request.desired_length = 5.0
request.branch_budget = 8

var generator := OpenWorldVineGenerator3D.new()
generator.auto_generate = false
generator.generation_request = request

var validation := generator.validate_request()
assert(validation.success)
generator.generate_vine()
print(JSON.stringify(generator.get_generation_report(), "  "))

var variant := generator.create_baked_variant()
ResourceSaver.save(variant, "res://generated/bramble_4417.owvinevariant")
```

## 형태 조정

| 목적 | 조정값 |
|---|---|
| 넓고 낮은 덤불 | radius 증가, height 감소 |
| 둥근 덤불 | radius와 height를 비슷하게 설정 |
| 내부가 복잡한 덤불 | tangle 증가, surface bias 감소 |
| 외곽 실루엣 강조 | surface bias 증가 |
| 뿌리 고정감 증가 | ground anchor ratio 증가 |
| 촘촘한 덤불 | stem count와 branch budget을 함께 증가 |

## Report 확인

- `bramble_stem_count`: 실제 생성된 독립 줄기 수
- `bramble_bounds`: 생성 경로의 로컬 AABB
- `anchor_count`: 전체 줄기의 anchor 합계
- `total_length`: 전체 줄기 길이 합계
- `lod*.thorns`, `lod*.leaf_cards`: LOD별 장식 통계

같은 Profile, Request와 seed에서는 path와 mesh가 동일해야 한다.
시각 품질 확인 전 report와 저장·재로드 결과부터 headless로 검증한다.
