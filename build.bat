@echo off
setlocal

echo +==========================================+
echo |  Orderbook Visualizer -- Build Script   |
echo +==========================================+
echo.

set VS_CMAKE="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set CMAKE_EXE=cmake

if exist %VS_CMAKE% (
    set CMAKE_EXE=%VS_CMAKE%
) else (
    where cmake >nul 2>&1
    if %ERRORLEVEL% neq 0 (
        echo ERROR: cmake not found. Install CMake or Visual Studio 2022+.
        pause & exit /b 1
    )
)

set BUILD_DIR=build
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [1/2] Configuring...
%CMAKE_EXE% -B "%BUILD_DIR%" -S . -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 ( echo ERROR: Configure failed. & pause & exit /b 1 )

echo.
echo [2/2] Building...
%CMAKE_EXE% --build "%BUILD_DIR%" --config Release --parallel 4
if %ERRORLEVEL% neq 0 ( echo ERROR: Build failed. & pause & exit /b 1 )

echo.
echo +==========================================+
echo |  BUILD SUCCESSFUL                        |
echo |  Run: build\Release\orderbook_viz.exe    |
echo |  Or:  run.bat           (BTC/USDT)       |
echo |  Or:  run.bat ETH       (ETH/USDT)       |
echo +==========================================+
echo.
pause
