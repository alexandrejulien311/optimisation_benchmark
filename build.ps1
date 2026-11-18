# Builds build/benchmark.exe with clang++ (MSYS2 CLANG64), no CMake needed.
#
# Usage (from the project root):
#   powershell -ExecutionPolicy Bypass -File build.ps1           # optimised build (-O2)
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Debug    # no optimisation, for LLDB
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Static   # standalone .exe (no DLLs needed)

param(
    [switch]$Debug,
    [switch]$Static
)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

if (-not (Get-Command clang++ -ErrorAction SilentlyContinue)) {
    Write-Error "clang++ not found. Install MSYS2 and add C:\msys64\clang64\bin to your user PATH (see README.md)."
}

# Every .cpp under src/ plus main.cpp: new source files are picked up automatically.
$sources = @('main.cpp') + (Get-ChildItem -Path src -Recurse -Filter *.cpp | ForEach-Object { $_.FullName })

$flags = @('-std=c++20', '-g', '-Wall', '-Wextra')
if ($Debug) { $flags += '-O0' } else { $flags += '-O2' }
if ($Static) { $flags += '-static' }

New-Item -ItemType Directory -Force -Path build | Out-Null
$output = 'build/benchmark.exe'

Write-Host "clang++ $($flags -join ' ') ($($sources.Count) files) -o $output"
& clang++ @flags @sources -o $output
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed (clang++ exit code $LASTEXITCODE)."
}
Write-Host "Built $output"
