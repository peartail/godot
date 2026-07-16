# OpenWorld Tree 버그 리포트 양식

한 리포트에는 한 가지 현상만 기록한다. 재현 파일과 스크린샷에는 같은 ID를 사용한다.

## 기본 정보

```text
Bug ID: OWT-
제목:
작성일:
Engine commit/build:
Renderer:
GPU / OS:
Test case: TC-
```

## 재현 조건

```text
Archetype:
Seed:
Crown shape:
Root style:
Secondary branch count/scale:
Branch droop:
Palm frond count/length/width/droop:
Trunk material:
Foliage material / cull mode:
```

## 재현 절차

1.
2.
3.

## 결과

```text
실제 결과:
기대 결과:
재현율: /10
심각도: Crash / Data loss / Major / Visual / Minor
회귀 여부: 신규 / 이전에도 발생 / 알 수 없음
```

## 증거

- 정면 스크린샷:
- 측면 스크린샷:
- 상단 스크린샷:
- `.tres` profile:
- Bake된 variant 또는 재현 scene:
- 콘솔 로그/백트레이스:

## 빠른 분류

- `FOL-S0`: foliage surface가 생성되지 않음
- `FOL-MAT`: surface는 있으나 재질 연결 문제
- `FOL-CULL`: 시점/culling에 따라 사라짐
- `FOL-GEO`: frond/blob 형상 품질 문제
- `BR-GEO`: 가지 접합 또는 삼각형 오류
- `ROOT-GEO`: 뿌리 접지 또는 형상 오류
- `SEED-DET`: 같은 seed의 결과가 달라짐
- `BAKE-IO`: Bake, 저장, 재로드 오류
- `EDITOR`: bounds, 선택, inspector 오류
- `CRASH`: 에디터 또는 런타임 크래시

## 샘플 결과표

| ID | Archetype | Seed | 판정 | 분류 | 비고 |
|---|---|---:|---|---|---|
| OWT-001 | Palm | 1207 | Fail | FOL-CULL | 측면에서 사라짐 |
| OWT-002 | Mangrove | 2402 | Pass | - | - |
