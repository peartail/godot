# Blender Tree GLB Export Guide

이 문서는 Blender에서 Godot로 나무를 전달하는 GLB 문서의 인덱스다. 세부 내용은 180줄 이하로 분리했다.

## 문서 구성

- [전달 파일과 Node/Collection/LOD 구조 계약](blender_tree_asset_structure_contract.md)
- [Material, UV, Wind, Collision, Custom Property 계약](blender_tree_material_wind_collision_contract.md)
- [glTF 설정, 체크리스트, Godot 검증과 Acceptance Contract](blender_tree_glb_settings_and_validation.md)
- [Blender 스타일라이즈드 나무 제작 문서](blender_stylized_tree_asset_guidelines.md)

## 기본 전달 흐름

```text
Blender source collections
        ↓ Apply/Validate
LOD meshes + materials + metadata
        ↓ glTF 2.0 export
GLB package
        ↓ Godot import
OpenWorldTreeVariant 또는 보조 generator module
```

## 주의

- `.blend`는 편집 원본이고 `.glb`는 엔진 전달 파일이다.
- Godot 네이티브 generator만 사용하는 나무에는 GLB가 필요하지 않다.
- Palm frond, hero root, 특수 canopy처럼 별도 제작한 module에는 이 계약을 적용한다.
