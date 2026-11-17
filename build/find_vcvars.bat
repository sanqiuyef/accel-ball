@echo off
rem Resolve the Visual Studio vcvars64.bat path.
rem Usage: call "%~dp0find_vcvars.bat" then call "%VCVARS%"
if defined VCVARS exit /b 0

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
  )
)

if not defined VCVARS (
  echo [build] Could not locate vcvars64.bat.
  echo [build] Set VCVARS to its full path, for example:
  echo [build]   set VCVARS=%%ProgramFiles%%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat
  exit /b 1
)

exit /b 0
