$ErrorActionPreference='Stop'
$root='C:\Users\hojun\Downloads\Ultima1K_Native_Installer'
$base='C:\Users\hojun\.cache\ultima1-build\native-installer-test'
New-Item -ItemType Directory -Force $base | Out-Null
$orig=Join-Path $base 'original'
$parent=Join-Path $base 'target'
New-Item -ItemType Directory -Force $orig,$parent | Out-Null
@('ULTIMA.EXE','GEN.EXE','OUT.EXE','MONDAIN.EXE','SPACE.EXE','MAP.BIN','EGATILES.BIN','EGATOWN.BIN','TCD.BIN') | ForEach-Object { [IO.File]::WriteAllText((Join-Path $orig $_),'FAKE-TEST-DATA') }
Add-Type -AssemblyName System.IO.Compression
foreach($edition in @('DOS','Win')){
 $exe=Join-Path $root ("Ultima1K($edition)_Setup.exe")
 $assembly=[Reflection.Assembly]::LoadFile($exe)
 $type=$assembly.GetType('Installer')
 $method=$type.GetMethod('Install',[Reflection.BindingFlags]'Static,NonPublic')
 $dst=Join-Path $parent ("Ultima1K($edition)")
 New-Item -ItemType Directory -Force (Join-Path $dst 'SAVEGAME') | Out-Null
 $sentinel=Join-Path $dst 'SAVEGAME\sentinel.test'
 [IO.File]::WriteAllText($sentinel,'SAVES-PRESERVED')
 $argsArray=New-Object 'System.Object[]' 2; $argsArray[0]=[string]$orig; $argsArray[1]=[string]$parent
 try {$out=$method.Invoke($null,$argsArray)}catch{throw $_.Exception}
 if([IO.File]::ReadAllText($sentinel) -ne 'SAVES-PRESERVED'){throw "$edition save overwritten"}
 if(-not (Test-Path (Join-Path $dst 'original_path.txt'))){throw "$edition path marker missing"}
 if($edition -eq 'DOS'){
  if(-not (Test-Path (Join-Path $dst 'ULTIHF99.EXE'))){throw 'DOS executable missing'}
  if(-not (Test-Path (Join-Path $dst 'GAMEDATA\ULTIMA.EXE'))){throw 'DOS source copy missing'}
 }else{if(-not (Test-Path (Join-Path $dst 'Ultima1KR.exe'))){throw 'Windows executable missing'}}
 Write-Output "$edition INSTALL_SMOKE_PASS $(Get-Item $exe | Select-Object -ExpandProperty Length)"
}
Get-ChildItem $root -File | ForEach-Object { $h=Get-FileHash $_.FullName -Algorithm SHA256; "$($_.Name) SHA256 $($h.Hash)" }
