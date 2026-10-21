# Runs on a fresh GitHub Actions Windows runner. Inputs are source-built artifacts.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$dist = Join-Path $root 'dist'
$win = Join-Path $dist 'stage-win'
$dos = Join-Path $dist 'stage-dos'
$release = Join-Path $dist 'release'
New-Item -ItemType Directory -Force $release | Out-Null
foreach ($p in @("$win\Ultima1KR.exe","$win\SDL2.dll","$dos\ULTIHF99.EXE",
  "$win\FONTS\H12.FNT","$win\FONTS\ROBO.FNT","$dos\FONTS\H12.FNT",
  "$dos\FONTS\ROBO.FNT")) {
  if (!(Test-Path -LiteralPath $p -PathType Leaf)) { throw "Missing build input: $p" }
}
foreach ($name in @('H12.FNT','ROBO.FNT')) {
  $expected = (Get-FileHash (Join-Path $root "assets\FONTS\$name") -Algorithm SHA256).Hash
  foreach ($stage in @($win,$dos)) {
    if ((Get-FileHash (Join-Path $stage "FONTS\$name") -Algorithm SHA256).Hash -ne $expected) {
      throw "Font integrity failure: $stage $name"
    }
  }
}
# The custom Windows installer contains ONLY project code, runtime DLLs and project fonts.
# Original commercial game data and user saves are deliberately never packaged.
$payload = Join-Path $dist 'windows-payload.zip'
if (Test-Path $payload) { Remove-Item $payload -Force }
Compress-Archive -Path (Join-Path $win '*') -DestinationPath $payload -CompressionLevel Optimal
$csc = 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe'
if (!(Test-Path $csc)) { throw 'Windows .NET Framework C# compiler missing' }
$winExe = Join-Path $release 'Ultima1K(Win)_Setup.exe'
& $csc /nologo /target:winexe "/out:$winExe" /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.IO.Compression.dll "/resource:$payload,payload.zip" (Join-Path $root 'installer\NativeInstaller.cs')
if ($LASTEXITCODE -ne 0 -or !(Test-Path $winExe)) { throw 'Windows installer compile failed' }

# DOSBox is a separately licensed open-source runtime, installed from the package manager.
choco install dosbox -y --no-progress --limit-output
if ($LASTEXITCODE -ne 0) { throw 'DOSBox installation failed' }
$dosboxExe = Get-ChildItem -Path @('C:\Program Files (x86)','C:\Program Files','C:\tools') -Filter DOSBox.exe -File -Recurse -ErrorAction SilentlyContinue |
  Where-Object { $_.FullName -match 'DOSBox' } | Select-Object -First 1
if (!$dosboxExe) { throw 'DOSBox.exe not found after package installation' }
$dosboxDest = Join-Path $dos 'DOSBox'
New-Item -ItemType Directory -Force $dosboxDest | Out-Null
foreach ($name in @('DOSBox.exe','SDL.dll','SDL_net.dll','COPYING.txt','AUTHORS.txt')) {
  $src = Join-Path $dosboxExe.DirectoryName $name
  if (!(Test-Path $src)) { throw "Missing DOSBox distribution component: $src" }
  Copy-Item $src (Join-Path $dosboxDest $name)
}
# CWSDPMI is distributed separately by the DJGPP project.
$csZip = Join-Path $dist 'csdpmi7b.zip'
Invoke-WebRequest 'https://www.delorie.com/pub/djgpp/current/v2misc/csdpmi7b.zip' -OutFile $csZip
Expand-Archive -LiteralPath $csZip -DestinationPath (Join-Path $dist 'csdpmi') -Force
Copy-Item (Join-Path $dist 'csdpmi\bin\CWSDPMI.EXE') (Join-Path $dos 'CWSDPMI.EXE')

choco install innosetup -y --no-progress --limit-output
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup installation failed' }
$iscc = @('C:\Program Files (x86)\Inno Setup 6\ISCC.exe','C:\Program Files\Inno Setup 6\ISCC.exe',"$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe") |
  Where-Object { Test-Path $_ } | Select-Object -First 1
if (!$iscc) { throw 'ISCC.exe not found' }
& (Join-Path $root 'installer\dos\build_inno_dos.ps1') -StageDir $dos -OutputDir $release -Compiler $iscc
if ($LASTEXITCODE -ne 0) { throw 'DOS installer build failed' }
$seven = 'C:\Program Files\7-Zip\7z.exe'
if (!(Test-Path $seven)) {
  choco install 7zip -y --no-progress --limit-output
  if ($LASTEXITCODE -ne 0) { throw '7-Zip installation failed' }
}
foreach ($edition in @('Win','DOS')) {
  $exe = Join-Path $release "Ultima1K(${edition})_Setup.exe"
  $archive = Join-Path $release "Ultima1K(${edition})_Setup.7z"
  if (!(Test-Path $exe)) { throw "Installer missing: $exe" }
  & $seven a -t7z -mx=9 $archive $exe
  if ($LASTEXITCODE -ne 0) { throw "Archive creation failed: $archive" }
  & $seven t $archive
  if ($LASTEXITCODE -ne 0) { throw "Archive verification failed: $archive" }
}
$manifest = foreach ($f in Get-ChildItem $release -File) {
  $h = Get-FileHash $f.FullName -Algorithm SHA256
  "$($h.Hash.ToLowerInvariant())  $($f.Name)"
}
$manifest | Set-Content (Join-Path $release 'SHA256SUMS.txt') -Encoding ascii
Get-ChildItem $release | Select-Object Name,Length
Write-Host 'INSTALLERS_AND_7Z_VERIFIED'
