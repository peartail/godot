# SimpleTerrain Build And Integration

## Module

현재 심플 터레인 구현은 엔진 모듈이다.

위치:

```text
modules/simple_terrain
```

등록 클래스:

- `SimpleTerrainData`
- `SimpleTerrain3D`
- `SimpleTerrainEditorPlugin`

## Build

```powershell
scons platform=windows target=editor dev_build=yes tools=yes module_mono_enabled=yes -j4
```

## Disable Build

모듈 disable 옵션:

```powershell
scons platform=windows target=editor dev_build=yes tools=yes module_simple_terrain_enabled=no -j4
```

## Mono / C#

ClassDB method/property 변경 후:

```powershell
.\bin\godot.windows.editor.dev.x86_64.mono.console.exe --headless --generate-mono-glue modules\mono\glue
python modules\mono\build_scripts\build_assemblies.py --godot-output-dir bin --godot-platform windows --dev-debug
```

rename 이후 stale wrapper가 남으면 제거한다.

예:

- `Terrain3D.cs`
- `TerrainData.cs`

현재 wrapper:

- `SimpleTerrain3D.cs`
- `SimpleTerrainData.cs`

## Doc XML

위치:

```text
modules/simple_terrain/doc_classes/SimpleTerrain3D.xml
modules/simple_terrain/doc_classes/SimpleTerrainData.xml
```

`config.py`:

```python
def get_doc_classes():
    return ["SimpleTerrain3D", "SimpleTerrainData"]

def get_doc_path():
    return "doc_classes"
```

## Registration

runtime class:

```text
MODULE_INITIALIZATION_LEVEL_SCENE
```

editor plugin:

```text
MODULE_INITIALIZATION_LEVEL_EDITOR
```

## OpenWorldTerrain 계획

`OpenWorldTerrain`은 별도 모듈로 추가한다.

목표:

- height texture.
- GPU displacement.
- tile/clipmap 실험.
- SimpleTerrain과 독립 유지.
