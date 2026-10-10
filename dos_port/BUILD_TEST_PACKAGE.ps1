
param([switch]$SkipBuild)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
$base=Split-Path -Parent $root
$game=Join-Path $root 'V1_0K'
$log=Join-Path $root 'BUILD_TEST_PACKAGE_LAST.log'
Start-Transcript -Path $log -Force | Out-Null
try {
 if(-not $SkipBuild){
  $compile='cd /mnt/c/Users/hojun/Downloads/Ultima1_KR_Work && tools/djgpp/djgpp/bin/i586-pc-msdosdjgpp-g++ -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -Idos_port/V1_0K -Isrc/HF93/src dos_port/V1_0K/dos_field_main.cpp dos_port/V1_0K/dos_sdl_video.cpp src/HF93/src/OverworldSimulation.cpp src/HF93/src/LocalSimulation.cpp -o dos_port/V1_0K/FIELD.EXE'
  & wsl.exe bash -lc $compile
  if($LASTEXITCODE -ne 0){throw "DJGPP compilation failed ($LASTEXITCODE)"}
 }
 $dosbox='C:\Program Files (x86)\DOSBox-0.74-3\DOSBox.exe'
 $tests=@(
  @{name='move';conf='dosbox_move_sound_validation.conf';result='MOVESND.OK';needle='MOVE SOUND PASS'},
  @{name='collision';conf='dosbox_collision_validation.conf';result='COLLISIO.OK';needle='COLLISION PASS'},
  @{name='water';conf='dosbox_water_validation.conf';result='WATER.OK';needle='WATER PASS'},
  @{name='save';conf='dosbox_save_validation.conf';result='FIELD.OK';needle='MOVES=3'}
 )
 foreach($t in $tests){
  $path=Join-Path $game $t.result
  Remove-Item $path -Force -ErrorAction SilentlyContinue
  $config=Join-Path $root $t.conf
  if(-not(Test-Path $config)){throw "Missing $config"}
  $process=Start-Process -FilePath $dosbox -ArgumentList @('-conf',('"' + $config + '"'),'-exit') -Wait -PassThru
  if($process.ExitCode -ne 0){throw "DOSBox $($t.name) failed with $($process.ExitCode)"}
  if(-not(Test-Path $path)){throw "No $($t.result)"}
  $output=Get-Content $path -Raw
  if(-not $output.Contains($t.needle)){throw "$($t.name): $output"}
  Write-Host "PASS $($t.name): $($output.Trim())"
 }
 $archive=Join-Path $root 'ULTIMA1_KR_DOS_VERIFIED_HANDOFF.zip'
 if(Test-Path $archive){Remove-Item $archive -Force}
 & python (Join-Path $root 'PACKAGE_DOS.py')
 if($LASTEXITCODE -ne 0){throw 'Python zip failed'}
 $hash=(Get-FileHash $archive -Algorithm SHA256).Hash
 Write-Host "SUCCESS package=$archive sha256=$hash"
}finally{Stop-Transcript | Out-Null}
