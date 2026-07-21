# OpenWorld Rock 샘플 프로젝트 제작 가이드

## 목적

커스텀 Godot 엔진의 Rock API로 BOULDER, SLAB, SHARD를 생성하고 LOD, collision,
Bake와 재로드를 GUI 의존 없이 검증하는 샘플 프로젝트를 만든다.

## 전제 조건

- 이 저장소에서 빌드한 Mono editor를 사용한다.
- 게임 프로젝트의 `AGENTS.md`에 다음 명령을 기록한다.

```powershell
C:\GithubProjects\godot\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-search OpenWorldRock
C:\GithubProjects\godot\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --agent-docs-class OpenWorldRockGenerator3D
```

- 샘플 제작과 검증은 GDScript 또는 C#과 headless 명령으로 재현 가능해야 한다.
- Inspector와 viewport는 결과 확인에만 사용하고 필수 제작 단계로 두지 않는다.

## 권장 폴더

```text
res://rock_sample/
  profiles/
  requests/
  baked/
  scenes/
  scripts/
  reports/
```

`baked/`와 `reports/`는 생성 결과다. `profiles/`, `requests/`, `scripts/`가 원본이다.

## 시작 자료

엔진 저장소의 다음 파일을 프로젝트로 복사하거나 같은 내용으로 다시 작성한다.

- `codex_docs/open_world_rock_boulder_request.tres`
- `codex_docs/open_world_rock_slab_request.tres`
- `codex_docs/open_world_rock_shard_request.tres`
- `codex_docs/open_world_rock_headless_sample.gd`
- 필요하면 `codex_docs/OpenWorldRockHeadlessSample.cs`

`.tres`의 sub-resource Profile을 별도 profile 파일로 분리해도 된다.

## 필수 장면

`res://rock_sample/scenes/rock_gallery.tscn`을 코드 또는 텍스트로 작성한다.

```text
RockGallery (Node3D)
  Environment
  Ground
  Boulder (OpenWorldRockGenerator3D)
  Slab (OpenWorldRockGenerator3D)
  Shard (OpenWorldRockGenerator3D)
  Camera3D
  DirectionalLight3D
```

- 세 Generator의 X 위치를 각각 `-4`, `0`, `4` 정도로 둔다.
- 각 Generator는 대응하는 request `.tres`를 참조한다.
- `auto_generate`는 장면 미리보기에서는 켜도 되지만 자동 테스트에서는 끈다.
- Ground의 표면은 로컬 Y=0에 둬 base inset을 비교할 수 있게 한다.
- 카메라와 조명 값도 `.tscn`에 숫자로 기록한다.

## 생성 스크립트

`res://rock_sample/scripts/generate_rock_samples.gd`를 `SceneTree` 스크립트로 만든다.

스크립트는 다음 순서를 반드시 수행한다.

1. 세 request `.tres`를 `ResourceLoader.load()`로 읽는다.
2. mode와 stable ID가 예상값인지 확인한다.
3. 각 request를 새 `OpenWorldRockGenerator3D`에 지정한다.
4. `validate_request()`를 호출하고 실패하면 JSON을 출력한 뒤 종료한다.
5. `generate_topology()`를 호출해 source point, hull index와 hash를 기록한다.
6. `generate_rock()`을 호출해 LOD와 collision point cloud를 생성한다.
7. `get_generation_report()`를 JSON 파일로 저장한다.
8. `create_baked_variant()`를 `baked/`에 `.tres`로 저장한다.
9. `CACHE_MODE_IGNORE`로 재로드하고 필수 속성을 비교한다.
10. 모든 항목이 통과하면 종료 코드 0, 실패하면 0이 아닌 코드를 반환한다.

권장 출력 이름:

```text
baked/boulder_8801.tres
baked/slab_8802.tres
baked/shard_8803.tres
reports/boulder_8801.json
reports/slab_8802.json
reports/shard_8803.json
```

## Headless 실행

```powershell
C:\GithubProjects\godot\bin\godot.windows.editor.dev.x86_64.mono.console.exe `
  --headless --path <PROJECT_PATH> `
  --script res://rock_sample/scripts/generate_rock_samples.gd
```

별도 검증 스크립트가 있다면 생성과 분리한다.

```powershell
C:\GithubProjects\godot\bin\godot.windows.editor.dev.x86_64.mono.console.exe `
  --headless --path <PROJECT_PATH> `
  --script res://rock_sample/scripts/validate_rock_samples.gd
```

## Gallery에서 표시할 정보

각 바위 옆 Label3D 또는 report UI에 다음을 표시한다.

- mode, seed와 stable ID
- topology hash
- LOD0/1/2 triangle 수
- base plane과 contact point 수
- collision point 수
- 현재 preview LOD

UI는 report를 읽어 표시하고 별도 계산으로 결과를 바꾸지 않는다.

## 선택적 LOD 데모

- 키 입력 또는 타이머 대신 숫자 인자로 preview LOD를 지정하는 API를 제공한다.
- `--lod 0`, `--lod 1`, `--lod 2` 같은 headless 인자를 처리해도 된다.
- 세 LOD의 pivot과 base plane이 같은지 report로 먼저 검사한다.
- 카메라 이동은 시각 확인용이며 테스트의 유일한 판정 수단이 아니어야 한다.

## 선택적 Gameplay 데모

- intact Variant에 damaged와 depleted Variant를 연결한다.
- `get_variant_for_state()` 결과로 mesh를 교체한다.
- runtime fracture나 voxel 파괴는 구현하지 않는다.
- stable ID와 state를 텍스트 save data로 기록한다.

## 완료 기준

- 세 mode가 텍스트 request만으로 생성된다.
- 같은 seed를 두 번 실행했을 때 topology hash와 배열이 같다.
- LOD triangle 수가 엄격히 감소한다.
- collision point cloud와, PhysicsServer 사용 시 collision shape가 유효하다.
- Bake 저장과 재로드 후 mesh, mode, seed, bounds와 metadata가 유지된다.
- gallery 장면이 커스텀 editor에서 열리고 세 형태가 구분된다.
- 전체 제작과 검증을 터미널 명령만으로 다시 실행할 수 있다.

세부 판정은 `open_world_rock_test_cases.md`를 따른다.
