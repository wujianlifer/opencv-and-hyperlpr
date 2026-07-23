@echo off
setlocal
set "ROOT=%~dp0"
if exist "%ROOT%build\msvc2022_6_10_3\Release\opencvAndHyperlpr.exe" (
    cd /d "%ROOT%build\msvc2022_6_10_3\Release"
) else if exist "%ROOT%build\msvc2022_6_10_3\Debug\opencvAndHyperlpr.exe" (
    cd /d "%ROOT%build\msvc2022_6_10_3\Debug"
) else (
    echo 未找到构建产物，请先在 Qt Creator 中构建项目（Release 或 Debug）。
    pause
    exit /b 1
)
opencvAndHyperlpr.exe
pause
