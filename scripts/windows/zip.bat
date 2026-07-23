@echo off
chcp 65001 >nul
setlocal

:: 获取当前日期时间（格式：yyyyMMdd_HHmm）
for /f %%t in ('powershell -command "Get-Date -Format 'yyyyMMdd_HHmm'"') do set "timestamp=%%t"

:: 设置路径变量
set "projectName=opencvAndHyperlpr"
set "distDir=..\dist"
set "zipName=%distDir%\%projectName%_%timestamp%.zip"
set "targetPath=..\..\build\msvc2022_6_10_3\Release"
set "zipToolPath=%~dp0"

:: 检查构建产物是否存在
if not exist "%targetPath%" (
    echo [ERROR] 未找到构建产物: %targetPath%
    echo         请先在 Qt Creator 中构建 Release 版本。
    pause
    exit /b 1
)

:: 确保 dist 目录存在
if not exist "%distDir%" mkdir "%distDir%"

:: 压缩
echo [INFO] 正在创建压缩包: %zipName%
"%zipToolPath%7za.exe" a -tzip "%zipName%" "%targetPath%"
if errorlevel 1 (
    echo [ERROR] zip failed.
    pause
    exit /b 1
) else (
    echo [SUCCESS] 已生成压缩包: %zipName%
)

echo [INFO] 所有操作已完成
pause
