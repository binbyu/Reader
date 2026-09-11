@echo off
rem ===========================================================================
rem  build-local.bat  -  local replacement for rebuild.bat
rem
rem  Why it exists (rebuild.bat only works on the author's own machine):
rem   * it hardcodes
rem       D:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\...
rem     and calls devenv, so nothing is found on a normal installation;
rem   * the project asks for PlatformToolset v142 and Windows SDK 10.0.17763.0,
rem     which a current Visual Studio usually does not have installed;
rem   * it reads the file version with wmic, which no longer exists on
rem     Windows 11 24H2+ (build 26000+), so it always ends in "ERROR: failed.";
rem   * tool\7z.exe is the full 7-Zip console build and needs tool\7z.dll,
rem     which is NOT part of this source tree, so packing fails with
rem     "Can't load module: 7z.dll".
rem
rem  This script finds the Visual Studio / Windows SDK you actually have,
rem  builds Release x86 + x64 and packs them into publish\.
rem
rem  Overrides:  set TOOLSET=v142     (default v143)
rem ===========================================================================
setlocal enabledelayedexpansion
set "work_dir=%~dp0"
cd /d "%work_dir%"

set "sln=Reader.sln"
set "publish=publish"
if not defined TOOLSET set "TOOLSET=v143"

set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" (echo [ERROR] vswhere.exe not found & goto _err)

rem prefer VS2022 (toolset v143), otherwise use the newest installed VS
set "msbuild="
for /f "usebackq delims=" %%i in (`"%vswhere%" -latest -products * -version "[17.0,18.0)" -find MSBuild\**\Bin\MSBuild.exe`) do set "msbuild=%%i"
if not defined msbuild (
    for /f "usebackq delims=" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "msbuild=%%i"
)
if not defined msbuild (echo [ERROR] MSBuild.exe not found & goto _err)
echo [INFO] MSBuild  : %msbuild%
echo [INFO] toolset  : %TOOLSET%

set "sdk="
for /f "delims=" %%i in ('dir /b /o-n "%ProgramFiles(x86)%\Windows Kits\10\Include" 2^>nul') do if not defined sdk set "sdk=%%i"
if not defined sdk (echo [ERROR] Windows SDK not found under Windows Kits\10\Include & goto _err)
echo [INFO] WinSDK   : %sdk%

rem archiver: a real 7-Zip if installed, else the vendored one (needs 7z.dll)
set "sevenzip="
if exist "%ProgramFiles%\7-Zip\7z.exe" set "sevenzip=%ProgramFiles%\7-Zip\7z.exe"
if not defined sevenzip if exist "%ProgramFiles(x86)%\7-Zip\7z.exe" set "sevenzip=%ProgramFiles(x86)%\7-Zip\7z.exe"
if not defined sevenzip set "sevenzip=%work_dir%tool\7z.exe"
echo [INFO] archiver : %sevenzip%

if not exist "%publish%" mkdir "%publish%"

call :build Release x86  || goto _err
call :build Release x64  || goto _err

echo.
echo [OK] all builds completed, packages are in "%publish%"
dir /b "%publish%"
pause
exit /b 0


:build
setlocal enabledelayedexpansion
set "cfg=%~1"
set "plat=%~2"
set "suffix="
if /i "%plat%"=="x64" set "suffix=_x64"

echo.
echo [INFO] === building %cfg% ^| %plat% ===
"%msbuild%" "%sln%" /nologo /v:m /t:Rebuild ^
    /p:Configuration=%cfg% /p:Platform=%plat% ^
    /p:PlatformToolset=%TOOLSET% /p:WindowsTargetPlatformVersion=%sdk%
if errorlevel 1 (echo [ERROR] build failed: %cfg% ^| %plat% & exit /b 1)

if /i "%plat%"=="x64" (set "outdir=x64\%cfg%") else (set "outdir=%cfg%")
set "exe=%work_dir%%outdir%\Reader.exe"
if not exist "%exe%" (echo [ERROR] not found: %exe% & exit /b 1)

set "vers="
for /f "usebackq delims=" %%v in (`powershell -NoProfile -Command "(Get-Item '%exe%').VersionInfo.FileVersion"`) do set "vers=%%v"
if not defined vers (echo [ERROR] cannot read file version of %exe% & exit /b 1)
echo [INFO] version : %vers%

set "pkgdir=%publish%\Reader_v%vers%%suffix%"
if exist "%pkgdir%" rd "%pkgdir%" /q /s
mkdir "%pkgdir%"
copy /y "%exe%" "%pkgdir%\" >nul
copy /y "%work_dir%readme.txt" "%pkgdir%\" >nul

pushd "%pkgdir%"
set "archive="
"%sevenzip%" a "Reader_v%vers%%suffix%.7z" Reader.exe readme.txt >nul 2>&1
if exist "Reader_v%vers%%suffix%.7z" set "archive=Reader_v%vers%%suffix%.7z"
if not defined archive (
    echo [WARN] 7-Zip failed ^(missing 7z.dll?^) - falling back to .zip
    powershell -NoProfile -Command "Compress-Archive -Path 'Reader.exe','readme.txt' -DestinationPath 'Reader_v%vers%%suffix%.zip' -Force"
    if exist "Reader_v%vers%%suffix%.zip" set "archive=Reader_v%vers%%suffix%.zip"
)
if not defined archive (popd & echo [ERROR] packing failed & exit /b 1)
del Reader.exe
del readme.txt
popd

echo [OK] package: %pkgdir%\%archive%
endlocal
exit /b 0


:_err
echo.
echo [FAILED]
pause
exit /b 1
