# Godot Stylized Tree Generator 1차 가이드

## 목적과 범위

`OpenWorldTreeGenerator3D`는 Blender에서 완성된 나무 변형을 가져오는 도구가 아니라, Godot 안에서 파라미터와 seed로 카툰풍 정적 나무 메시를 만드는 제작 노드다.

1차 버전은 다음 파이프라인을 제공한다.

```text
OpenWorldTreeGenerationProfile + seed
                 ↓
OpenWorldTreeGenerator3D (미리보기/제작)
                 ↓ Bake Variant...
OpenWorldTreeVariant (.tres, LOD0)
                 ↓
OpenWorldTreeSpecies → OpenWorldTree3D 배치/렌더링
```

생성 결과는 스켈레톤이나 런타임 절차 메시가 아니라 저장 가능한 정적 `ArrayMesh`다. 동일한 프로필과 seed는 동일한 정점 데이터를 만든다.

## 메시 생성 방식

### Seed 기반 trunk

`tree_height`를 `trunk_segments`만큼 나누어 중심선을 만들고 seed 기반의 작은 수평 굽힘을 더한다. 각 중심선 지점에 `trunk_radial_sides`개의 링 정점을 만들고 `trunk_base_radius`에서 `trunk_tip_radius`까지 반지름을 줄여 연결한다.

### Interval / phyllotaxy 가지

가지는 `branch_start_ratio`부터 `branch_end_ratio`까지 `branch_interval` 간격으로 붙는다. 다음 가지의 방위각은 `phyllotaxy_angle_degrees`만큼 회전한다. 기본 137.5도는 가지가 한쪽에 겹치는 현상을 줄이는 황금각 분포다. 길이, 방향, 굽힘에는 seed 기반 편차가 적용된다.

### Tapered low-poly branch mesh

각 가지도 trunk와 같은 링 기반 튜브지만, 끝 반지름이 0에 가까워지는 테이퍼 메시다. trunk의 `trunk_radial_sides`를 공유하므로 5~8 정도가 카툰풍 실루엣과 비용 사이에서 적당하다.

### Canopy blob

Canopy blob은 잎 한 장이 아니라 수관을 덩어리로 표현하는 낮은 해상도의 각진 타원체다. 1차 버전은 세분화된 아이코사헤드론을 가지 끝과 나무 꼭대기에 배치한다. `canopy_blob_count`, 반지름 범위, 세로 비율, 위치 흔들림, 표면 거칠기로 둥글고 단순한 카툰 수관부터 울퉁불퉁한 수관까지 조절한다.

### 두 개의 surface

- Surface 0: trunk와 모든 branch
- Surface 1: 모든 canopy blob

따라서 나무 전체가 하나의 `ArrayMesh`이면서도 줄기 재질과 잎 재질을 따로 지정할 수 있다. `canopy_blob_count`가 0이면 foliage surface가 없으므로, 현재 `OpenWorldTree3D`용 일반 변형은 1 이상을 권장한다.

## 에디터 사용 순서

1. Scene에 `OpenWorldTreeGenerator3D` 노드를 추가한다.
2. `generation_profile`을 펼치거나 별도의 `.tres`로 저장해 형태를 조절한다.
3. `trunk_material`과 `foliage_material`을 지정한다.
4. seed를 직접 바꾸거나 Inspector 하단의 **Randomize Seed**를 누른다.
5. 자동 갱신을 껐다면 **Generate Tree**로 결과를 다시 만든다.
6. 원하는 결과에서 **Bake Variant...**를 누르고 `OpenWorldTreeVariant` `.tres`를 저장한다.
7. 여러 baked variant를 `OpenWorldTreeSpecies.variants`에 넣고 `OpenWorldTree3D`에 species와 placement data를 지정한다.

프로필 하나를 여러 생성기가 공유하면 프로필 변경 시 모두 갱신된다. 나무 하나만 수정하려면 Inspector에서 프로필을 **Make Unique** 또는 복제한 뒤 편집한다.

## 권장 1차 카툰 프리셋 범위

| 항목 | 권장 시작 범위 |
|---|---:|
| trunk radial sides | 5~8 |
| trunk segments | 5~10 |
| branch segments | 2~5 |
| branch interval | 0.45~0.9 m |
| phyllotaxy angle | 120~150° |
| canopy blob count | 6~16 |
| canopy vertical scale | 0.65~1.0 |
| canopy roughness | 0.05~0.2 |

실루엣 변화는 seed만으로 해결하기보다 높이·가지 길이·수관 크기가 조금씩 다른 프로필 2~4개와 각 프로필의 여러 seed를 함께 굽는 편이 좋다.

## 스크립트 API 예시

### GDScript

```gdscript
var generator := OpenWorldTreeGenerator3D.new()
var profile := OpenWorldTreeGenerationProfile.new()
profile.tree_height = 5.5
profile.trunk_radial_sides = 6
profile.canopy_blob_count = 10

generator.auto_generate = false
generator.generation_profile = profile
generator.seed = 1207
generator.generate_tree()

var variant: OpenWorldTreeVariant = generator.create_baked_variant()
ResourceSaver.save(variant, "res://trees/umbrella_1207.tres")
```

### C#

```csharp
var generator = new OpenWorldTreeGenerator3D {
    AutoGenerate = false,
    Seed = 1207,
    GenerationProfile = new OpenWorldTreeGenerationProfile {
        TreeHeight = 5.5f,
        TrunkRadialSides = 6,
        CanopyBlobCount = 10,
    },
};

generator.GenerateTree();
OpenWorldTreeVariant variant = generator.CreateBakedVariant();
ResourceSaver.Save(variant, "res://trees/umbrella_1207.tres");
```

정확한 멤버, 메서드, 신호 설명은 엔진 클래스 도움말의 `OpenWorldTreeGenerator3D`와 `OpenWorldTreeGenerationProfile`에서 볼 수 있다.

## 1차 버전에서 의도적으로 제외된 기능

- 뿌리 메시와 지면 적응
- LOD1/LOD2 자동 단순화 및 billboard
- 충돌 Shape/벌목 상태/낙하 물리 자동 생성
- 잎 카드, 개별 잎, 꽃과 열매
- wind vertex mask와 셰이더 애니메이션
- 메시 절단 및 목재 조각 생성
- UV atlas 제작 및 lightmap UV2

벌목 게임플레이는 baked variant의 `source_seed`, `collision_radius`, `collision_height`를 기초 메타데이터로 사용할 수 있다. 실제 상호작용 시에는 `OpenWorldTree3D.set_tree_enabled(id, false)`로 정적 인스턴스를 숨기고 별도의 물리 나무 씬을 스폰하는 구조가 적합하다.

## 다음 개발 단계 제안

1. 프로필에 trunk/foliage 색상과 wind mask 채널 규약 추가
2. LOD1 단순화 및 LOD2 blob 병합 또는 billboard 베이크
3. root flare와 1단계 자식 가지 추가
4. 충돌 프록시와 벌목용 interactive scene 변환 API 추가
5. 여러 seed를 일괄 생성·미리보기·species에 추가하는 Variant Batch Baker 추가
