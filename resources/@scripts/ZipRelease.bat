@echo off
set "sevenzip=C:\Program Files\7-Zip\7z.exe"
set "windeployqt=C:\Qt\6.10.1\mingw_64\bin\windeployqt.exe"
set "openssl=C:\Qt\Tools\OpenSSL-Win64"
set "projectdir=E:\_qtprojects\LanMessenger"


if not exist "%projectdir%\build\Release\LanMessenger.exe" (
    echo Error: Source executable not found.
    pause
    exit /b 1
)

:: Build and Zip
del /q /s "%projectdir%\build\Deployment\*" 2>nul
copy /y "%projectdir%\build\Release\LanMessenger.exe" "%projectdir%\build\Deployment\LanMessenger.exe"
if not exist "%projectdir%\build\Deployment" mkdir "%projectdir%\build\Deployment"
"%windeployqt%" "%projectdir%\build\Deployment\LanMessenger.exe"
if exist "%projectdir%\build\Deployment\translations" rd /s /q "%projectdir%\build\Deployment\translations"
if exist "%projectdir%\build\Win64.zip" del "%projectdir%\build\Win64.zip"
copy /y "%openssl%\libcrypto-3-x64.dll" "%projectdir%\build\Deployment\"
copy /y "%openssl%\libssl-3-x64.dll" "%projectdir%\build\Deployment\"

del "%projectdir%\build\Win64.zip" 2>nul
"%sevenzip%" a "%projectdir%\build\Win64.zip" "%projectdir%\build\Deployment\*"


pause