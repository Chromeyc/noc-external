@echo off
echo Updating offsets...
echo.

set "SCRIPT_DIR=%~dp0"

curl -o "%SCRIPT_DIR%nocExternal\sdk\offsets.h" "https://imtheo.lol/Offsets/Offsets.hpp"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Offsets updated successfully!
    echo File saved to: %SCRIPT_DIR%nocExternal\sdk\offsets.h
) else (
    echo.
    echo Failed to update offsets.
)

echo.
pause


