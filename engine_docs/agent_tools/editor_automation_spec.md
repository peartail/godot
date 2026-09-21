# Godot Editor Automation — 설계 스펙

## 상태와 목적

- 작성일: 2026-09-20. 갱신: 2026-09-21. 상태: **구현 전 설계 초안. 단계 0 검증 완료**.
- 사용자가 요청한 산출물은 스펙 문서다. 이 문서 작성으로 구현·빌드·배포를 수행하지 않는다.
- 단계 0은 검증을 마쳤다. 다음 세션은 미결정 4번을 닫고 단계 1을 시작한다. 명령 이름과 내부 클래스명은 제안이다.
- 목표: CLI에서 실행 중인 커스텀 Godot에 요청하여 씬 편집, 생성, 리로드, 촬영을 수행한다.
- 복잡한 작업은 GDScript 전체를 한 요청에 담아 반복·조건·계산·비동기 대기를 실행한다.
- CLI와 향후 MCP 어댑터가 동일한 엔진 기능을 사용한다. MCP 서버 자체는 초기 범위에서 제외한다.

관련 문서:

- [요청·응답 및 작업 프로토콜](editor_automation_protocol.md)
- [스크립트 실행 계약](editor_automation_scripts.md)
- [단계 0 기술 검증 결과](editor_automation_stage0_findings.md)
- [현재 내장 문서 조회 CLI](agent_docs_cli.md)

## 조사 기준

- 조사한 바이너리: `4.8.dev.mono.custom_build.5ab5c7df0`.
- 문서 작성 시 소스 HEAD: `780a02d9777c6159a273644102c641a8d510a594`.
- 바이너리와 HEAD는 서로 다른 시점이다. 구현 세션에서 빌드와 API를 다시 대조한다.
- `--agent-docs-class`로 EditorInterface, JSONRPC, GDScript, 지형·바위 생성 API를 조회했다.
- 문서 조회에서 로그·캐시 경로 쓰기 오류와 `SplineMesh3D.mesh` 중복 등록 오류가 출력됐다.
- 이 조사는 API·소스 확인이다. 동적 실행, 원격 편집, 촬영 성공을 검증한 것은 아니다.

## 구조

```text
CLI: 단일 명령 / JSON 파일 / GDScript 파일 / 표준입력
  → localhost TCP + JSON-RPC
  → 에디터 내 명령 서버 → 작업 큐 → 메인 스레드에서 실행
      ├─ 기존 에디터·노드·리소스 API
      └─ GDScript 실행기 + 실행 컨텍스트
  ← 작업 상태 / 결과 / 오류 / 아티팩트 경로
```

- 초기 서버는 커스텀 엔진의 editor 전용 C++ 기능으로 구현하는 방향을 제안한다.
- 서버는 명시적 실행 옵션으로만 켜며, 일반 게임 실행·export template에서는 제공하지 않는다.
- CLI는 에디터에 접속하는 별도 클라이언트다. `godotctl`은 가칭이다.
- 기존 `godot --script`는 별도 프로세스를 실행하므로 열린 에디터에 접속하는 기능을 대신하지 않는다.
- 네트워크 요청에서 SceneTree·에디터 객체를 직접 다른 스레드로 수정하지 않는다.
- 기존 JSONRPC와 Godot 네트워크 기능을 재사용한다. 별도 범용 RPC 프레임워크는 도입하지 않는다.
- 전체 엔진 API를 개별 명령으로 복제하지 않는다. 자주 쓰는 명령과 전체 스크립트 실행을 함께 제공한다.

## 기능 범위와 순서

| 단계 | 범위 | 완료 조건 |
| --- | --- | --- |
| 0 | GDScript 동적 실행 기술 검증 | **완료.** [결과](editor_automation_stage0_findings.md) |
| 1 | 연결·상태·작업 관리·씬 조회·리로드 | 세션 식별, 미저장 보호, 완료 확인, 구조화된 오류 |
| 2 | script.run + 최소 ctx | 파라미터, 반복·조건, 비동기 대기, 결과·로그·취소 확인 |
| 3 | 편집·저장·촬영 | 메시 생성→배치→저장→재로드→촬영 흐름 검증 |
| 4 | 복잡한 저작 작업 | 지형 브러시·생성기·베이크, 리소스 편집, 명령 묶음 |
| 후속 | runtime 제어·재시작·MCP | 별도 스펙 확장 후 구현 |

단계 2는 선택적인 부가 기능이 아니다. 복잡한 작업을 한 요청으로 수행하는 핵심 범위다.
최초 실사용 버전은 단계 0~3을 포함하고, 단계 4는 실제 필요한 도구부터 추가한다.

## 명령 범주

| 범주 | 제안 명령 | 규칙 |
| --- | --- | --- |
| 조회 | status, capabilities, scene.tree, object.describe | 세션·씬·지원 기능·속성 타입 확인 |
| 씬 | scene.open, scene.reload, scene.save | 대상 경로 명시, 기본 자동 저장 없음 |
| 파일 | filesystem.scan, resource.reimport | 실제 완료 대기, import 중 재진입 방지 |
| 편집 | node.create, scene.instantiate, object.set | owner·Undo·수정 상태 처리 |
| 촬영 | viewport.capture | editor 뷰포트와 출력 경로 명시 |
| 코드 | script.run | 파일 또는 소스와 JSON 인자 수신 |
| 묶음 | batch.run | 순차 실행, 실패 시 후속 중단, 자동 롤백 없음 |
| 작업 | job.get, job.cancel | 진행·결과·협력적 취소 |

`capabilities`는 실제 지원 명령만 반환한다. 미구현 명령을 성공 또는 빈 결과로 처리하지 않는다.

## 편집 의미

- 대상은 `session + scene path + scene generation + NodePath`로 식별한다.
- 현재 선택 노드나 현재 탭을 암묵적 대상으로 사용하지 않는다. `.`은 고정된 대상 씬 루트다.
- 로컬 transform과 world transform을 구분한다. 회전 입력은 단위도 명시한다.
- 새 노드는 적절한 owner를 설정하고 저장·재로드 후에도 존재하는지 검증한다.
- 일반 편집 명령과 ctx 편집 함수는 EditorUndoRedoManager를 통해 Undo를 기록한다.
- 긴 await 구간에 미완료 Undo 액션을 열어 두지 않는다. 적용 단위로 액션을 완료한다.
- 공유 리소스 수정과 복제 후 할당을 구분한다. 모호한 공유 리소스 편집은 명시적 정책을 요구한다.
- 외부 리소스 저장은 씬 저장과 별개다. 실제 기록한 파일 목록을 결과에 포함한다.
- 상속 씬·인스턴스 내부 편집은 에디터 규칙을 따른다. 미지원 override는 오류로 반환한다.
- 생성·검증·베이크·보고를 구분한다. 생성 결과의 저장 여부는 명령에서 명시한다.
- UI 버튼 안에만 있는 로직은 공용 작업 함수로 분리하고 UI와 CLI가 같이 사용한다.
- 직접 API 호출로 발생한 모든 부작용의 Undo를 자동 보장하지 않는다.

## 리로드와 촬영

- `scene.reload`는 열린 씬에만 적용하며 미저장 정책 기본값은 `error`다.
- `save`, `discard`는 요청에 명시한 경우에만 허용한다. save 실패 시 reload하지 않는다.
- 현 구현은 reload 시 씬을 다시 읽고 Undo 기록을 지운다. 응답과 문서에 이를 드러낸다.
- 파일 스캔, 재임포트, 씬 reload, 게임 재시작, C# 어셈블리 reload는 각각 다른 작업이다.
- C# 즉석 컴파일, C# 어셈블리 reload, 에디터 재시작은 초기 버전 범위에 포함하지 않는다.
- 씬이 교체되면 generation을 바꾼다. 기존 노드 참조는 다시 해석해야 한다.
- 에디터 재시작 후 작업 재개는 향후 CLI 재연결 기능에서 처리한다. 스크립트 연속 실행을 약속하지 않는다.
- 촬영은 렌더링 중인 에디터 viewport 0~3 중 하나를 지정한다.
- 초기 촬영은 기존 카메라 구도를 사용한다. 카메라 배치·gizmo 숨김·runtime 촬영은 별도 확장이다.
- 렌더링 완료 후 이미지를 얻는다. 단순 요청 접수나 고정 sleep을 완료 조건으로 사용하지 않는다.
- 일반 `--headless`는 렌더링을 끈다. 촬영 불가 상태에는 `RENDERING_UNAVAILABLE`을 반환한다.
- 결과에는 파일 경로, 이미지 크기, 촬영 대상과 관련 상태를 포함한다.

## 제안 방향과 미결정 사항

설계 방향은 로컬 에디터 서버, CLI, GDScript 실행, 최소 실행 컨텍스트다.
아래 1·2·5·6은 [단계 0 검증](editor_automation_stage0_findings.md) 결과로, 3은 결정으로 닫혔다.
남은 미결정은 4뿐이다.

### 1. C++ 작업 관리와 GDScript async 연결 — 결정됨

보조 실행기는 필요 없다. 코루틴 `run`을 호출하면 `GDScriptFunctionState`가 즉시 반환되고,
그 객체의 `completed` 시그널이 최종 반환값을 실어 방출된다. C++에서 여기에 연결한다.

`resume()`과 `is_valid()`는 스크립트에 바인딩되어 있지 않다. 상태 폴링은 C++에서만 가능하다.

### 2. 실행 오류의 job 귀속과 종료 판정 — 결정됨

`add_error_handler()`로 수집하고, 핸들러가 받는 `file`을 job이 만든 스크립트의
`gdscript://<instance_id>.gd`와 대조해 귀속한다. 이 경로는 `GDScript` 생성자에서 부여된다.

`reload()` 반환값과 `call()` 반환값에는 오류 내용이 없으므로 이 경로 외에 대안이 없다.
한계: 사용자 스크립트가 다른 스크립트를 호출하다 난 오류는 `file`이 달라 놓친다.
또한 이 포크의 기존 `SplineMesh3D` 오류가 스트림에 섞이므로 수집기가 걸러야 한다.

종료 판정은 오류 수집과 별개다. 완료되지 않는 코루틴을 엔진이 알려주지 않으므로
**모든 job은 자체 deadline을 가진다.**

### 3. 최초 CLI 구현 언어와 배포 방식 — 결정됨

**Python으로 시작한다.** 이 저장소는 이미 SCons를 통해 Python에 의존하며
(`scripts/build.ps1`도 `python -m SCons`로 폴백한다), 주 사용자는 에이전트다.

이는 배포용 최종 선택이 아니다. 프로토콜이 JSON-RPC over TCP이므로 클라이언트는 교체 가능하다.
일반 사용자 배포가 필요해지면 엔진 실행 파일의 client mode를 나중에 추가한다.
검증되지 않은 프로토콜에 C++ 클라이언트 비용을 먼저 쓰지 않는다.

### 4. 세션 발견 파일의 접근 제어와 token 저장 — 미결정

유일하게 남은 미검증 항목이다. localhost 바인딩만으로는 같은 머신의 다른 프로세스를 막지 못한다.
Windows 사용자 단위 접근 제어와 token 저장 위치·수명을 단계 1 착수 전에 확인한다.

### 5. 직접 편집 시 추적 한계 — 범위 결정으로 대체

측정 과제가 아니라 경계를 정하는 문제로 다룬다. 세 가지를 분리한다.

| 항목 | 방침 |
| --- | --- |
| generation (낡은 NodePath 거부) | 유지한다. 리로드 후 잘못된 노드 조작을 막는 안전장치다 |
| 씬 수정 상태(dirty) | 유지한다. ctx 편집이 Undo를 거치면 따라온다 |
| 임의 직접 API 호출의 완전 추적 | 포기한다. 작성자 책임이다 |

dirty를 유지하는 이유는 Undo 기능 자체가 아니라 **에디터가 그 위에 얹혀 있기 때문이다.**
`EditorNode::_update_unsaved_cache()`는 `EditorUndoRedoManager::is_history_unsaved()`만 본다.
`EditorUndoRedoManager`를 우회해 편집하면 씬이 dirty로 표시되지 않고, 닫을 때 저장 확인이 뜨지 않으며,
미저장 기본 정책이 `error`인 `scene.reload`가 거부하지 않고 그대로 리로드해 변경을 조용히 버린다.

### 6. 동시 편집 충돌과 프레임 응답성 — 분리해서 결정

두 문제이며 답이 다르다.

- **대량 생성 시 응답성**: 작업 큐와 `ctx.checkpoint()` 프레임 양보로 해결한다.
  남은 것은 설계가 아니라 측정이다. 프레임당 처리량은 실제 지형·바위 생성으로 정한다.
- **동시 GUI 편집 충돌**: 큐잉으로 해결되지 않는다. 큐는 job들끼리를 직렬화할 뿐
  사람의 뷰포트 편집을 막지 못한다. 모두 메인 스레드에서 돌아 데이터 레이스는 없으나,
  checkpoint로 양보한 사이에 대상 노드가 사라질 수 있다. 이는 5의 generation·생존 확인으로 막는다.

### 7. 컴파일 오류 메시지 전달 — 신규, 미결정

단계 0에서 새로 드러난 항목이다. `GDScript::reload()`는 오류 코드만 반환하고 메시지를 주지 않으며,
`// TODO: Show all error messages.` 때문에 첫 오류 하나만 출력한다.
프로토콜의 `COMPILE_ERROR`가 여러 오류를 담으려면 엔진 `GDScript::reload()` 수정이 필요하다.
초기 범위에 포함할지 결정한다. 포함하지 않으면 첫 오류만 보고한다고 문서에 명시한다.

## 검증 및 다음 세션 인수인계

1. 저장소 AGENTS.md와 이 문서의 연결 문서를 읽고 기존 미커밋 변경을 보존한다.
2. 단계 0은 완료다. [검증 결과](editor_automation_stage0_findings.md)의 "구현에 넘기는 제약" 6개를 먼저 반영한다.
3. 임시 테스트 프로젝트에서만 생성·저장·reload를 검증한다. 사용자 맵을 사용하지 않는다.
4. 잘못된 세션·토큰·타입·NodePath·씬 세대에는 부작용 없이 명확한 오류를 반환한다.
5. 미저장 씬 reload 거부와 명시적 save/discard를 각각 검증한다.
6. 메시 생성·배치 후 Undo/Redo와 저장·reload 결과의 노드·리소스·transform을 확인한다.
7. 복잡한 GDScript 1회 요청으로 여러 노드 생성, 조건부 배치, await, 결과 반환을 확인한다.
8. 파싱 오류·부분 변경 후 오류·취소·클라이언트 연결 종료를 각각 검증한다.
9. 렌더링 모드에서 실제 PNG의 내용·크기를 확인한다. headless에서는 촬영 실패를 검증한다.
10. 엔진 빌드는 scripts/build.ps1을 사용한다. 공개 바인딩 변경 시 MonoGlue·agent docs도 검증한다.

## 근거 코드

- [CLI 시작점](../../main/main.cpp), [JSONRPC](../../modules/jsonrpc/jsonrpc.cpp)
- [EditorInterface](../../editor/editor_interface.cpp), [씬 reload](../../editor/editor_node.cpp)
- [EditorScript](../../editor/script/editor_script.cpp), [GDScript](../../modules/gdscript/gdscript.h)
- [Undo](../../doc/classes/EditorUndoRedoManager.xml), [저장 owner](../../doc/classes/PackedScene.xml)
- [Viewport](../../doc/classes/Viewport.xml), [RenderingServer](../../doc/classes/RenderingServer.xml)
- [기존 runtime debugger](../../scene/debugger/scene_debugger.cpp)
- [바위 생성 API](../../modules/open_world_terrain/doc_classes/OpenWorldRockGenerator3D.xml)
- [지형 API](../../modules/open_world_terrain/doc_classes/OpenWorldTerrain3D.xml)
