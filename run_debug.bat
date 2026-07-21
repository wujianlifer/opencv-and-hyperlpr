@echo off
setlocal
set PATH=e:\QtProject\opencvAndHyperlpr\build\msvc2022_6_10_3-Debug;%PATH%
cd /d "e:\QtProject\opencvAndHyperlpr\build\msvc2022_6_10_3-Debug"
echo Running opencvAndHyperlpr.exe...
opencvAndHyperlpr.exe
echo Exit code: %errorlevel%
pause
