@echo off
setlocal

:: Get the directory where the batch file is located
set "batchDir=%~dp0"

:: Define path to flatc.exe (adjust if needed)
set "flatcPath=%batchDir%..\flatbuf\flatc.exe"

:: Define output folder for generated schemas: a "schemas" folder in the same directory as the batch file
set "outputDir=%batchDir%schemas"

:: Create output folder if it doesn't exist
if not exist "%outputDir%" (
    mkdir "%outputDir%"
)

:: Define the schema file location (adjust if needed)
set "schemeDir=%batchDir%..\..\software\schemas\"
set "schemaFile=%schemeDir%gpsdo.fbs"

:: Check if flatc.exe exists
if not exist "%flatcPath%" (
    echo Error: flatc.exe not found at "%flatcPath%"
    exit /b
)

:: Check if schema file exists
if not exist "%schemaFile%" (
    echo Error: Schema file not found at "%schemaFile%"
    exit /b
)

:: Run flatc to generate Python code into the outputDir
echo Generating Python schemas into "%outputDir%"...
"%flatcPath%" --python -o "%outputDir%" "%schemaFile%"

:: Check if schema generation was successful
if errorlevel 1 (
    echo Error: Schema generation failed.
    exit /b
)

echo Schema generation completed successfully.
endlocal
