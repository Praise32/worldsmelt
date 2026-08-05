param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$outputDir = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build\procedural-combat-demo'))
$expectedPrefix = $projectRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $outputDir.StartsWith($expectedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Percorso output non sicuro: $outputDir"
}

$raylibDir = Join-Path $projectRoot 'deps\raylib'
$raylibLibrary = Join-Path $raylibDir 'build-windows\raylib\libraylib.a'
$luaDir = Join-Path $projectRoot 'deps\lua-5.5.0'
$luaLibrary = Join-Path $luaDir 'src\liblua.a'
$executable = Join-Path $outputDir 'worldsmelt-procedural-combat-demo.exe'

$sources = @(
    (Join-Path $PSScriptRoot 'main.c'),
    (Join-Path $PSScriptRoot 'demo_script_api.c'),
    (Join-Path $projectRoot 'src\script\script_sandbox.c'),
    (Join-Path $projectRoot 'src\core\game_math.c')
)

$gccCommand = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gccCommand) {
    throw 'gcc non trovato. Usa la toolchain MinGW/WinLibs gia prevista dal progetto.'
}

$requiredFiles = @($sources) + @($raylibLibrary, $luaLibrary)
$missingFiles = @($requiredFiles | Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) })
if ($missingFiles.Count -gt 0) {
    throw "File richiesti mancanti:`n$($missingFiles -join "`n")"
}

$logsDir = Join-Path $outputDir 'logs'
New-Item -ItemType Directory -Force -Path $outputDir, $logsDir | Out-Null

# Le risorse vengono copiate accanto all'EXE: la demo non dipende dalla
# cartella di lavoro dalla quale viene avviata. Puliamo soltanto le tre copie
# interne all'output verificato, cosi file rinominati non restano come residui.
foreach ($resourceName in @('assets', 'scripts', 'shaders')) {
    $sourceDir = Join-Path $PSScriptRoot $resourceName
    if (-not (Test-Path -LiteralPath $sourceDir -PathType Container)) {
        throw "Cartella richiesta mancante: $sourceDir"
    }

    $targetDir = Join-Path $outputDir $resourceName
    if (Test-Path -LiteralPath $targetDir) {
        Remove-Item -LiteralPath $targetDir -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
    Get-ChildItem -LiteralPath $sourceDir -Force | Copy-Item -Destination $targetDir -Recurse -Force
}

$arguments = @(
    '-std=c99',
    '-Wall',
    '-Wextra',
    '-O2',
    '-D_DEFAULT_SOURCE',
    '-D_POSIX_C_SOURCE=200809L',
    '-DPLATFORM_DESKTOP',
    "-I$PSScriptRoot",
    "-I$(Join-Path $projectRoot 'src')",
    "-I$(Join-Path $raylibDir 'src')",
    "-I$(Join-Path $luaDir 'src')"
) + $sources + @(
    $raylibLibrary,
    $luaLibrary,
    '-lopengl32',
    '-lgdi32',
    '-lwinmm',
    '-lshell32',
    '-lm',
    '-static-libgcc',
    '-Wl,--stack,8388608',
    '-o',
    $executable
)

& $gccCommand.Source @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Build della demo fallita con codice $LASTEXITCODE."
}

Write-Host "Demo compilata: $executable"
