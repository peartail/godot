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

.PARAMETER MonoGlue
  SCons 빌드 성공 후 mono-glue + assemblies를 이어서 실행합니다.

.PARAMETER Jobs
  병렬 작업 수. Windows/MSVC에서는 PDB 충돌 방지를 위해 기본 1.

.PARAMETER ExtraArgs
  SCons에 추가로 전달할 인자. 예: -ExtraArgs "module_simple_terrain_enabled=no"

.EXAMPLE
  .\scripts\build.ps1
  .\scripts\build.ps1 -Preset terrain
  .\scripts\build.ps1 -Preset editor -MonoGlue
  .\scripts\build.ps1 -Preset mono-glue
#>
[CmdletBinding()]
param(
    [ValidateSet('editor', 'terrain', 'mono-glue', 'assemblies')]
    [string] $Preset = 'editor',

    [int] $Jobs = 1,

    [switch] $MonoGlue,

    [string[]] $ExtraArgs = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

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

    $command = Get-Command scons -ErrorAction SilentlyContinue
    if ($command) {
        Write-Host "scons $($SConsArgs -join ' ')" -ForegroundColor DarkGray
        & scons @SConsArgs
    } else {
        Write-Host "python -m SCons $($SConsArgs -join ' ')" -ForegroundColor DarkGray
        & python -m SCons @SConsArgs
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
    & $GodotConsole --headless --generate-mono-glue modules\mono\glue
    if ($LASTEXITCODE -ne 0) {
        throw "Mono glue generation failed with exit code $LASTEXITCODE"
    }

    Write-Step 'Mono assemblies'
    & python modules\mono\build_scripts\build_assemblies.py --godot-output-dir bin --godot-platform windows --dev-debug
    if ($LASTEXITCODE -ne 0) {
        throw "Mono assemblies build failed with exit code $LASTEXITCODE"
    }
}

function Invoke-AssembliesOnly {
    Write-Step 'Mono assemblies'
    & python modules\mono\build_scripts\build_assemblies.py --godot-output-dir bin --godot-platform windows --dev-debug
    if ($LASTEXITCODE -ne 0) {
        throw "Mono assemblies build failed with exit code $LASTEXITCODE"
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
}

Write-Host ""
Write-Host "Build finished: $Preset" -ForegroundColor Green
