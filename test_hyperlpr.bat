@echo off
cd /d "E:\QtProject\opencvAndHyperlpr\build\msvc2022_6_10_3-Debug"

echo Testing HyperLPR with sample image...
echo.

set "MODEL_PATH=E:\QtProject\opencvAndHyperlpr\build\msvc2022_6_10_3-Debug\resource\models\r2_mobile"
set "IMAGE_PATH=E:\QtProject\opencvAndHyperlpr\pictures\car_plate_sample.jpg"

echo Model path: %MODEL_PATH%
echo Image path: %IMAGE_PATH%
echo.

if exist "%IMAGE_PATH%" (
    echo Image exists.
) else (
    echo ERROR: Image not found!
    pause
    exit /b 1
)

if exist "%MODEL_PATH%" (
    echo Model directory exists.
) else (
    echo ERROR: Model directory not found!
    pause
    exit /b 1
)

echo.
echo Running test...
echo.

pause
