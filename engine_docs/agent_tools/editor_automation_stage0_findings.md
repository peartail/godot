# Editor Automation — 단계 0 기술 검증 결과

[상위 스펙](editor_automation_spec.md) · [프로토콜](editor_automation_protocol.md) · [스크립트 계약](editor_automation_scripts.md)

## 범위와 상태

- 작성일: 2026-09-21. 대상: 스펙 "기능 범위와 순서" 표의 **단계 0 (GDScript 동적 실행 기술 검증)**,
  그리고 스펙 미결정 **4번 (세션 파일 접근 제어와 token 저장)**.
- 검증 항목: 컴파일 오류, return, await, 실행 오류, 객체 수명, 그리고 세션 파일 권한과 token 생성.
- 구현·빌드·배포는 수행하지 않았다. 이 문서는 단계 1 이후 설계를 확정하기 위한 근거다.
- 소스 읽기와 **실제 실행**을 함께 수행했다. 결론마다 근거 유형을 표시한다.

## 검증 환경

- 실행 바이너리: `4.8.dev.mono.custom_build.1be0b1ae2` (2026-09-17 빌드).
- 소스 HEAD: `3cc7f2db5a`.
- 바이너리와 HEAD 사이 `modules/gdscript/` 변경은 4건이며 전부 테스트 추가·유틸리티·문서 링크다.
  동적 컴파일과 await 의미론에 영향이 없어 재빌드 없이 검증했다. 단계 1 착수 시 재빌드 후 재확인한다.
- 임시 프로젝트를 `--headless`로 실행했다. 사용자 맵(`shipAdventure`)은 사용하지 않았다.
- 실행 로그에 이 포크의 기존 오류 `Object 'SplineMesh3D' already has member 'mesh'`가 매번 출력된다.
  이번 작업과 무관하지만 **오류 스트림을 파싱하는 구현에는 잡음이 된다.**

## 결과 요약

| # | 질문 | 결론 | 근거 |
| --- | --- | --- | --- |
| 1 | 소스 문자열을 런타임에 컴파일할 수 있는가 | 가능 | 실행 |
| 2 | 컴파일 오류를 감지할 수 있는가 | 코드만 가능, 메시지는 불가 | 실행 + 소스 |
| 3 | run의 반환값을 받을 수 있는가 | 동기 호출은 가능 | 실행 |
| 4 | await가 있으면 어떻게 되는가 | `GDScriptFunctionState` 반환 | 실행 + 소스 |
| 5 | await 완료와 최종 반환값을 받을 수 있는가 | `completed` 시그널로 가능 | 실행 + 소스 |
| 6 | 실행 오류를 호출부에서 알 수 있는가 | 불가, 반환값에 신호 없음 | 실행 |
| 7 | 오류를 특정 job에 귀속할 수 있는가 | 합성 경로로 가능, 한계 있음 | 소스 |
| 8 | await 중 객체 수명이 유지되는가 | **유지되지 않음. 무증상 미완료** | 실행 |
| 9 | 인스턴스가 있는 스크립트를 reload할 수 있는가 | 불가 | 실행 + 소스 |

## 상세

### 1~3. 컴파일과 동기 반환

`GDScript.new()` → `source_code` 설정 → `reload()`로 컴파일된다. 정상 소스는 `reload()`가 `0 (OK)`을 반환하고,
`RefCounted`에 `set_script` 후 `call("run", ctx, args)`로 Dictionary를 그대로 돌려받는다.

```
A reload err = 0 (0 == OK)
A returned type = Dictionary
A returned value = { "count": 42, "kind": "sync" }
```

### 2. 컴파일 오류 — 코드는 잡히고 메시지는 못 잡는다

파싱 실패 시 `reload()`는 `43 (Parse error)`을 반환하고 `can_instantiate()`가 `false`가 된다. 여기까지는 계약에 쓸 수 있다.

**그러나 오류 메시지는 반환되지 않는다.** `GDScript::reload()`는 메시지를 `_err_print_error(..., ERR_HANDLER_SCRIPT)`로
출력만 하고 호출부에 전달하지 않는다(`modules/gdscript/gdscript.cpp:843`). GDScript 쪽에는 `get_errors` 같은 조회 API가 없다
(실행으로 확인: `has_method("get_errors") = false`).

오류 개수는 단계에 따라 다르다. **파싱 단계는 첫 오류 하나만** 출력하며 `// TODO: Show all error messages.`
주석이 붙어 있다(`gdscript.cpp:827`). **분석 단계는 오류 목록을 순회해 전부** 출력한다(`gdscript.cpp:848`).
즉 잘리는 것은 문법 오류뿐이고, 타입 오류처럼 서로 독립적인 분석 오류는 모두 보고된다.

→ 프로토콜이 `COMPILE_ERROR`에 줄 번호와 메시지를 담으려면 **C++에서 `add_error_handler()`로 오류를 수집해야 한다.**
`reload()` 반환값만으로는 "컴파일 실패" 이상을 말할 수 없다.

### 4~5. await — 기대대로 동작한다

`run`에 `await`가 있으면 `call()`은 즉시 **`GDScriptFunctionState`** 를 반환하고, 그 객체의 `completed` 시그널이
**최종 반환값을 실어** 방출된다.

```
C immediate return class = GDScriptFunctionState
C completed payload = { "awaited_frames": 3, "kind": "async" }
```

엔진 구현도 일치한다. `gdscript_vm.cpp:2650`에서 상태 객체를 만들고 `completed` 시그널을 연결하며,
`gdscript_vm.cpp:4020`의 `p_state->completed.emit(args, 1)`가 최종 반환값을 전달한다.
4015행 주석이 의도를 명시한다: *"Postpone the function exiting and the call stack clearing until the last `await` is completed."*

주의: `GDScriptFunctionState`가 스크립트에 노출하는 것은 `completed` 시그널과 `_signal_callback`뿐이다.
`resume()`과 `is_valid()`는 바인딩되어 있지 않으므로 **GDScript만으로는 상태를 폴링할 수 없고**, C++에서 직접 다뤄야 한다.

→ 스펙이 남겨둔 "작은 @tool GDScript 보조 실행기가 필요한가"에 대한 답: **await 완료 수신 자체에는 필요 없다.**
C++에서 `GDScriptFunctionState`의 `completed`에 연결하면 된다.

### 6. 실행 오류 — 호출부는 성공과 구별할 수 없다

`run` 내부에서 런타임 오류가 나면 엔진이 `SCRIPT ERROR:`를 출력하지만, `call()`은 오류를 알리지 않고
**빈 Dictionary를 반환한다.**

```
D calling ...
SCRIPT ERROR: Invalid access to property or key 'missing_key' on a base object of type 'Dictionary'.
   at: run (gdscript://-9223372008786491884.gd:5)
D returned type = Dictionary value = {  }
```

반환값만으로는 "정상적으로 빈 Dictionary를 반환한 경우"와 구별되지 않는다.
`Callable::CallError`는 잘못된 인자 수·타입 같은 **호출 실패**를 알려줄 뿐 스크립트 내부 런타임 오류에는 설정되지 않는다.

→ 프로토콜의 `SCRIPT_RUNTIME_ERROR`는 **오류 핸들러 수집에 의존해야 하며**, 반환값 검사로 대체할 수 없다.

### 7. 오류 귀속 — 합성 경로가 열쇠다

경로 없는 GDScript는 **생성자에서** 인스턴스 ID 기반 경로를 부여받는다(`modules/gdscript/gdscript.cpp:1337`):

```cpp
path = vformat("gdscript://%d.gd", get_instance_id());
```

이 경로가 컴파일 오류와 런타임 오류의 `at:` 위치로 그대로 출력된다(위 6번 로그의 `gdscript://-9223372008786491884.gd:5`).
오류 핸들러 콜백은 `(userdata, function, file, line, error, message, editor_notify, type)`를 받으므로
`file`을 job이 만든 스크립트의 `gdscript://<instance_id>.gd`와 대조하면 귀속이 가능하다.

한계 두 가지를 기록한다.

- 이 경로는 **`resource_path`로 노출되지 않는다.** 실행으로 확인한 결과 동적 스크립트 두 개의 `resource_path`는 모두 빈 문자열이다.
  C++에서 `get_instance_id()`로 직접 만들어야 한다.
- 사용자 스크립트가 **다른 스크립트를 호출하다 오류가 나면** `file`은 그쪽 경로가 된다.
  경로 일치만으로는 그 오류를 놓친다. 실행 중인 job이 하나라는 전제를 두거나 별도 범위 표시가 필요하다.

### 8. 객체 수명 — 가장 위험한 발견

**await 중에 인스턴스 참조를 놓으면 코루틴은 아무 오류 없이 영원히 완료되지 않는다.**

세 가지 경우를 분리해 확인했다.

| 경우 | 결과 |
| --- | --- |
| 스크립트·인스턴스 모두 보유 (대조군) | `completed = true`, 정상 반환 |
| **인스턴스 참조를 놓음** | 인스턴스 해제됨, **`completed` 영원히 미방출** |
| 스크립트 참조만 놓음 | `completed = true` (인스턴스가 스크립트를 붙잡음) |

1차 실행에서는 이 지점에서 프로세스가 그대로 멈춰 타임아웃으로 죽였다. 오류도, 경고도, 타임아웃도 없다.

→ 스펙 "실행 흐름" 4항(*async 대기 중 source 리소스, 인스턴스, ctx 및 continuation 수명을 유지한다*)은
권고가 아니라 **지키지 않으면 job이 조용히 영구 미완료가 되는 필수 조건**이다.
`GDScriptFunctionState`는 인스턴스를 살려두지 않는다. 결정적인 참조는 **스크립트가 아니라 인스턴스**다.
job 관리자는 자체 deadline을 반드시 가져야 한다. 엔진은 이 상황을 알려주지 않는다.

### 9. 인스턴스가 있는 스크립트는 reload 불가

```
F before = { "v": 1 }
ERROR: Cannot reload script while instances exist.
F reload err = 22 (Already in use)
F after = { "v": 1 }
```

`reload()`는 `ERR_ALREADY_IN_USE`를 반환하고 기존 인스턴스는 옛 코드를 유지한다(`gdscript.cpp:758`).

→ 스크립트 계약의 *"각 요청에 새 스크립트 인스턴스를 만든다"* 는 스타일 선택이 아니라 **엔진이 강제하는 제약**이다.
스크립트 객체를 재사용해 소스만 갈아끼우는 설계는 불가능하다.

또한 `reload()` 진입부에 `if (reloading) { return OK; }` 가 있어(`gdscript.cpp:742`)
**재진입 호출이 성공으로 보인다.** 중첩 실행 경로에서 이 반환값을 신뢰하면 안 된다.

## 스펙 미결정 사항에 대한 답

| 스펙 항목 | 이번 검증 결과 |
| --- | --- |
| 1. C++ 작업 관리와 GDScript async 연결 | `GDScriptFunctionState::completed`로 충분하다. 보조 실행기는 await 수신 목적으로는 불필요 |
| 2. 실행 오류의 job 귀속과 종료 판정 | `add_error_handler` + `gdscript://<instance_id>.gd` 대조로 가능. 타 스크립트 오류는 누락 |
| 4. 세션 파일 접근 제어와 token 저장 | `.godot/agent/`는 불가. 사용자 전용 경로로 옮긴다. 아래 절 참조 |
| 5. generation·수정 상태 추적 한계 | 범위 결정으로 대체. generation·dirty는 유지, 완전 추적은 포기 |

3(CLI 구현 언어)과 6(동시 편집·프레임 응답성)은 검증이 아닌 결정으로 닫혔다. 스펙 해당 절을 참조한다.

## 구현에 넘기는 제약

1. **job마다 새 `GDScript` 객체를 만든다.** 재사용 후 `reload()`는 `ERR_ALREADY_IN_USE`로 실패한다.
2. **인스턴스 참조를 job이 직접 보유한다.** 놓으면 무증상 영구 미완료가 된다.
3. **모든 job에 deadline을 둔다.** 완료되지 않는 코루틴을 엔진이 알려주지 않는다.
4. **오류 수집은 `add_error_handler`로 한다.** `reload()` 반환값과 `call()` 반환값은 오류 내용을 담지 않는다.
5. **스크립트 식별은 `gdscript://<instance_id>.gd`로 한다.** `resource_path`는 비어 있다.
6. 오류 스트림에 이 포크의 기존 `SplineMesh3D` 오류가 섞인다. 수집기가 이를 걸러야 한다.

## 남은 미검증

- 실제 씬 편집·저장·reload 후의 노드·리소스·transform 보존 (스펙 인수인계 6항).
- 협력적 취소와 클라이언트 연결 종료 처리 (8항).
- 렌더링 모드 촬영과 headless에서의 `RENDERING_UNAVAILABLE` (9항).
- 에디터 안에서의 동작. 이번 검증은 **에디터가 아닌 일반 실행**에서 수행했다.
  `@tool`, `EditorInterface`, `EditorUndoRedoManager`가 얽힌 경로는 별도 확인이 필요하다.

---

# 미결정 4번 — 세션 파일 위치와 token

## 결론

프로토콜이 제안했던 `.godot/agent/`는 **사용자 전용이 아니다.** 발견 파일 전체를 사용자 전용 경로로 옮긴다.

## 실측 권한

두 후보 위치에 실제로 파일을 만들고 ACL을 읽었다.

| 위치 | 소유자 외 접근 |
| --- | --- |
| `user://` → `%APPDATA%\Godot\app_userdata\<project>\` | `CodexSandboxUsers` 읽기만 |
| `C:\GithubProjects\shipAdventure\.godot` | **`BUILTIN\Users` 읽기 + `Authenticated Users` 수정** |

모두 상위 디렉터리에서 상속된 권한이다(`IsInherited = True`).
프로토콜 원안대로 `.godot/agent/`에 token을 두면 이 머신의 모든 로컬 사용자가 읽을 수 있고,
인증된 사용자는 고칠 수도 있다.

이는 `.godot/` 자체의 성질이 아니라 **프로젝트를 어디에 두었는가의 결과다.**
`%USERPROFILE%` 아래 프로젝트였다면 안전했다. 사용자가 프로젝트 위치를 자유롭게 정하므로
프로젝트 디렉터리에 기대는 설계는 보장을 만들 수 없다.

## 엔진은 권한을 고치지 못한다

`FileAccessWindows::_set_unix_permissions()`는 `ERR_UNAVAILABLE`을 반환한다
(`drivers/windows/file_access_windows.cpp:533`). 실행으로도 `2 (Unavailable)`을 확인했다.
Godot에는 Windows ACL API가 없다.

`FileAccess.set_hidden_attribute()`는 `OK`를 반환하지만 숨김 속성은 보안 통제가 아니다.

→ 보호 수단은 **경로 선택뿐이다.**

## 쓰기 권한이 더 위험하다

읽기보다 `Authenticated Users`의 `Modify`가 문제다. 발견 파일의 `port`를 공격자 서버로 바꿔 두면
CLI가 그쪽에 접속해 token을 그대로 넘긴다.

프로토콜의 *"발견 파일의 존재만 신뢰하지 않는다"* 방어는 여기서 무력하다.
검증에 쓰는 `session_id`·`project_path`도 같은 파일에서 읽으므로, 위조 파일과 위조 서버가 짝을 맞추면 통과한다.

→ token만 다른 곳에 두고 나머지를 프로젝트에 남기는 절충은 쓸 수 없다. **발견 파일 전체가 보안 자산이다.**

## token 생성은 문제없다

`Crypto.generate_random_bytes()`는 mbedTLS PSA의 `psa_generate_random()`을 호출한다
(`modules/mbedtls/crypto_mbedtls.cpp:528`). 32바이트를 생성해 호출마다 다른 값임을 확인했다.

## 남은 한계

이 머신에서는 `user://`도 완전한 사용자 전용이 아니다. `CodexSandboxUsers` 그룹에 읽기 권한이 있다.
머신 정책으로 보이며, 이 환경에 "완전한 사용자 전용 경로"는 없다는 뜻이다.
더 강한 격리가 필요하면 네이티브 Win32 ACL 설정을 C++로 구현해야 한다.
