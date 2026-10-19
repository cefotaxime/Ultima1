$ErrorActionPreference='Stop'
$exe='C:\Users\hojun\Downloads\Ultima1K_Native_Installer_Minimized\Ultima1K(Win)_Setup.exe'
$orig='C:\Users\hojun\.cache\ultima1-build\native-installer-test\original'
$parent='C:\Users\hojun\.cache\ultima1-build\native-installer-minimized-test'
$target=Join-Path $parent 'Ultima1K(Win)'
New-Item -ItemType Directory -Force (Join-Path $target 'SAVEGAME') | Out-Null
[IO.File]::WriteAllText((Join-Path $target 'SAVEGAME\sentinel.test'),'KEEP-ME')
$a=[Reflection.Assembly]::LoadFile($exe)
$m=$a.GetType('Installer').GetMethod('Install',[Reflection.BindingFlags]'Static,NonPublic')
$argsArray=New-Object 'System.Object[]' 2
$argsArray[0]=[string]$orig;$argsArray[1]=[string]$parent
$m.Invoke($null,$argsArray) | Out-Null
if((Get-Content (Join-Path $target 'SAVEGAME\sentinel.test') -Raw) -ne 'KEEP-ME'){throw 'Save changed'}
$src='C:\Users\hojun\.cache\ultima1-build\legacy-installer-payload'
foreach($file in @('Ultima1KR.exe','SDL2.dll','libstdc++-6.dll','libgcc_s_seh-1.dll','libwinpthread-1.dll','FONTS\H12.FNT','FONTS\ROBO.FNT')){
 if(-not(Test-Path (Join-Path $target $file))){throw "Missing $file"}
}
Write-Output "INSTALL_PASS"
Write-Output "SAVES_PASS"
Get-FileHash $exe -Algorithm SHA256 | Select-Object Path,Hash
