# SimpleTerrain Material Pipeline

## Material Priority

`SimpleTerrain3D`는 모든 chunk instance에 적용할 material을 하나 선택한다.

우선순위:

1. `terrain_material`
2. built-in height triplanar material
3. inherited `material_override`

## Built-In Triplanar Material

활성화:

```text
use_builtin_triplanar_material = true
```

shader는 lazy 생성된다.

custom material을 사용하는 경우 built-in shader를 만들지 않는다.

## Height Blend

높이 layer:

- low.
- mid.
- high.

속성:

- `triplanar_low_height`
- `triplanar_high_height`
- `triplanar_blend_width`

`smoothstep`을 사용해 height band를 부드럽게 섞는다.

## Triplanar Projection

texture는 world space에서 샘플링한다.

projection 축:

- YZ.
- XZ.
- XY.

world normal을 기준으로 projection을 blend한다.

## 현재 한계

- splatmap painting 없음.
- layer별 normal/roughness map 없음.
- slope 기반 material rule 없음.
- chunk별 material variation 없음.

## 다음 후보

`OpenWorldTerrain`에서는 height texture 기반 GPU displacement와 별도로 material texture streaming을 검토한다.
