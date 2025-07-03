# PowerShell test runner for ParZip C project
# Usage: Open a PowerShell prompt, `cd` into this directory, and run `.
run_tests.ps1`
$ErrorActionPreference = 'Stop'

Write-Host "🏁 ParZip C Tests (PowerShell) 🏁" -ForegroundColor Cyan

# Paths
$srcDir = "..\src"

# Unit Tests
Write-Host "🔧 Compiling and executing unit tests..." -ForegroundColor Yellow

Write-Host "  - utils"
& gcc -Wall -Wextra -std=c99 -O2 -pthread -I $srcDir "$srcDir\utils.c" "test_utils.c" -o "test_utils.exe"
Write-Host "  Running test_utils.exe" -NoNewline; .\test_utils.exe; Write-Host "  ✔ Passed"

Write-Host "  - compressor"
& gcc -Wall -Wextra -std=c99 -O2 -pthread -I $srcDir "$srcDir\compressor.c" "test_compressor.c" -o "test_compressor.exe" -lz
Write-Host "  Running test_compressor.exe" -NoNewline; .\test_compressor.exe; Write-Host "  ✔ Passed"

# Build ParZip
Write-Host "🛠️  Building ParZip binary..." -ForegroundColor Yellow
Push-Location ..
& make
Pop-Location

# Integration Test
Write-Host "⚙️  Running integration test..." -ForegroundColor Yellow
$sampleIn = "sample.txt"
$samplePz = "sample.txt.pz"
$sampleOut = "sample.txt.out"
"Este es un texto de prueba para ParZip." | Out-File -Encoding utf8 $sampleIn

Write-Host "  👉 Compressing $sampleIn -> $samplePz"
& "..\parzip.exe" -t 2 -b 1024 -l 6 $sampleIn $samplePz

Write-Host "  👉 Decompressing $samplePz -> $sampleOut"
& "..\parzip.exe" -d -t 2 $samplePz $sampleOut

Write-Host "  🔍 Verifying integrity..."
& fc /B $sampleIn $sampleOut | Out-Null
if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✅ Integration passed: files identical" -ForegroundColor Green
    Remove-Item $sampleIn, $samplePz, $sampleOut
} else {
    Write-Error "  ❌ Integration failed: files differ"
    exit 1
}

Write-Host "🎉 All tests passed successfully!" -ForegroundColor Green
