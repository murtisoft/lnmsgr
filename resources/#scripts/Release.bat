@echo off
set "7zip=C:\Program Files\7-Zip\7z.exe"
set "windeployqt=C:\Qt\6.10.1\mingw_64\bin\windeployqt.exe"
set "src=..\..\build\Release\LanMessenger.exe"
set "destDir=..\..\..\LanMsgRelated\Deployment"
set "zipPath=..\..\..\LanMsgRelated\Win64.zip"
set "targetRoot=..\..\..\lnmsgr"

if not exist "%src%" (
    echo Error: Source executable not found.
    pause
    exit /b 1
)

:: Build and Zip
del /q /s "%destDir%\*" 2>nul
copy /y "%src%" "%destDir%\LanMessenger.exe"
"%windeployqt%" "%destDir%\LanMessenger.exe"
if exist "%destDir%\translations" rd /s /q "%destDir%\translations"
if exist "%zipPath%" del "%zipPath%"
"%7zip%" a "%zipPath%" "%destDir%\*"

:: Targeted lnmsgr sync
rd /s /q "%targetRoot%\resources" 2>nul
rd /s /q "%targetRoot%\sources" 2>nul
del /q "%targetRoot%\CMakeLists.txt" 2>nul

xcopy /e /i /y "..\..\resources" "%targetRoot%\resources"
xcopy /e /i /y "..\..\sources" "%targetRoot%\sources"
copy /y "..\..\CMakeLists.txt" "%targetRoot%\CMakeLists.txt"

pause