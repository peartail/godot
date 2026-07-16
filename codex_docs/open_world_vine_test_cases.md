# OpenWorld 덩굴 테스트 케이스

## 자동 테스트

엔진 테스트 그룹:

```text
[OpenWorldVine]
```

검증 범위:

1. Agent-First Request와 editor auto-instantiation
2. explicit anchor 결정론
3. stem/foliage 두 surface
4. LOD triangle 감소와 Wind vertex color
5. Hanging endpoint와 sag
6. 충돌체 없는 MeshInstance Creeping/Climbing projection
7. Tree support graph와 TreeWrap
8. Variant 저장·재로드와 support metadata
9. 잘못된 support의 구조화 error code
10. Variant LOD와 support-loss 정책

실행 가능한 샘플 구성은 [테스트 샘플 생성](open_world_vine_test_sample_setup.md)을 따른다.

## TC-01 Explicit Anchor

입력:

- 6개 이상의 굽은 anchor
- 동일 Profile과 seed
- leaf density 1

확인:

- 반복 생성 mesh vertex 배열 동일
- LOD0/1/2 생성
- 각 LOD에 stem/foliage surface 존재
- LOD0 > LOD1 > LOD2 triangle
- UV, tangent, color 배열 존재

## TC-02 Creeping

샘플 구성:

```text
Node3D
├─ MeshInstance3D SupportPlane
└─ OpenWorldVineGenerator3D
```

Request:

- mode: Creeping
- support_path: `../SupportPlane`
- start: `(0, 0.4, 0)`
- direction: `(1, 0, 0)`

확인:

- 별도 PhysicsBody 없이 생성
- anchor가 `surface_offset` 높이를 유지
- 최대 경사 초과 시 중단
- 같은 seed에서 path 동일

## TC-03 Climbing

샘플 구성:

- BoxMesh 또는 curved MeshInstance
- 표면 가까운 start point

확인:

- anchor normal이 표면 바깥을 향함
- 위쪽 bias에 따라 높이가 증가
- 최대 gap을 넘으면 profile 정책 적용
- STOP은 마지막 부착점에서 종료
- SWITCH_TO_HANGING은 attached flag가 0으로 변경

## TC-04 Hanging

입력:

- start `(0, 4, 0)`
- target `(5, 4, 0)`
- sag `1.0`

확인:

- 첫 점과 마지막 점 유지
- 중간점이 직선보다 아래에 위치
- 첫 anchor만 attached
- support_path 없이 Headless 생성

## TC-05 TreeWrap

샘플 구성:

```text
Node3D
├─ OpenWorldTreeGenerator3D Tree
└─ OpenWorldVineGenerator3D Vine
```

확인:

- Tree 생성 후 support graph가 존재
- graph path 0은 trunk
- primary branch graph의 parent는 trunk
- Vine anchor가 radius+offset 바깥에 위치
- tree seed와 vine seed가 같으면 반복 결과 동일
- Tree Variant에 support graph가 보존

## TC-06 Runtime LOD

확인:

- LOD1/LOD2 거리에서 메시 전환
- max distance 이후 hidden
- camera_path 미지정 시 viewport camera 사용
- `force_lod_update()` 즉시 반영

## TC-07 Support Loss

- KEEP: 기존 메시 유지
- HIDE: 노드 숨김
- DETACH: `support_detach_requested` signal 발생

실제 물리 낙하는 이번 범위가 아니다.

## TC-08 오류 보고

다음을 각각 실행한다.

- request 없음
- profile 없음
- zero length
- 한 개 explicit anchor
- 존재하지 않는 support_path
- 빈 MeshInstance
- Tree 생성 전 TreeWrap

확인:

- crash 없음
- `success=false`
- 해결 가능한 error code와 속성명 출력

## Headless 완료 기준

1. 텍스트 Request 작성
2. `validate_request()` 성공
3. `generate_vine()` 실행
4. report JSON 출력
5. Variant 저장
6. Variant 재로드
7. LOD와 metadata 비교

마우스 입력이나 Inspector 버튼은 요구하지 않는다.
