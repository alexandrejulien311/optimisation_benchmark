# Builds build/benchmark.exe with clang++ (MSYS2 CLANG64), no CMake needed,
# and optionally runs it, opens the report, or checks the output against README.md.
#
# Usage (from any folder; paths are resolved from the project root):
#   powershell -ExecutionPolicy Bypass -File build.ps1           # build only (-O2)
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Run      # build, then run
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Open     # build, run, open the HTML report
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Check    # build, run, compare with README.md
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Debug    # no optimisation, for LLDB
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Static   # standalone .exe (no DLLs needed)

param(
    [switch]$Run,
    [switch]$Open,
    [switch]$Check,
    [switch]$Debug,
    [switch]$Static
)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

if (-not (Get-Command clang++ -ErrorAction SilentlyContinue)) {
    Write-Error "clang++ not found. Install MSYS2 and add C:\msys64\clang64\bin to your user PATH (see README.md)."
}

# --- Build -------------------------------------------------------------------

# Every .cpp under src/ plus main.cpp: new source files are picked up automatically.
# Sorted so the command line is identical on every machine.
$sources = @('main.cpp') + (Get-ChildItem -Path src -Recurse -Filter *.cpp |
    Sort-Object FullName | ForEach-Object { $_.FullName })

$flags = @('-std=c++20', '-g', '-Wall', '-Wextra')
if ($Debug) { $flags += '-O0' } else { $flags += '-O2' }
if ($Static) { $flags += '-static' }

New-Item -ItemType Directory -Force -Path build | Out-Null
$exe = 'build/benchmark.exe'

Write-Host (& clang++ --version | Select-Object -First 1)
Write-Host "clang++ $($flags -join ' ') ($($sources.Count) files) -o $exe"
& clang++ @flags @sources -o $exe
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed (clang++ exit code $LASTEXITCODE)."
}
Write-Host "Built $exe"

if (-not ($Run -or $Open -or $Check)) { exit 0 }

# --- Run ---------------------------------------------------------------------

Write-Host ''
$output = @(& "./$exe")
$runExit = $LASTEXITCODE
$output | ForEach-Object { Write-Host $_ }
if ($runExit -ne 0) {
    Write-Error "benchmark.exe failed (exit code $runExit)."
}

# --- Check against README.md -------------------------------------------------

if ($Check) {
    # Expected output: the README block from the "Function" header to the "Report" line.
    $readme = Get-Content README.md -Encoding UTF8
    $start = [array]::FindIndex($readme, [Predicate[string]] { param($l) $l -like 'Function*' })
    $end = [array]::FindIndex($readme, [Predicate[string]] { param($l) $l -like 'Report written*' })
    if ($start -lt 0 -or $end -lt $start) {
        Write-Error "Could not find the expected output block in README.md."
    }
    $expected = $readme[$start..$end]
    $actual = @($output | Where-Object { $_ -ne $null })
    $actualStart = [array]::FindIndex([string[]]$actual, [Predicate[string]] { param($l) $l -like 'Function*' })
    if ($actualStart -gt 0) { $actual = $actual[$actualStart..($actual.Count - 1)] }

    $mismatches = 0
    $count = [Math]::Max($expected.Count, $actual.Count)
    for ($i = 0; $i -lt $count; $i++) {
        $e = if ($i -lt $expected.Count) { $expected[$i] } else { '<missing>' }
        $a = if ($i -lt $actual.Count) { $actual[$i] } else { '<missing>' }
        if ($e.TrimEnd() -ne $a.TrimEnd()) {
            if ($mismatches -eq 0) { Write-Host '' }
            Write-Host "line $($i + 1):"
            Write-Host "  expected: $e"
            Write-Host "  actual:   $a"
            $mismatches++
        }
    }
    Write-Host ''
    if ($mismatches -gt 0) {
        Write-Host "Output differs from README.md on $mismatches line(s)." -ForegroundColor Red
        Write-Host "With a compiler other than clang/libc++ (MSYS2 CLANG64), only the last digits of tiny f - f* values may differ."
        exit 1
    }
    Write-Host 'Output matches README.md exactly.' -ForegroundColor Green
}

# --- Open the report ---------------------------------------------------------

if ($Open) {
    Invoke-Item benchmark_report.html
}
