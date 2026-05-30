@echo off
set EXE=build\Release\orderbook_viz.exe
if not exist "%EXE%" (
    echo Binary not found. Run build.bat first.
    pause
    exit /b 1
)
%EXE% %*
