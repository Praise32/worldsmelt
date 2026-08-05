param(
    [string]$PythonPath = 'C:\Users\maria\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$outputDir = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build\procedural-combat-demo'))
$expectedPrefix = $projectRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $outputDir.StartsWith($expectedPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Percorso output non sicuro: $outputDir"
}

& (Join-Path $PSScriptRoot 'build.ps1')

$framesDir = Join-Path $outputDir 'frames'
if (Test-Path -LiteralPath $framesDir) {
    Remove-Item -LiteralPath $framesDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $framesDir | Out-Null

$executable = Join-Path $outputDir 'worldsmelt-procedural-combat-demo.exe'
Push-Location $outputDir
try {
    & $executable --capture $framesDir
    if ($LASTEXITCODE -ne 0) {
        throw "Cattura Raylib fallita con codice $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

$frameCount = @(Get-ChildItem -LiteralPath $framesDir -Filter 'frame-*.png' -File).Count
if ($frameCount -ne 450) {
    throw "Cattura incompleta: attesi 450 frame PNG, trovati $frameCount."
}

if (-not (Test-Path -LiteralPath $PythonPath -PathType Leaf)) {
    $pythonCommand = Get-Command python -ErrorAction SilentlyContinue
    if (-not $pythonCommand) {
        $pythonCommand = Get-Command py -ErrorAction SilentlyContinue
    }
    if (-not $pythonCommand) {
        throw 'Python con Pillow non trovato.'
    }
    $PythonPath = $pythonCommand.Source
}

& $PythonPath (Join-Path $PSScriptRoot 'make_preview.py') `
    --frames $framesDir `
    --gif (Join-Path $outputDir 'worldsmelt-procedural-combat.gif') `
    --webp (Join-Path $outputDir 'worldsmelt-procedural-combat.webp') `
    --contact-sheet (Join-Path $outputDir 'worldsmelt-procedural-combat-contact-sheet.png')
if ($LASTEXITCODE -ne 0) {
    throw "Creazione anteprime fallita con codice $LASTEXITCODE."
}

Write-Host "Cattura verificata: $frameCount frame a 15 fps (30 secondi)."
Write-Host "Anteprime pronte in: $outputDir"

