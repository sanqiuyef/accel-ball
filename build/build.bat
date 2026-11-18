@echo off
setlocal
for %%I in ("%~dp0..") do set "ROOT=%%~fI"

set "MODE=%~1"
if "%MODE%"=="" set "MODE=release"

set "RCNAME=app.rc"
set "OUTNAME=AccelBall.exe"
set "EXTRA_DEFINE="
if /I "%MODE%"=="dev" (
  set "RCNAME=app-dev.rc"
  set "OUTNAME=AccelBall-dev.exe"
  set "EXTRA_DEFINE=/DACCELBALL_DEV"
)

if not exist "%ROOT%\dist" mkdir "%ROOT%\dist"
if not exist "%ROOT%\dist\obj" mkdir "%ROOT%\dist\obj"

call "%~dp0find_vcvars.bat"
if errorlevel 1 exit /b 1
call "%VCVARS%" >nul
if errorlevel 1 (
  echo [build] vcvars64.bat failed
  exit /b 1
)

pushd "%ROOT%\src"
rc /nologo /fo "%ROOT%\dist\app.res" "%RCNAME%"
if errorlevel 1 (
  popd
  echo [build] rc failed
  exit /b 1
)
popd

cl /nologo /std:c++17 /O2 /MT /EHsc /W4 /utf-8 /DUNICODE /D_UNICODE %EXTRA_DEFINE% ^
  /Fo"%ROOT%\dist\obj\\" /Fd"%ROOT%\dist\obj\\" /Fe"%ROOT%\dist\%OUTNAME%" ^
  "%ROOT%\src\main.cpp" "%ROOT%\src\ball_window.cpp" "%ROOT%\src\metrics.cpp" ^
  "%ROOT%\src\cleaner.cpp" "%ROOT%\src\render.cpp" "%ROOT%\src\util.cpp" ^
  "%ROOT%\src\autostart.cpp" ^
  "%ROOT%\dist\app.res" ^
  /link /SUBSYSTEM:WINDOWS /MANIFEST:NO user32.lib gdi32.lib gdiplus.lib psapi.lib ^
  iphlpapi.lib shell32.lib advapi32.lib ole32.lib
if errorlevel 1 (
  echo [build] compile failed
  exit /b 1
)

echo [build] output: %ROOT%\dist\%OUTNAME%
endlocal
