@echo off
rem Build the network counter diagnostic probe (development only).
setlocal
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
call "%~dp0find_vcvars.bat"
if errorlevel 1 exit /b 1
call "%VCVARS%" >nul
if errorlevel 1 (
  echo [probe] vcvars64.bat failed
  exit /b 1
)
if not exist "%ROOT%\dist\obj" mkdir "%ROOT%\dist\obj"
pushd "%ROOT%"
cl /nologo /std:c++17 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /I src ^
  tools\net_probe.cpp ^
  /Fo"%ROOT%\dist\obj\\" /Fe"%ROOT%\dist\net_probe.exe" ^
  /link iphlpapi.lib ws2_32.lib
if errorlevel 1 (
  popd
  echo [probe] compile failed
  exit /b 1
)

cl /nologo /std:c++17 /O2 /MT /utf-8 /DUNICODE /D_UNICODE /I src ^
  tools\autostart_probe.cpp src\autostart.cpp ^
  /Fo"%ROOT%\dist\obj\\" /Fe"%ROOT%\dist\autostart_probe.exe" ^
  /link
if errorlevel 1 (
  popd
  echo [probe] autostart probe compile failed
  exit /b 1
)
popd
echo [probe] output: %ROOT%\dist\net_probe.exe
echo [probe] output: %ROOT%\dist\autostart_probe.exe
endlocal
