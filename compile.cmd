@echo off
echo Checking for existing files...
if exist "wifi_monitor.exe" (
    echo Deleting existing wifi_monitor.exe...
    del /f "wifi_monitor.exe"
)
if exist "*.obj" (
    echo Deleting old object files...
    del /f *.obj
)

echo Compiling new wifi_monitor.exe...
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
cd /d %~dp0
cl.exe /Fe:"wifi_monitor.exe" wifi_restart.c /link wlanapi.lib ole32.lib shell32.lib advapi32.lib

echo Cleaning up...
if exist "*.obj" del /f *.obj

if exist "wifi_monitor.exe" (
    echo Compilation successful!
) else (
    echo Compilation failed!
)
pause