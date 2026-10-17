$ErrorActionPreference='Stop'
$root='C:\Users\hojun\Ultima1'
$src="$root\installer\dos"
$stage='C:\Users\hojun\.cache\ultima1-build\dos-installer-stage'
$out='C:\Users\hojun\Downloads\Ultima1K_DOS_Installer'
$runtime='C:\Users\hojun\.cache\ultima1-build\dos-runtime'
$dosbox='C:\Program Files (x86)\DOSBox-0.74-3'
if(Test-Path $stage){Remove-Item $stage -Recurse -Force}
New-Item -ItemType Directory -Force "$stage\DOSBox","$stage\FONTS",$out | Out-Null
Copy-Item "$runtime\ULTIHF99.EXE","$runtime\CWSDPMI.EXE" $stage -Force
Copy-Item "$root\assets\FONTS\H12.FNT","$root\assets\FONTS\ROBO.FNT" "$stage\FONTS" -Force
Copy-Item "$dosbox\DOSBox.exe","$dosbox\SDL.dll","$dosbox\SDL_net.dll" "$stage\DOSBox" -Force
Copy-Item "$dosbox\Documentation\COPYING.txt","$dosbox\Documentation\AUTHORS.txt" "$stage\DOSBox" -Force
Copy-Item "$dosbox\DOSBox 0.74-3 Manual.txt" "$stage\DOSBox" -Force
Copy-Item "$src\Ultima1K.bat" "$stage\Ultima1K.bat" -Force
$config=[IO.File]::ReadAllText("$root\snapshot\DOS_support\dosbox_800x600.conf")
$config=$config.Replace('fullscreen=false','fullscreen=true')
[IO.File]::WriteAllText("$stage\dosbox_ultima1.conf",$config,[Text.Encoding]::ASCII)
$zip="$src\payload.zip"
if(Test-Path $zip){Remove-Item $zip -Force}
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($stage,$zip,[IO.Compression.CompressionLevel]::Optimal,$false)
$csc='C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe'
Push-Location $src
try {
 & $csc /nologo /target:winexe /out:"$out\Ultima1K(DOS)_Setup.exe" /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /resource:setup.ps1,setup.ps1 /resource:payload.zip,payload.zip StandaloneInstaller.cs
 if($LASTEXITCODE -ne 0){throw 'Compiler failure'}
} finally {Pop-Location}
Get-Item "$out\Ultima1K(DOS)_Setup.exe" | Select-Object FullName,Length
