@echo off
setlocal

:: Get the directory where the batch file is located
set "batchDir=%~dp0"

:: Define path to flatc.exe
set "flatcPath=%batchDir%..\flatbuf\flatc.exe"
set "schemeDir=%batchDir%..\..\software\schemas\"

:: Check if flatc.exe exists
if not exist "%flatcPath%" (
    echo Error: flatc.exe not found at "%flatcPath%"
    exit /b
)

:: Run flatc with the corrected paths to generate schemas
echo Generating schemas...
"%flatcPath%" --python "%schemeDir%gpsdo.fbs"

:: Check if schema generation was successful
if errorlevel 1 (
    echo Error: Schema generation failed.
    exit /b
)

echo Schema generation completed successfully.

endlocal
