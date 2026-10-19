$ErrorActionPreference = 'Stop'
$csc = 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe'
$root = 'C:\Users\hojun\Ultima1'
$out = 'C:\Users\hojun\Downloads\Ultima1K_Native_Installer'
New-Item -ItemType Directory -Force $out | Out-Null
$refs = @('/reference:System.Windows.Forms.dll','/reference:System.Drawing.dll','/reference:System.IO.Compression.dll')
& $csc /nologo /target:winexe /define:DOS ("/out:$out\Ultima1K(DOS)_Setup.exe") $refs ("/resource:$root\installer\dos\payload.zip,payload.zip") "$root\installer\NativeInstaller.cs"
if($LASTEXITCODE -ne 0){throw 'DOS compile failed'}
& $csc /nologo /target:winexe ("/out:$out\Ultima1K(Win)_Setup.exe") $refs '/resource:C:\Users\hojun\Downloads\Ultima1K_Windows_Current\payload.zip,payload.zip' "$root\installer\NativeInstaller.cs"
if($LASTEXITCODE -ne 0){throw 'WIN compile failed'}
Get-ChildItem $out | Select-Object Name,Length
Get-ChildItem $out -File | Get-FileHash -Algorithm SHA256 | Select-Object Path,Hash
