# End-to-end DOS installer smoke test without redistributing proprietary game data.
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$installer = Join-Path $repo 'dist\release\Ultima1K(DOS)_Setup.exe'
if (!(Test-Path $installer)) { throw 'DOS installer missing' }
$base = Join-Path $env:RUNNER_TEMP 'ultima1-installer-smoke'
$original = Join-Path $base 'licensed-data-fixture'
$target = Join-Path $base 'installed-dos'
New-Item -ItemType Directory -Force $original,(Join-Path $target 'SAVEGAME') | Out-Null
$names = @('ULTIMA.EXE','GEN.EXE','OUT.EXE','MONDAIN.EXE','SPACE.EXE','MAP.BIN','EGATILES.BIN','EGATOWN.BIN','TCD.BIN')
foreach ($name in $names) { [IO.File]::WriteAllBytes((Join-Path $original $name),[byte[]](1,2,3)) }
$sentinel = Join-Path $target 'SAVEGAME\must-survive.txt'
Set-Content $sentinel 'do not overwrite' -Encoding ascii
& $installer /VERYSILENT /SUPPRESSMSGBOXES /NORESTART "/DIR=$target" "/ORIGINAL=$original" "/LOG=$(Join-Path $base 'setup.log')"
if ($LASTEXITCODE -ne 0) { throw "DOS silent install failed: $LASTEXITCODE" }
foreach ($p in @('ULTIHF99.EXE','CWSDPMI.EXE','DOSBox\DOSBox.exe','FONTS\H12.FNT','FONTS\ROBO.FNT','GAMEDATA\ULTIMA.EXE','SAVEGAME\must-survive.txt')) {
  if (!(Test-Path (Join-Path $target $p))) { throw "Missing after install: $p" }
}
if ((Get-Content $sentinel -Raw).Trim() -ne 'do not overwrite') { throw 'Saved game sentinel changed' }
Write-Host 'DOS_SILENT_INSTALL_AND_SAVE_PRESERVATION_OK'
