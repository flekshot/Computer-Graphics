@echo off
setlocal
cd /d "%~dp0"
cmake -S . -B build -A x64
if errorlevel 1 goto failure
cmake --build build --config Release --parallel
if errorlevel 1 goto failure
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto failure
"%~dp0build\Release\SolarSystemExplorer.exe"
exit /b %errorlevel%
:failure
echo Build or verification failed. Read the error above.
pause
exit /b 1
