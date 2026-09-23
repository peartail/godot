# Agent Tools

Agent-facing helpers for engine documentation and editor automation.

## Subtopics

- [Agent Docs CLI](agent_docs_cli.md)
- [Editor Automation Specification](editor_automation_spec.md): CLI/RPC 제어와 복잡한 GDScript 작업 실행 설계 초안.
- [Editor Automation Stage 0 Findings](editor_automation_stage0_findings.md): GDScript 동적 실행 기술 검증 결과와 구현 제약.
- [Editor Automation Protocol](editor_automation_protocol.md): JSON-RPC 계약. 단계 1A만 구현되어 있다.

## Code Links

- `doc/classes/`: class reference XML consumed by editor help and agent docs.
- `modules/*/doc_classes/`: module class reference XML.
- `editor/`: editor-side documentation and help integration.
- `editor/agent/`: the editor automation server and its session identity.
- `scripts/godotctl/`: the Python client and its end-to-end test.
