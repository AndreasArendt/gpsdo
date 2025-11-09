@echo off
setlocal

:: Get the directory where the batch file is located
set "batchDir=%~dp0"

:: Define path to flatcc.exe
set "flatccPath=%batchDir%..\gpsdo\libs\flatcc\bin\Release\flatcc.exe"

:: Check if flatcc.exe exists
if not exist "%flatccPath%" (
    echo Error: flatcc.exe not found at "%flatccPath%"
    exit /b
)

:: Create "include" directory if it doesn't exist
if not exist "%batchDir%include" (
    echo Creating include directory...
    mkdir "%batchDir%include"
)

:: Clean previous generated files in "include" folder
echo Cleaning previous generated files...
del /q "%batchDir%include\*"

:: Run flatcc with the corrected paths to generate schemas
echo Generating schemas...
"%flatccPath%" -a -o "%batchDir%include" "%batchDir%gpsdo.fbs"

:: Check if schema generation was successful
if errorlevel 1 (
    echo Error: Schema generation failed.
    exit /b
)

echo Schema generation completed successfully.

endlocal
