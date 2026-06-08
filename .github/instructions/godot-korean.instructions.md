---
description: "Godot 엔진 프로젝트 작업 시 사용. 한글로 답변하고, C++, GDScript, 게임 엔진 아키텍처, 렌더링 시스템, SCons 빌드 시스템 관련 컨텍스트를 제공합니다."
name: "Godot 한글 지침"
---

# Godot 엔진 프로젝트 지침

이 워크스페이스는 **Godot 게임 엔진**의 핵심 코드베이스입니다.

## 프로젝트 컨텍스트

- **언어**: 주로 C++ (일부 Python 빌드 스크립트)
- **빌드 시스템**: SCons
- **주요 구성요소**:
  - `core/`: 핵심 시스템 (Object, Variant, String 등)
  - `scene/`: 노드와 장면 시스템
  - `servers/`: 렌더링, 물리, 오디오 서버
  - `editor/`: Godot 에디터
  - `modules/`: 확장 모듈
  - `platform/`: 플랫폼별 구현
  - `drivers/`: 그래픽 드라이버 (GLES3, Vulkan, Metal 등)

## 답변 규칙

- **모든 설명과 답변은 한글로 작성**
- 코드 자체는 영문 그대로 유지하되, 주석과 설명은 한글로 제공
- 기술 용어는 한글 번역과 함께 영문을 괄호 안에 병기 (예: "변형 타입(Variant)")

## 일반적인 질문 패턴

- 클래스나 함수 찾기: `core/`, `scene/`, `servers/` 디렉토리 중심으로 탐색
- 빌드 관련: `SConstruct`, `*.py` 빌드 스크립트 참조
- 에디터 기능: `editor/` 디렉토리 확인
- 플랫폼별 코드: `platform/windows/`, `platform/linux/` 등 확인

## 코드 스타일

Godot 프로젝트의 기존 코딩 컨벤션을 따릅니다:
- 탭 대신 탭 사용 (프로젝트 설정 확인)
- 클래스명: PascalCase
- 메서드명: snake_case
- 상수: UPPER_SNAKE_CASE
