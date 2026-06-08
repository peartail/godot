# OpenWorldTerrain Architecture

## 목적

`OpenWorldTerrain`은 기존 `SimpleTerrain`을 변경하지 않고, 대규모 지형 렌더링을 실험하기 위한 별도 엔진 모듈이다.

`SimpleTerrain`은 CPU 높이 배열을 청크 메시로 변환하는 편집 중심 구조이고, `OpenWorldTerrain`은 height texture를 GPU에서 샘플링해 vertex displacement하는 렌더링 중심 구조다.

## 모듈 위치

- 런타임 모듈: `modules/open_world_terrain`
- 데이터 리소스: `OpenWorldTerrainData`
- 렌더링 노드: `OpenWorldTerrain3D`
- 문서 XML: `modules/open_world_terrain/doc_classes`

## 현재 단계

현재 구현은 실험용 1차 스켈레톤이다.

- 단일 patch mesh 생성
- `PackedFloat32Array` height data 저장
- `Image.FORMAT_RF` height image 생성
- `ImageTexture` 업로드
- built-in shader material에서 vertex displacement
- ClassDB API와 doc class XML 공개

## 데이터 흐름

1. `OpenWorldTerrainData.height_data`에 정규화 높이값을 저장한다.
2. `OpenWorldTerrainData.create_height_image()`가 `FORMAT_RF` 이미지를 만든다.
3. `OpenWorldTerrain3D`가 이미지를 `ImageTexture`로 업로드한다.
4. `OpenWorldTerrain3D`의 built-in shader가 height texture를 샘플링한다.
5. vertex shader에서 `VERTEX.y`를 `height * height_scale`만큼 이동한다.

## SimpleTerrain과의 차이

`SimpleTerrain3D`는 실제 메시 vertex 높이를 CPU에서 바꾼다. 브러시 편집 후에는 영향을 받은 청크 메시를 다시 만든다.

`OpenWorldTerrain3D`는 메시 자체를 평면 패치로 유지한다. 높이 변화는 texture update와 shader displacement로 처리하는 방향을 목표로 한다.

## 앞으로의 확장 후보

- height texture partial update
- clipmap 또는 quadtree patch LOD
- virtual texture 또는 tiled heightmap streaming
- GPU normal map 생성
- compute shader 기반 brush edit
- collision용 저해상도 proxy 생성
- 에디터 전용 OpenWorldTerrain paint mode

