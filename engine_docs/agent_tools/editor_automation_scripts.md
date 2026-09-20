# Editor Automation — 스크립트 실행 계약

[상위 스펙](editor_automation_spec.md) · [프로토콜](editor_automation_protocol.md)

## 목적과 실행 단위

- 전체 GDScript 소스를 한 번에 보내 에디터 메인 스레드에서 실행한다.
- 첫 언어는 GDScript다. 게임 프로젝트가 C#이어도 Godot 노드·리소스 조작에 사용할 수 있다.
- C# 문자열 즉석 컴파일, 일반 Python 실행, 임의 언어 선택은 초기 범위에서 제외한다.
- 각 요청에 새 스크립트 인스턴스를 만든다. 전역 변수가 누적되는 REPL은 초기 범위에 없다.
- 별도 범용 DSL을 만들지 않는다. 반복·조건·함수·수학·await는 GDScript를 사용한다.
- 원본 source, source_name, args는 작업에 보관하고 보관량을 제한한다. 민감한 입력을 일반 로그에 출력하지 않는다.
- 경로 의존성이 있는 코드는 res:// 기준 명시적 참조를 사용한다. source_name은 진단 이름이지 로드 경로가 아니다.
- res:// 의존성은 실행 대상 프로젝트에서 해석한다. 초기 버전은 다중 파일 업로드 패키지를 제공하지 않는다.

## script.run 입력

```json
{
  "target": {"scene": "res://scenes/test.tscn", "generation": 3},
  "source_name": "place_rocks.gd",
  "language": "gdscript",
  "source": "@tool\nextends RefCounted\n\nfunc run(ctx, args: Dictionary) -> Dictionary:\n    return {\"count\": args[\"count\"]}\n",
  "args": {"count": 10}
}
```

- 전체 소스 형식: `@tool`, `extends RefCounted`, `func run(ctx, args: Dictionary) -> Dictionary`.
- run은 동기 또는 async일 수 있다. helper 함수와 내부 계산은 자유롭게 작성한다.
- 초기 계약에서 class_name 등록과 자동 실행용 _init/_static_init 부작용은 사용하지 않는다.
- 위 규칙은 저작 계약이다. 임의 코드를 완전히 정적으로 검증하는 sandbox를 의미하지 않는다.
- return은 JSON 변환 가능한 Dictionary다. 명시적인 도메인 실패는 ctx.fail로 종료한다.
- GDScript에는 일반적인 try/catch가 없으므로 예외 처리 구문을 계약에 가정하지 않는다.
- 결과 타입 오류는 RESULT_ENCODING_ERROR다. 실행 중 발생한 변경을 되돌렸다고 표시하지 않는다.

## 실행 흐름

1. 인증·입력 크기·대상·generation·함수 계약을 확인한다.
2. GDScript 리소스에 source_code를 설정하고 reload로 컴파일한다. 오류를 수집한다.
3. 인스턴스를 생성하고 전용 실행기로 run(ctx, args)을 호출한다.
4. async 대기 중 source 리소스, 인스턴스, ctx 및 continuation 수명을 유지한다.
5. return 또는 명시적 실패·실행 오류·협력적 취소에서 job을 종료한다.
6. result·logs·effects·artifacts를 직렬화하고 실행용 참조를 해제한다.

주의: 인스턴스 생성·컴파일 과정도 사용자 코드가 실행될 수 있는 경로로 취급한다.
단순 EditorScript._run은 반환값이 없고 RefCounted 수명 문제가 있어 그대로 RPC 완료 계약으로 쓰지 않는다.
현재 JSONRPC의 동기 Callable 호출만으로 await 완료가 처리된다고 가정하지 않는다.
필요하면 작은 @tool GDScript 보조 실행기가 await와 완료 신호를 담당하도록 한다.

## 최소 실행 컨텍스트 ctx

아래 이름은 제안이며 엔진 전체 API를 감싸는 객체가 아니다.

| API | 역할 |
| --- | --- |
| get_node(path) | 고정 대상 씬의 NodePath 해석, 생존·generation 확인 |
| instantiate(scene_path) | PackedScene 인스턴스 생성, 아직 트리에 넣지 않음 |
| add_node(node, parent_path) | 노드 추가, owner, Undo, 수정 상태 기록, 성공 bool 반환 |
| set_property(object, property, value) | 검증 후 Undo를 통한 속성 수정, 성공 bool 반환 |
| checkpoint() | 다음 프레임으로 양보, 취소·deadline·대상 유효성 확인 |
| capture_viewport(index, output_path) | 그리기 완료 대기와 이미지 저장, 아티팩트 반환 |
| log(message), progress(current, total) | 크기 제한이 있는 해당 job의 로그·진행 보고 |
| fail(code, message) | job을 실패로 표시, 보조 API의 후속 부작용 실행 거부 |

- ctx.fail은 GDScript 스택을 강제로 unwind하지 않는다. 호출한 스크립트는 즉시 return해야 한다.
- ctx 편집 실패는 job에 오류를 기록하고 false를 반환한다. 호출자는 실패 시 즉시 return한다.
- 취소·실패 뒤에도 직접 Godot API를 호출하는 코드를 강제 차단한다고 보장하지 않는다.
- checkpoint는 계속 가능 여부를 bool로 반환한다. false이면 스크립트는 즉시 return한다.
- ctx가 이미 실패·취소 상태이면 이후 run의 return 값으로 succeeded 상태를 덮어쓰지 않는다.
- 내부적으로 ctx 편집 기능과 단일 CLI 편집 명령은 같은 작업 함수를 사용한다.
- ctx가 만든 미부착 임시 노드는 작업 종료 시 정리한다. 트리에 반영한 변경은 자동 삭제하지 않는다.
- raw 객체 참조는 reload를 넘겨 보관하지 않는다. ctx가 해석한 객체도 사용 시 생존 확인이 필요하다.
- arbitrary API 사용 시 Undo·저장·효과 추적은 작성자 책임이다. ctx가 모든 직접 변경을 관찰하지는 않는다.
- 직접 print와 엔진 오류의 job 귀속 가능 범위는 기술 검증으로 확정한다. ctx.log 경로는 필수다.

## 복잡한 작업 예시

아래는 제안 ctx를 사용하는 미구현 예시다. points는 XZ 좌표 배열이다.

```gdscript
@tool
extends RefCounted

func run(ctx, args: Dictionary) -> Dictionary:
    var paths = []
    for point in args["points"]:
        if not await ctx.checkpoint():
            return {}
        var terrain = ctx.get_node("Terrain")
        if not is_instance_valid(terrain):
            ctx.fail("INVALID_TARGET", "Terrain node is unavailable")
            return {}
        var hit = terrain.get_brush_hit(
            Vector3(point[0], 100, point[1]), Vector3.DOWN)
        if hit.is_empty():
            continue
        var rock = ctx.instantiate(args["rock_scene"])
        if not is_instance_valid(rock):
            ctx.fail("INVALID_TARGET", "Rock scene could not be instantiated")
            return {}
        if not ctx.add_node(rock, "."):
            return {}
        if not ctx.set_property(rock, "global_position", hit["position"]):
            return {}
        paths.append(str(rock.get_path()))
    var capture = await ctx.capture_viewport(0, args["output"])
    return {"placed": paths, "capture": capture}
```

지형 높이 조회→조건부 배치→프레임 양보→촬영을 한 번의 script.run으로 수행하는 예다.
입력 검증은 실제 명령에서 추가한다. 이 예시는 자동 저장하거나 실패 시 전체 롤백하지 않는다.
지형·바위 등 특수 생성 API는 기존 ClassDB 기능을 직접 사용하고 결과 report를 검사한다.

## 실패·취소·격리의 경계

- 스크립트는 신뢰한 로컬 개발 코드다. 실행 권한은 에디터 프로세스의 파일·OS 접근 권한을 포함한다.
- 단어 차단이나 메서드 목록만으로 안전한 sandbox라고 표현하지 않는다.
- 메인 스레드의 무한 루프·블로킹 native 호출은 일반 timeout으로 안전하게 중단할 수 없다.
- 큰 반복은 checkpoint를 호출해야 한다. await한다고 CPU 작업이 자동으로 다른 스레드로 이동하지 않는다.
- 일반 실행 오류 후 이미 변경한 상태는 남을 수 있다. partial_changes 또는 unknown으로 보고한다.
- 요청 전체 원자성·자동 rollback·완전한 드라이런은 제공하지 않는다.
- 강제 종료가 필요한 오프라인 생성은 향후 별도 Godot 프로세스로 격리한다.
- 별도 프로세스는 실행 중인 에디터 객체를 직접 공유하지 않는다. 생성물 저장·검증 후 import한다.
- arbitrary script의 자동 재시도는 금지한다. 사용자 코드가 반복 실행을 고려한 경우에만 다시 실행한다.
- 씬 reload 후에는 참조를 다시 취득한다. 초기 ctx에는 reload/restart helper를 넣지 않는다.
- 에디터 종료·재시작을 스크립트 중간에 호출하면 결과 전달과 연속 실행을 보장하지 않는다.

## 구현 전 필수 기술 검증

1. 메모리 소스의 tool 인스턴스 생성과 Dictionary 반환.
2. 문법 오류의 source_name·줄·메시지 수집, 실패 시 씬 변경 없음.
3. 한 프레임 및 여러 프레임 await 뒤 정확히 한 번 완료, 대기 중 객체 생존.
4. runtime 오류가 await 전/후에 발생할 때 failed 종료와 job 귀속.
5. 큐 취소, checkpoint 취소, never-emitted signal 대기 시 deadline 처리와 참조 정리.
6. 종료한 job의 늦은 continuation이 추가 ctx 변경을 하지 않는지 검증.
7. 스크립트 일부 변경 후 실패에서 Undo·effects와 최종 상태의 일치.
8. 직접 API 부작용이 추적되지 않을 때 incomplete/unknown 보고.

필수 항목이 불가능하면 제한을 문서화하고 실행 계약을 조정한다. 로그가 없다는 이유로 성공시키지 않는다.

## 참고

- [Godot Script API](https://docs.godotengine.org/en/stable/classes/class_script.html)
- [Godot EditorScript API](https://docs.godotengine.org/en/stable/classes/class_editorscript.html)
- [Coplay Unity MCP execute_code](https://coplaydev.github.io/unity-mcp/reference/tools/scripting_ext/execute_code)

Unity 사례는 코드 문자열을 에디터 안에서 실행하는 방식의 참고다. 같은 컴파일러·격리·Undo 보장을 가정하지 않는다.
