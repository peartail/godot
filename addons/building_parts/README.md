# Building Parts Editor

Godot 에디터에서 건물 파트 메타데이터를 쉽게 관리할 수 있는 플러그인입니다.

## 기능

- ✅ **BuildingMeta 스크립트**: 건물 파트 정보를 관리하는 베이스 클래스
- ✅ **커스텀 Inspector UI**: 메타데이터를 직접 편집할 수 있는 UI
- ✅ **3D 뷰포트 오버레이**: 파트 타입별 색상으로 시각화
- ✅ **자동 ID 생성**: 중복 없는 고유 ID 자동 생성
- ✅ **Transform 기반 계산**: Position, Dimensions, Volume 실시간 표시
- ✅ **데이터 내보내기**: JSON으로 메타데이터 내보내기

## 설치 방법

1. `addons/building_parts/` 폴더를 프로젝트에 복사
2. Godot 에디터에서 `Project > Project Settings > Plugins` 열기
3. "Building Parts Editor" 플러그인 활성화

## 사용 방법

### 1. BuildingMeta 스크립트 붙이기

```
1. 씬에 MeshInstance3D 노드 추가
2. Inspector에서 Script 섹션 클릭
3. "Load" 버튼 → `building_meta.gd` 선택
4. 또는 "New Script" → "Inherits: BuildingMeta" 선택
```

### 2. 메타데이터 설정

Inspector에서 다음 항목들을 설정할 수 있습니다:

**Building Part Info**
- Part Name: 사람이 읽을 수 있는 이름
- Part Type: Floor, Wall, Ceiling, Door, Window, Roof, Column, Beam, Stair

**Semantic Info**
- Floor Level: 층 번호
- Grid Position: 그리드 좌표
- Is Structural: 구조적 요소 여부

**Architectural Properties**
- Material Type: 재료 타입
- Finish Type: 마감 타입
- Fire Rating: 내화 등급

### 3. 커스텀 UI 기능

BuildingMeta가 붙은 노드를 선택하면 Inspector 하단에 추가 UI가 나타납니다:

- **ID 표시**: 자동 생성된 고유 ID
- **Regenerate 버튼**: 새로운 ID 생성
- **Transform 정보**: Position, Center, Dimensions, Volume 실시간 표시
- **Copy Data JSON**: 메타데이터를 JSON으로 클립보드에 복사
- **Export to File**: 메타데이터를 파일로 저장 (`exports/` 폴더)

### 4. 3D 뷰포트 시각화

3D 뷰에서 BuildingMeta가 붙은 메시는:
- 파트 타입에 따라 **색상별 바운딩 박스** 표시
- 중심점에 **십자 표시**
- 구조적 요소는 **대각선 표시**

#### 색상 코드
- 🟤 Floor (갈색)
- 🔵 Wall (파란색)
- 🔵 Ceiling (하늘색)
- 🟢 Door (초록색)
- 🔵 Window (청록색)
- 🔴 Roof (빨간색)
- 🟠 Column (주황색)
- 🟡 Beam (노란색)
- 🟣 Stair (보라색)

## 코드 예시

### GDScript에서 메타데이터 접근

```gdscript
extends Node3D

func _ready():
    var wall = $Wall as BuildingMeta
    
    # 메타데이터 가져오기
    var data = wall.get_part_data()
    print("Part ID: ", data["id"])
    print("Part Type: ", data["part_type"])
    
    # Transform 기반 정보
    var dims = wall.get_dimensions()
    print("Size: ", dims)
    
    var center = wall.get_center_position()
    print("Center: ", center)
```

### 프로그래밍 방식으로 BuildingMeta 생성

```gdscript
var mesh_instance = MeshInstance3D.new()
mesh_instance.mesh = BoxMesh.new()
mesh_instance.set_script(preload("res://addons/building_parts/building_meta.gd"))

# 속성 설정
mesh_instance.part_name = "Living Room Wall"
mesh_instance.part_type = "Wall"
mesh_instance.floor_level = 1
mesh_instance.is_structural = true

add_child(mesh_instance)
```

## 파일 구조

```
addons/building_parts/
├── plugin.cfg                      # 플러그인 설정
├── plugin.gd                       # 메인 플러그인 스크립트
├── building_meta.gd                # BuildingMeta 베이스 클래스
├── inspector_plugin.gd             # Inspector 커스텀 UI 등록
├── metadata_editor_control.gd      # Inspector UI 컨트롤
├── gizmo_plugin.gd                 # 3D 뷰포트 오버레이
└── README.md                       # 이 파일
```

## 라이센스

MIT License

## 기여

버그 리포트와 기능 제안은 언제나 환영합니다!
