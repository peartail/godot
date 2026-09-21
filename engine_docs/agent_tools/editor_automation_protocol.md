# Editor Automation — 프로토콜 초안

[상위 스펙](editor_automation_spec.md) · [스크립트 계약](editor_automation_scripts.md)

이 문서의 명령·필드는 구현을 위한 제안 계약이며 현재 사용 가능한 CLI가 아니다.

## 연결과 발견

- 에디터 시작 옵션 가칭: `--agent-server`. 기본 비활성화, localhost에만 bind한다.
- 초기 전송은 TCP 위 UTF-8 JSON-RPC 2.0이다. 각 메시지는 한 줄 JSON으로 framing한다.
- JSON 문자열 내부 줄바꿈은 escape한다. TCP 패킷 경계를 메시지 경계로 가정하지 않는다.
- 최대 메시지 크기·작업 수·보관 크기를 제한하고 초과 시 명시적 오류를 반환한다.
- 크기 제한의 초기 숫자는 구현 검증에서 정하고 capabilities에 노출한다. 전체 메시지를 무제한 적재하지 않는다.
- 일반 로그는 프로토콜 스트림에 섞지 않는다. CLI stdout은 JSON, stderr는 진단용이다.
- 세션 발견 파일은 **사용자 전용 경로**에 둔다. 프로젝트 디렉터리에는 두지 않는다.
  Windows에서는 `user://`가 해석되는 사용자 프로필 아래이며, 프로젝트별로 파일을 구분한다.
  `.godot/agent/`에는 세션 파일을 두지 않는다. 같은 디렉터리의 `outputs/`는 비밀이 아닌
  프로젝트 산출물이므로 그대로 둔다. [검증 근거](editor_automation_stage0_findings.md)
- 정보: protocol_version, session_id, project_path, pid, port, 시작 시각 및 인증 연결 정보.
- token은 세션마다 새로 만들고 Git·일반 로그에 남기지 않는다. 생성은 `Crypto.generate_random_bytes`를 쓴다.
- 발견 파일 전체가 보안 자산이다. token만 분리하고 나머지를 프로젝트에 두는 절충은 쓰지 않는다.
  port를 고쳐 쓸 수 있는 위치에 두면 CLI가 위조된 서버에 접속해 token을 그대로 넘긴다.
- 엔진은 Windows 파일 권한을 설정하지 못한다. 보호는 **경로 선택으로만** 달성한다.
  더 강한 격리가 필요하면 네이티브 ACL 설정을 별도로 구현한다.
- 첫 요청 `session.authenticate`가 token을 검증한다. 인증 전 다른 요청은 거부한다.
- 발견 파일의 존재만 신뢰하지 않는다. 연결 후 session_id·project_path·protocol_version을 검증한다.
  단 이 값들도 같은 파일에서 오므로, 파일을 고칠 수 있는 공격자에게는 방어가 되지 않는다.
- 같은 프로젝트에 여러 에디터가 있으면 `--session`으로 선택한다. 임의의 첫 프로세스를 고르지 않는다.
- 종료한 세션의 파일은 유효한 세션으로 취급하지 않는다. 재시작하면 새 session_id를 발급한다.
- 초기 버전은 원격 네트워크 서비스와 MCP 표준 transport를 제공하지 않는다.

## 요청과 완료

- 모든 실행 요청에 JSON-RPC id를 요구한다. 부작용 작업은 notification으로 실행하지 않는다.
- 조회는 즉시 결과를 반환한다. 편집·reload·촬영·스크립트·batch는 job을 접수하고 job_id를 반환한다.
- JSON-RPC id는 한 요청의 응답 연결용이다. job_id는 실제 작업의 수명·결과 조회용이다.
- 초기 CLI는 job.get을 polling한다. 진행 이벤트 push는 필수가 아니다.
- 연결 종료는 작업 취소가 아니다. 완료 결과는 제한된 기간/용량 동안 같은 세션에 보관한다.
- CLI는 응답을 잃은 부작용 요청을 자동 재전송하지 않는다.
- 접수 여부를 모르면 상태를 `unknown`으로 표시하고 scene/status로 확인한다. exactly-once를 보장하지 않는다.

```json
{
  "jsonrpc": "2.0",
  "id": 7,
  "method": "scene.reload",
  "params": {
    "target": {
      "scene": "res://scenes/test.tscn",
      "generation": 3
    },
    "if_dirty": "error"
  }
}
```

```json
{"jsonrpc":"2.0","id":7,"result":{"job_id":"j-12","state":"queued"}}
```

`job.get` 최종 결과의 예:

```json
{
  "job_id": "j-12",
  "state": "succeeded",
  "result": {"scene": "res://scenes/test.tscn", "generation": 4},
  "error": null,
  "effects": {"tracking": "managed", "partial_changes": false},
  "artifacts": [],
  "warnings": [],
  "logs": [],
  "logs_truncated": false
}
```

## job 상태와 스케줄링

```text
queued → running → succeeded / failed / cancelled
```

- await 중에도 running이다. `phase`와 선택적 progress로 대기 원인을 표시한다.
- 초기 버전은 한 번에 하나의 실행 job을 처리한다. await 중에도 다른 편집 job을 끼워 넣지 않는다.
- 단, 가능한 프레임마다 상태 조회·취소 요청을 처리한다. import의 중첩 main loop에도 재진입하지 않는다.
- GUI 사용은 정지하지 않는다. 대상 노드 생존과 scene generation을 변경 직전에 확인한다.
- 모든 동시 GUI 편집 충돌을 막는 전역 트랜잭션은 제공하지 않는다.
- 큐에 있을 때 취소하면 cancelled. 실행 중 취소는 플래그를 설정하고 다음 협력 지점에서 종료한다.
- `cancel_requested`와 실제 `cancelled`를 구분한다. 취소한 작업의 이미 적용된 변경은 유지될 수 있다.
- CLI `--wait-timeout`은 클라이언트 대기 제한이다. 초과하면 job_id를 반환하며 작업은 계속될 수 있다.
- 서버 작업 deadline도 협력적으로 검사한다. 무한 루프를 강제 중단하는 보장이 아니다.
- 세션 종료 시 메모리의 작업 정보는 소멸한다. 재시작을 넘는 실행 이력 복구는 초기 범위에서 제외한다.

## 대상과 값 직렬화

- 연결로 session을 고정하고, 요청으로 scene 경로와 generation을 고정한다.
- scene.tree는 현재 generation과 씬 루트 기준 NodePath를 반환한다.
- NodePath는 reload 후 다시 해석한다. ObjectID나 원시 포인터를 영속 식별자로 사용하지 않는다.
- 초기 버전은 저장 경로가 있는 열린 씬을 대상으로 한다. 새 untitled 씬 대상 식별은 후속 확장이다.
- JSON 기본 타입은 그대로 사용한다. 타입 변환은 속성 정보와 검증을 거친다.
- Variant는 명시적 태그를 사용한다: `{"$type":"Vector3","value":[0,1,0]}`.
- 필요 타입: Vector2/3, Color, Transform3D, NodePath, Resource 경로 참조, packed array.
- Node 참조는 대상 씬의 NodePath로 표현한다. RID·Callable·실행 상태를 JSON으로 직렬화하지 않는다.
- 일반 JSON 배열을 임의로 Vector3로 추정하지 않는다. enum 이름과 정수는 해당 enum 기준으로 검증한다.
- 지원되지 않는 값은 `UNSUPPORTED_VALUE_TYPE`으로 반환한다. 임의 문자열로 바꾸어 성공시키지 않는다.
- 큰 메시·이미지는 파일로 저장하고 경로·타입·크기를 반환한다. 무제한 base64를 응답에 넣지 않는다.
- 출력 상대 경로는 명시적 output root를 기준으로 해석한다. 기본 위치는 `.godot/agent/outputs/`다.
- 표준 명령과 ctx는 기존 파일을 기본 덮어쓰지 않는다. overwrite가 명시된 경우만 허용한다.
- 임의 스크립트의 직접 FileAccess 호출까지 위 경로 규칙으로 제한하는 sandbox는 제공하지 않는다.

## 오류와 부분 변경

- JSON-RPC envelope 오류는 표준 error.code를 사용한다.
- 접수 전 검증 실패는 JSON-RPC error.data에 안정적인 도메인 code와 상세를 넣는다.
- 접수 후 실패는 job.state=failed이며 job.error에 code, message, 선택적 details를 넣는다.
- 초기 도메인 code: AUTH_FAILED, SESSION_MISMATCH, UNSUPPORTED_VERSION, UNSUPPORTED_CAPABILITY,
  INVALID_TARGET, STALE_TARGET, INVALID_PARAMS, UNSUPPORTED_VALUE_TYPE, SCENE_DIRTY, SAVE_FAILED,
  SCRIPT_PARSE_ERROR, SCRIPT_RUNTIME_ERROR, RESULT_ENCODING_ERROR, RENDERING_UNAVAILABLE, OUTPUT_EXISTS.
- `SCRIPT_PARSE_ERROR`는 **문법 오류를 첫 하나만** 보고한다. 엔진이 파싱 단계에서 첫 오류만 출력하기 때문이다.
  타입 오류 등 분석 단계 오류는 전부 보고한다. 이 비대칭을 응답과 사용자 문서에 드러낸다.
- 취소는 cancelled 상태, 서버의 협력적 deadline 종료는 failed와 DEADLINE_EXCEEDED로 구분한다.
- 실행 전 오류와 일부 변경 후 오류를 구분한다. effects.partial_changes는 true/false/unknown이다.
- effects.tracking은 managed 또는 incomplete다. 임의 코드 직접 부작용은 incomplete일 수 있다.
- 전체 rollback을 보장하지 않는다. 성공 전에 이미 생성된 노드·파일 목록을 가능한 범위에서 보고한다.
- 생성 메서드가 void를 반환했다는 이유로 성공 판정하지 않는다. 도메인 report·신호·상태를 확인한다.
- 실행 오류의 파일/소스 이름, 줄 번호, 메시지를 보존하되 다른 job의 로그를 잘못 귀속하지 않는다.

## batch.run

- `commands` 배열을 받아 나열된 순서대로 각 작업의 완료를 기다린다.
- 실패하면 후속 작업을 실행하지 않고 완료한 항목과 실패 항목을 반환한다.
- 하나의 최상위 job으로 실행하고 내부 단계가 다시 전역 큐를 기다리는 교착을 만들지 않는다.
- 초기 batch는 일반 저작 명령만 포함한다. 중첩 batch·script.run·session/job 제어는 거부한다.
- 자동 rollback과 원자성을 약속하지 않는다. JSON-RPC batch 배열의 의미와 별개다.
- 변수·조건·반복·앞 단계 결과 참조가 필요하면 script.run을 사용한다.

## CLI 계약

```powershell
godotctl --project <project> --session <id> status
godotctl --project <project> scene reload --path res://scenes/test.tscn --if-dirty error --wait
godotctl --project <project> script run --file .\place_rocks.gd --args-file .\placement.json --wait
godotctl --project <project> script run --stdin --args-file .\placement.json --wait
godotctl --project <project> job get --id j-12
```

- 명령줄 예시는 가칭이다. 긴 소스는 file/stdin, 구조화된 입력은 JSON 파일을 권장한다.
- --file은 CLI가 읽어 source로 전송한다. 에디터가 클라이언트의 임의 파일 경로를 열지 않는다.
- 종료 코드 0: 접수 또는 --wait 성공. 비동기 접수 응답에는 state=queued/running을 명시한다.
- 종료 코드 제안: 1 작업 실패/취소, 2 입력 오류, 3 연결/인증/버전 오류, 4 대기 시간 초과/결과 불명.
- --wait에서 접수만으로 exit 0을 반환하지 않는다. 상태·job_id는 오류 때도 가능한 한 출력한다.
