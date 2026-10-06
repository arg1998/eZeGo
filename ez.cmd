@echo off
rem Optional launcher for Windows (B-4). Loads the MSVC developer environment, then maps the
rem same short words as ./ez onto the canonical CMake commands. Not yet run on Windows.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\windows\ez.ps1" %*
exit /b %ERRORLEVEL%
