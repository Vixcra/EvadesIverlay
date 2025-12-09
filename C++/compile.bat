@echo off
taskkill /F /IM EvadesOverlay.exe 2>nul
timeout /t 1 /nobreak >nul
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /D_UNICODE /DUNICODE /std:c++17 EvadesOverlay.cpp /Fe:EvadesOverlay.exe gdiplus.lib user32.lib gdi32.lib > compile_log.txt 2>&1
type compile_log.txt
