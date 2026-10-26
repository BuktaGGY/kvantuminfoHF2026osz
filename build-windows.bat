@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
  echo Run from the Visual Studio x64 Native Tools Command Prompt.
  pause
  exit /b 1
)
cl /nologo /std:c++17 /O2 /EHsc /MT main.cpp window.cpp /Fe:BlochLab.exe /link opengl32.lib glu32.lib gdi32.lib user32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /O2 /EHsc /MT tests.cpp /Fe:quantum_tests.exe
if errorlevel 1 exit /b 1
quantum_tests.exe
if errorlevel 1 exit /b 1
echo Build OK: BlochLab.exe (no external runtime or graphics libraries required)
pause
