# Blender Stylized Tree Asset Development Guidelines

이 문서는 Blender Tree 제작 문서의 인덱스다. 세부 내용은 180줄 이하의 문서로 분리했다.

## 문서 구성

- [디자인 원칙, 변형 규칙, Scene 구성](blender_stylized_tree_design_and_scene.md)
- [Trunk/Branch/Canopy 모델링, Geometry Nodes, 재질과 Wind 데이터](blender_stylized_tree_modeling_and_materials.md)
- [LOD, Collision, 후보 선별, Godot Export와 검증](blender_stylized_tree_lod_export_validation.md)
- [GLB 익스포트 문서 인덱스](blender_tree_glb_export_guide.md)

## 사용 순서

1. 디자인/Scene 문서로 수형과 Collection 구조를 정한다.
2. 모델링/재질 문서로 trunk, branch, canopy와 vertex data를 제작한다.
3. LOD/검증 문서로 전달 메시를 검사한다.
4. GLB 익스포트 문서에 따라 Godot용 파일을 출력한다.

## 현재 Godot 생성기와의 관계

현재 `OpenWorldTreeGenerator3D`는 Godot 안에서 정적 나무를 생성하므로 Blender 완성 나무가 필수는 아니다. 이 Blender 문서군은 다음 경우에 사용한다.

- 특수 canopy 또는 leaf/frond module 제작
- generator로 만들기 어려운 hero tree
- 재질/UV/vertex channel 기준 자산 제작
- 외부 모델을 `OpenWorldTreeVariant`로 연결하는 보조 파이프라인
