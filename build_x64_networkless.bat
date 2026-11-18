@echo off
setlocal enableextensions
cd /d "%~dp0"

set "BUILD_TOOLS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools"
set "MSBUILD=%BUILD_TOOLS%\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
    echo [ERROR] MSBuild not found at "%MSBUILD%"
    exit /b 1
)

if exist "Reader\Reader_bak.vcxproj" del "Reader\Reader_bak.vcxproj" /q
copy /y "Reader\Reader.vcxproj" "Reader\Reader_bak.vcxproj" >nul
if errorlevel 1 (
    echo [ERROR] failed to backup vcxproj
    exit /b 1
)

tool\repstr.exe "ENABLE_NETWORK;" "" "Reader\Reader.vcxproj"

"%MSBUILD%" Reader.sln /property:Configuration=Release /property:Platform=x64 /target:Build /verbosity:minimal /consoleloggerparameters:Summary /m
set "rc=%ERRORLEVEL%"

del "Reader\Reader.vcxproj" /q
ren "Reader\Reader_bak.vcxproj" "Reader.vcxproj"

if exist "x64\Release\Reader.exe" (
    echo [OK] build succeeded: x64\Release\Reader.exe
    exit /b 0
)
echo [ERROR] build failed, rc=%rc%
exit /b 1
