@echo off
chcp 65001 >nul
setlocal

set "buildPath=..\..\build\msvc2022_6_10_3"

if exist "%buildPath%" (
    rd /s /q "%buildPath%"
    echo 已删除构建目录: %buildPath%
) else (
    echo 构建目录不存在: %buildPath%
)

echo 操作完成
pause
