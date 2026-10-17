@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1
"DOSBox\DOSBox.exe" -conf "dosbox_ultima1.conf" -fullscreen
set "RC=%ERRORLEVEL%"
popd
exit /b %RC%
