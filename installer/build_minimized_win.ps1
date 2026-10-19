$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$base='C:\Users\hojun\.cache\ultima1-build'
$source=Join-Path $base 'win-minimal-stage'
$zip=Join-Path $base 'win-minimal-payload.zip'
$out='C:\Users\hojun\Downloads\Ultima1K_Native_Installer_Minimized'
New-Item -ItemType Directory -Force $out | Out-Null
if(Test-Path $zip){Remove-Item $zip -Force}
[IO.Compression.ZipFile]::CreateFromDirectory($source,$zip,[IO.Compression.CompressionLevel]::Optimal,$false)
$csc='C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe'
& $csc /nologo /target:winexe ("/out:$out\Ultima1K(Win)_Setup.exe") /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.IO.Compression.dll ("/resource:$zip,payload.zip") 'C:\Users\hojun\Ultima1\installer\NativeInstaller.cs'
if($LASTEXITCODE -ne 0){throw 'Compile failure'}
Get-Item $zip,(Join-Path $out 'Ultima1K(Win)_Setup.exe') | Select-Object FullName,Length
