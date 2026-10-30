@echo off

set "work_dir=%cd%"
set "sln_name=Reader"
set "publish=publish"
set "ret=0"
set "vers="

setlocal enableextensions

:: Find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)
if not defined VS_PATH (
    call :print_error "Visual Studio installation not found"
    exit /b 1
)

:: Set MSBuild path
set "MSBUILD=%VS_PATH%\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
    set "MSBUILD=%VS_PATH%\MSBuild\15.0\Bin\MSBuild.exe"
)
if not exist "%MSBUILD%" (
    call :print_error "MSBuild.exe not found"
    exit /b 1
)

:: build networkless x64
call:func_build Release x64 1
if "%ret%" == "1" goto _err
call:func_get_version x64\Release
if "%vers%" == "" goto _err
call:func_copy_to_publish x64\Release %vers% 1
call:func_packet %vers% networkless
call:func_cleanup x64\Release 1

:: build Release x86(support winxp)
call:func_build Release x86
if "%ret%" == "1" goto _err
call:func_get_version Release
if "%vers%" == "" goto _err
call:func_copy_to_publish Release %vers%
call:func_packet %vers% x86_winxp
call:func_cleanup Release 1

:: build Debug x64
call:func_build Debug x64
if "%ret%" == "1" goto _err
call:func_get_version x64\Debug
if "%vers%" == "" goto _err
call:func_copy_to_publish x64\Debug %vers%
call:func_packet %vers% debug
call:func_cleanup x64\Debug 1

:: build Release x64
call:func_build Release x64
if "%ret%" == "1" goto _err
call:func_get_version x64\Release
if "%vers%" == "" goto _err
call:func_copy_to_publish x64\Release %vers%
call:func_packet %vers%
call:func_cleanup x64\Release 1
call:func_cleanup x64 1


:: completed
:_completed
echo "SUCCESS: completed."
pause
exit

:: failed
:_err
echo "ERROR: failed."
pause
exit


:func_build
:: %1 is Release/Debug, %2 is x86/x64, %3 is networkless
if "%3" == "1" (
copy "%sln_name%\%sln_name%.vcxproj" "%sln_name%\%sln_name%_bak.vcxproj"
tool\repstr "ENABLE_NETWORK;" "" "%sln_name%\%sln_name%.vcxproj"
)

:: devenv %sln_name%.sln /rebuild "%1|%2"
"%MSBUILD%" "%sln_name%.sln" ^
			/property:Configuration=%1 ^
            /property:Platform=%2 ^
            /target:Clean;Build ^
            /verbosity:minimal ^
            /consoleloggerparameters:Summary

if "%3" == "1" (
del "%sln_name%\%sln_name%.vcxproj" /q
ren "%sln_name%\%sln_name%_bak.vcxproj" "%sln_name%.vcxproj"
)
if "%2" == "x64" (
if not exist "%work_dir%\%2\%1\%sln_name%.exe" (
set "ret=1"
)
) else (
if not exist "%work_dir%\%1\%sln_name%.exe" (
set "ret=1"
)
)
goto:eof

:func_get_version
:: %1 is config
set "file=%work_dir%\%1\%sln_name%.exe"
if not defined file (
set "ret=1"
goto:eof
)
if not exist "%file%" (
set "ret=1"
goto:eof
)
FOR /F "tokens=2 delims==" %%a in ('
    wmic datafile where name^="%file:\=\\%" get Version /value 
') do set "vers=%%a"
goto:eof

:func_copy_to_publish
:: %1 is config, %2 is version, %3 need cleanup
if "%3" == "1" (
if exist "%work_dir%\%publish%\Reader_v%2" rd "%work_dir%\%publish%\Reader_v%2" /q /s
mkdir "%work_dir%\%publish%\Reader_v%2"
)
xcopy /y "%work_dir%\%1\%sln_name%.exe" "%work_dir%\%publish%\Reader_v%2\"
xcopy /y "%work_dir%\misc\readme.txt" "%work_dir%\%publish%\Reader_v%2\"
goto:eof

:func_packet
:: %1 is version, %2 is suffixname
if "%2" == "" (
set "target=%work_dir%\%publish%\Reader_v%1\%sln_name%_v%1.7z"
) else (
set "target=%work_dir%\%publish%\Reader_v%1\%sln_name%_v%1_%2.7z"
)
cd "%work_dir%\%publish%\Reader_v%1"
..\..\tool\7z a %target% %sln_name%.exe readme.txt
del %sln_name%.exe
del readme.txt
cd "%work_dir%"
goto:eof

:func_cleanup
:: %1 is config, %2 need clean target
if exist "%work_dir%\%sln_name%\%1" rd "%work_dir%\%sln_name%\%1" /q /s
if "%2" == "1" (
if exist "%work_dir%\%1" rd "%work_dir%\%1" /q /s
)
goto:eof

:print_error
echo [ERROR] %~1
exit /b 0