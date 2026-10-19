param(
 [Parameter(Mandatory=$true)][string]$StageDir,
 [Parameter(Mandatory=$true)][string]$OutputDir,
 [string]$Compiler = "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$source=Join-Path $PSScriptRoot 'InnoSetup_DOS.iss'
if(!(Test-Path $Compiler)){throw "Inno Setup compiler missing: $Compiler"}
$must=@('ULTIHF99.EXE','CWSDPMI.EXE','DOSBox\DOSBox.exe','DOSBox\SDL.dll','DOSBox\SDL_net.dll','DOSBox\COPYING.txt','DOSBox\AUTHORS.txt','FONTS\H12.FNT','FONTS\ROBO.FNT','Ultima1K.bat','dosbox_ultima1.conf')
foreach($name in $must){if(!(Test-Path (Join-Path $StageDir $name))){throw "Missing payload: $name"}}
$fontDir=Join-Path $repo 'assets\FONTS'
foreach($name in @('H12.FNT','ROBO.FNT')){
 $trusted=(Get-FileHash (Join-Path $fontDir $name) -Algorithm SHA256).Hash
 $staged=(Get-FileHash (Join-Path $StageDir "FONTS\$name") -Algorithm SHA256).Hash
 if($trusted -ne $staged){throw "Bundled font differs from repository: $name"}
}
New-Item -ItemType Directory -Force $OutputDir | Out-Null
$stageAbs=[IO.Path]::GetFullPath($StageDir)
$outAbs=[IO.Path]::GetFullPath($OutputDir)
& $Compiler "/DPayload=$stageAbs" "/DReleaseDir=$outAbs" $source
if($LASTEXITCODE -ne 0){throw "ISCC compilation failed: $LASTEXITCODE"}
$exe=Join-Path $outAbs 'Ultima1K(DOS)_Setup.exe'
Get-Item $exe | Select-Object FullName,Length
Get-FileHash $exe -Algorithm SHA256 | Select-Object Hash
