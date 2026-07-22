#Requires -Version 5.1
<#
.SYNOPSIS
  Godot Engine 포크 빌드 스크립트 (Windows / PowerShell)

.DESCRIPTION
  이 포크의 기본 SCons 빌드 프리셋을 한 줄로 실행합니다.
  Cursor 에이전트는 긴 scons 명령 대신 이 스크립트를 사용하세요.

.PARAMETER Preset
  editor   - Mono 에디터 dev 빌드 (기본)
  terrain  - OpenWorldTerrain 모듈 포함 에디터 빌드
  mono-glue - Mono glue + assemblies 재생성 (빌드 없음)
  assemblies - C# assemblies만 재빌드
  public-api - 기존 바이너리로 agent-docs 공개 API 검증만 수행

.PARAMETER MonoGlue
  SCons 빌드 성공 후 mono-glue + assemblies를 이어서 실행합니다.

.PARAMETER PublicApi
  ClassDB/공개 API 변경 후 검증 경로. editor/terrain에서는 MonoGlue를 자동 포함하고,
  -PublicApiClasses에 지정한 클래스를 headless agent-docs로 확인합니다.
  Preset public-api 에서는 이 스위치 없이 -PublicApiClasses만으로 검증합니다.

.PARAMETER PublicApiClasses
  -PublicApi 또는 -Preset public-api 와 함께 사용. 검증할 ClassDB 클래스 이름 목록.
  예: -PublicApiClasses OpenWorldPlacement3D,OpenWorldPlacementEntry

.PARAMETER Jobs
  병렬 작업 수. Windows/MSVC에서는 PDB 충돌 방지를 위해 기본 1.

.PARAMETER ExtraArgs
  SCons에 추가로 전달할 인자. 예: -ExtraArgs "module_simple_terrain_enabled=no"

.EXAMPLE
  .\scripts\build.ps1
  .\scripts\build.ps1 -Preset terrain
  .\scripts\build.ps1 -Preset editor -MonoGlue
  .\scripts\build.ps1 -Preset mono-glue
  .\scripts\build.ps1 -Preset terrain -PublicApi -PublicApiClasses OpenWorldPlacement3D,OpenWorldPlacementEntry
  .\scripts\build.ps1 -Preset public-api -PublicApiClasses OpenWorldPlacement3D
#>
[CmdletBinding()]
param(
    [ValidateSet('editor', 'terrain', 'mono-glue', 'assemblies', 'public-api')]
    [string] $Preset = 'editor',

    [int] $Jobs = 1,

    [switch] $MonoGlue,

    [switch] $PublicApi,

    [string[]] $PublicApiClasses = @(),

    [string[]] $ExtraArgs = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# scons가 stderr에 경고(예: ANGLE/AccessKit 의존성 미설치)를 출력해도
# PowerShell 7.4+가 이를 치명적 오류로 승격시키지 않도록 한다. 종료 코드로만 판단한다.
$PSNativeCommandUseErrorActionPreference = $false

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $RepoRoot

$GodotConsole = Join-Path $RepoRoot 'bin\godot.windows.editor.dev.x86_64.mono.console.exe'

function Write-Step {
    param([string] $Message)
    Write-Host ""
    Write-Host "=== $Message ===" -ForegroundColor Cyan
}

function Invoke-SCons {
    param([string[]] $SConsArgs)

    # scons는 경고를 stderr로 출력한다. Windows PowerShell 5.1에서는 $ErrorActionPreference='Stop'과
    # 결합될 때 네이티브 명령의 stderr가 종료 오류(NativeCommandError)로 승격되어 빌드가 중단된다.
    # 이 구간에서만 'Continue'로 낮추고, 성공/실패는 종료 코드로만 판단한다.
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $command = Get-Command scons -ErrorAction SilentlyContinue
        if ($command) {
            Write-Host "scons $($SConsArgs -join ' ')" -ForegroundColor DarkGray
            & scons @SConsArgs
        } else {
            Write-Host "python -m SCons $($SConsArgs -join ' ')" -ForegroundColor DarkGray
            & python -m SCons @SConsArgs
        }
    } finally {
        $ErrorActionPreference = $previousEap
    }

    if ($LASTEXITCODE -ne 0) {
        throw "SCons failed with exit code $LASTEXITCODE"
    }
}

function Invoke-MonoGlue {
    if (-not (Test-Path $GodotConsole)) {
        throw "Godot binary not found: $GodotConsole`nRun .\scripts\build.ps1 -Preset editor first."
    }

    Write-Step 'Mono glue regeneration'
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $GodotConsole --headless --generate-mono-glue modules\mono\glue
    } finally {
        $ErrorActionPreference = $previousEap
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Mono glue generation failed with exit code $LASTEXITCODE"
    }

    Invoke-AssembliesOnly
}

function Invoke-AssembliesOnly {
    Write-Step 'Mono assemblies'
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & python modules\mono\build_scripts\build_assemblies.py --godot-output-dir bin --godot-platform windows --dev-debug
    } finally {
        $ErrorActionPreference = $previousEap
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Mono assemblies build failed with exit code $LASTEXITCODE"
    }
}

function Get-PublicApiClassList {
    $classes = @()
    foreach ($entry in $PublicApiClasses) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }
        foreach ($part in ($entry -split ',')) {
            $name = $part.Trim()
            if (-not [string]::IsNullOrWhiteSpace($name)) {
                $classes += $name
            }
        }
    }
    return ,$classes
}

function Invoke-PublicApiVerify {
    param([string[]] $Classes)

    if ($null -eq $Classes -or $Classes.Count -eq 0) {
        throw "PublicApi requires -PublicApiClasses Class1,Class2,...`nSee .cursor/skills/godot-public-api/SKILL.md"
    }

    if (-not (Test-Path $GodotConsole)) {
        throw "Godot binary not found: $GodotConsole`nRun .\scripts\build.ps1 -Preset editor (or terrain) first."
    }

    Write-Step 'Public API agent-docs verify'
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        foreach ($className in $Classes) {
            Write-Host "agent-docs-class $className" -ForegroundColor DarkGray
            $outputLines = & $GodotConsole --headless --agent-docs-class $className 2>&1
            $exitCode = $LASTEXITCODE
            $outputText = ($outputLines | Out-String)
            if ($exitCode -ne 0) {
                throw "agent-docs-class failed for '$className' with exit code $exitCode"
            }
            if ($outputText -notmatch [regex]::Escape($className)) {
                throw "agent-docs-class output did not include class '$className'. Is it in get_doc_classes() and doc_classes XML?"
            }
            Write-Host "OK: $className" -ForegroundColor Green
        }
    } finally {
        $ErrorActionPreference = $previousEap
    }
}

function Invoke-EditorBuild {
    param([hashtable] $ModuleOptions)

    $args = @(
        'platform=windows',
        'target=editor',
        'dev_build=yes',
        'module_mono_enabled=yes',
        "-j$Jobs"
    )

    foreach ($key in ($ModuleOptions.Keys | Sort-Object)) {
        $args += "$key=$($ModuleOptions[$key])"
    }

    if ($ExtraArgs.Count -gt 0) {
        $args += $ExtraArgs
    }

    Write-Step "SCons build ($Preset)"
    Invoke-SCons -SConsArgs $args
}

# Public API verification path: keep ClassDB, XML, Mono glue, and agent-docs aligned.
if ($Preset -eq 'public-api') {
    $PublicApi = $true
}
if ($PublicApi -and -not $MonoGlue -and ($Preset -eq 'editor' -or $Preset -eq 'terrain')) {
    Write-Host "PublicApi implies MonoGlue for preset '$Preset'." -ForegroundColor Yellow
    $MonoGlue = $true
}

$resolvedPublicApiClasses = Get-PublicApiClassList

switch ($Preset) {
    'editor' {
        Invoke-EditorBuild -ModuleOptions @{}
        if ($MonoGlue) {
            Invoke-MonoGlue
        }
    }
    'terrain' {
        Invoke-EditorBuild -ModuleOptions @{
            module_open_world_terrain_enabled = 'yes'
        }
        if ($MonoGlue) {
            Invoke-MonoGlue
        }
    }
    'mono-glue' {
        Invoke-MonoGlue
    }
    'assemblies' {
        Invoke-AssembliesOnly
    }
    'public-api' {
        # Verify-only: requires an existing editor binary.
    }
}

if ($PublicApi) {
    Invoke-PublicApiVerify -Classes $resolvedPublicApiClasses
}

Write-Host ""
Write-Host "Build finished: $Preset" -ForegroundColor Green
if ($PublicApi) {
    Write-Host "Public API verified: $($resolvedPublicApiClasses -join ', ')" -ForegroundColor Green
}
