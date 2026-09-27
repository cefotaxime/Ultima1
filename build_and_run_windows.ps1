param(
    [Parameter(Mandatory=$true)][string]$UltimaZip,
    [Parameter(Mandatory=$true)][string]$H12,
    [Parameter(Mandatory=$true)][string]$Robo,
    [switch]$Trace
)
$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Here

function Require-File([string]$Path,[string]$Label) {
    $p = (Resolve-Path $Path).Path
    if (-not (Test-Path $p -PathType Leaf)) { throw "$Label not found: $Path" }
    return $p
}
$UltimaZip = Require-File $UltimaZip 'Ultima I ZIP'
$H12 = Require-File $H12 'H12.FNT'
$Robo = Require-File $Robo 'ROBO.FNT'
if ((Get-Item $H12).Length -ne 11520) { throw 'H12.FNT size must be 11520 bytes' }
if ((Get-Item $Robo).Length -ne 4096) { throw 'ROBO.FNT size must be 4096 bytes' }

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) { $python = Get-Command py -ErrorAction SilentlyContinue }
if (-not $python) { throw 'Python 3 is required.' }
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) { throw 'CMake is required.' }

$vcpkgRoot = $env:VCPKG_ROOT
if (-not $vcpkgRoot) {
    $candidate = Join-Path $Here '.deps\vcpkg'
    if (Test-Path (Join-Path $candidate 'scripts\buildsystems\vcpkg.cmake')) { $vcpkgRoot = $candidate }
}
if (-not $vcpkgRoot) {
    throw @'
VCPKG_ROOT is not set and .deps\vcpkg was not found.
Install vcpkg once, then set VCPKG_ROOT. Example:
  git clone https://github.com/microsoft/vcpkg C:\vcpkg
  C:\vcpkg\bootstrap-vcpkg.bat
  $env:VCPKG_ROOT='C:\vcpkg'
Then rerun this script.
'@
}
$toolchain = Join-Path $vcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
if (-not (Test-Path $toolchain)) { throw "vcpkg toolchain not found: $toolchain" }

Write-Host '[1/4] Preparing original Ultima I data'
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue (Join-Path $Here 'gamedata'),(Join-Path $Here 'fonts')
New-Item -ItemType Directory -Force (Join-Path $Here 'gamedata'),(Join-Path $Here 'fonts') | Out-Null
if ($python.Name -eq 'py.exe' -or $python.Name -eq 'py') {
    & $python.Source -3 (Join-Path $Here 'tools\prepare_from_zip.py') $UltimaZip (Join-Path $Here 'gamedata')
} else {
    & $python.Source (Join-Path $Here 'tools\prepare_from_zip.py') $UltimaZip (Join-Path $Here 'gamedata')
}
Copy-Item $H12 (Join-Path $Here 'fonts\H12.FNT')
Copy-Item $Robo (Join-Path $Here 'fonts\ROBO.FNT')

Write-Host '[2/4] Configuring native Windows build through vcpkg SDL2'
$build = Join-Path $Here 'build-windows'
cmake -S $Here -B $build -A x64 "-DCMAKE_TOOLCHAIN_FILE=$toolchain" "-DVCPKG_TARGET_TRIPLET=x64-windows"

Write-Host '[3/4] Building u1kr_clean'
cmake --build $build --config Release --target u1kr_clean
$exe = Join-Path $build 'Release\u1kr_clean.exe'
if (-not (Test-Path $exe)) { $exe = Join-Path $build 'u1kr_clean.exe' }
if (-not (Test-Path $exe)) { throw 'u1kr_clean.exe was not produced.' }

Write-Host '[4/4] Starting native Windows SDL2/WASAPI comparison build'
$env:SDL_AUDIODRIVER = 'wasapi'
if ($Trace) { $env:U1_SFX_TRACE = '1' } else { Remove-Item Env:U1_SFX_TRACE -ErrorAction SilentlyContinue }
Write-Host 'Expected audio driver: wasapi'
Write-Host "Executable: $exe"
& $exe
