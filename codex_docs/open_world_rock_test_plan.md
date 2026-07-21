# OpenWorld 바위 시스템 테스트 계획

## 목적

Rock Generator가 GUI 조작 없이 결정론적 topology, 정적 LOD와 collision을 생성·저장·재로드하는지 검증한다.

## 필수 Generation Report

- mode와 seed
- source point, hull vertex와 face 수
- discarded·duplicate point 수
- base contact point 수와 base plane
- LOD별 vertex, triangle과 AABB
- collision point와 face 수
- topology hash
- warning/error code와 해결 가능한 메시지

## 자동 테스트

### 결정론

- 같은 Profile, Request와 seed의 source point 일치
- hull index와 vertex 배열 일치
- LOD0/1/2와 collision 배열 일치
- topology hash 일치
- 다른 seed는 bounds 제한 안에서 다른 silhouette 생성

### 형태

- BOULDER는 세 축이 Profile 허용 범위에 있음
- SLAB은 Y축 비율과 strata 양자화 기준을 만족
- SHARD는 지정 주축이 다른 축보다 김
- base contact point가 공통 평면에 있음
- pivot과 base plane이 세 LOD에서 동일

### Mesh

- winding과 normal이 외부 방향
- flat normal과 facet 경계 유지
- UV 또는 triplanar 보조 좌표 배열 크기 일치
- Vertex Color RGBA mask 범위가 0~1
- LOD triangle 수가 단조 감소
- 모든 vertex, normal과 tangent에 NaN·Inf 없음

### Collision과 Bake

- convex collision이 visual bounds를 벗어나지 않음
- collision point 수가 Profile 제한 이하
- Variant 저장·재로드 후 mode, seed와 bounds 유지
- LOD mesh와 collision shape가 재로드 후 유효

### 오류 내구성

- request/profile 누락
- zero·negative size
- 너무 적은 explicit point
- 중복점과 모든 점이 공선·공면인 입력
- 극단적인 aspect ratio와 point count
- 실패 시 crash 없이 구조화된 error code 반환

## Headless 시나리오

1. `.tres` 또는 코드로 Profile과 Request 작성
2. `validate_request()` 결과를 JSON으로 출력
3. `generate_topology()`와 `generate_rock()` 실행
4. report와 topology hash 저장
5. Variant 저장·재로드
6. LOD, collision과 metadata 비교
7. 다른 seed batch를 생성하고 결과 통계 비교

## 회귀 검증

- 기존 OpenWorldTree 전체 테스트
- 기존 OpenWorldVine 전체 테스트
- Mono editor tests-enabled 빌드
- Mono glue와 Debug/Release assemblies 생성
- agent docs에서 모든 Rock 공개 클래스 조회

## 수동 Visual TC

- 세 Mode가 멀리서도 구분되는지 확인
- 바닥에 떠 있거나 과도하게 파묻히지 않는지 확인
- LOD 전환 시 pivot 이동과 큰 silhouette pop 확인
- flat facet 조명과 moss/upward mask 확인
- collision debug shape가 외형과 크게 어긋나지 않는지 확인

Visual TC는 결과 검증용이며 제작에 마우스 입력을 요구하지 않는다.
