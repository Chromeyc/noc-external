@echo off
echo Updating offsets...
echo.

REM Get the directory where the batch file is located
set "SCRIPT_DIR=%~dp0"

REM Use curl to download offsets and save to offsets.h
curl -o "%SCRIPT_DIR%nocExternal\sdk\offsets.h" "https://imtheo.lol/Offsets/Offsets.hpp"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Offsets updated successfully!
    echo File saved to: %SCRIPT_DIR%nocExternal\sdk\offsets.h
) else (
    echo.
    echo Failed to update offsets. Make sure curl is installed and you have internet connection.
)

echo.
pause

