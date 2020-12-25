@echo off

set "drive_letter=C"
set "work_dir=%cd%"
set "sln_name=Reader"
set "vsdevcmd=%drive_letter%:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
set "config=Release"

:: rebuild solution
if exist "%work_dir%\%config%\%sln_name%.exe" del "%work_dir%\%config%\%sln_name%.exe"
call "%vsdevcmd%"
devenv %sln_name%.sln /rebuild %config%

:: check target file
if not exist "%work_dir%\%config%\%sln_name%.exe" goto _err

:: get version
setlocal enableextensions
set "file=%work_dir%\%config%\%sln_name%.exe"
if not defined file goto _err
if not exist "%file%" goto _err
set "vers="
FOR /F "tokens=2 delims==" %%a in ('
    wmic datafile where name^="%file:\=\\%" get Version /value 
') do set "vers=%%a"
echo VERSION: %vers%

:: generate zip file
set "target=%sln_name%_v%vers%"
if exist "%work_dir%\%config%\%target%" rd "%work_dir%\%config%\%target%" /q /s
mkdir "%work_dir%\%config%\%target%"
xcopy /y "%work_dir%\readme.txt" "%work_dir%\%config%\%target%\"
xcopy /y "%work_dir%\%config%\%sln_name%.exe" "%work_dir%\%config%\%target%\"
cd "%work_dir%\%config%\%target%\"
..\..\tool\7z a %target%.zip %sln_name%.exe readme.txt
..\..\tool\7z a %target%.7z %sln_name%.exe readme.txt
del %sln_name%.exe readme.txt
cd "%work_dir%\"
endlocal

:: completed
echo "SUCCESS: completed."
pause
exit

:: failed
:_err
echo "ERROR: failed."
pause