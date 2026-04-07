@echo off
::================================================================================================
::	This script is for Syncing executables after compile, between the development computer,
::	and test computer. For easy testing over Lan. I assumed git bash wasnt installed there.
::	Edit the variables given at the top to your liking.
::	Drop it into the executable directory on Remote machine and run.
::================================================================================================
set "Remote=\\Murticom-2026\e\_qtprojects\LanMessenger\build\Debug\LanMessenger.exe"
set "Local=%~dp0LanMessenger.exe"
::================================================================================================

:Loop
cls
if not exist "%Remote%" goto Wait
for %%A in ("%Remote%") do set "RemoteTime=%%~tA"
if "%RemoteTime%" EQU "%LastSynced%" goto Wait

if not exist "%Local%" goto CheckStable
for %%A in ("%Local%") do set "LocalTime=%%~tA"
if "%RemoteTime%" EQU "%LocalTime%" (
    set "LastSynced=%RemoteTime%"
    goto Wait
)


:CheckStable
for %%A in ("%Remote%") do set "Size1=%%~zA"
timeout /t 2 /nobreak >nul
for %%A in ("%Remote%") do set "Size2=%%~zA"
if "%Size1%" NEQ "%Size2%" (
    echo Still compiling...
    goto CheckStable
)

:Sync
echo Syncing...
taskkill /f /im LanMessenger.exe /t >nul 2>&1
timeout /t 1 /nobreak >nul
copy /y "%Remote%" "%Local%"
set "LastSynced=%RemoteTime%"
start "" "%Local%"

:Wait
echo Waiting for changes.
timeout /t 3 /nobreak >nul
goto Loop