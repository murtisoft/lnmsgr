@echo off
::================================================================================================
::	This script is for syncing executables after compile, between the development computer,
::	and test computer. For easy testing over Lan. I assumed git bash wasnt installed, there.
::	Edit the variables given at the top to your liking.
::	Drop it into the executable directory on remote machine and run.
::================================================================================================
set "remote=\\Murticom-2026\e\_qtprojects\LanMessenger\build\Debug\LanMessenger.exe"
set "local=%~dp0LanMessenger.exe"
::================================================================================================

:loop
cls
if not exist "%remote%" goto wait
for %%A in ("%remote%") do set "remote_time=%%~tA"
if "%remote_time%" EQU "%last_synced%" goto wait

if not exist "%local%" goto check_stable
for %%A in ("%local%") do set "local_time=%%~tA"
if "%remote_time%" EQU "%local_time%" (
    set "last_synced=%remote_time%"
    goto wait
)


:check_stable
for %%A in ("%remote%") do set "size1=%%~zA"
timeout /t 2 /nobreak >nul
for %%A in ("%remote%") do set "size2=%%~zA"
if "%size1%" NEQ "%size2%" (
    echo Still compiling...
    goto check_stable
)

:sync
echo Syncing...
taskkill /f /im LanMessenger.exe /t >nul 2>&1
copy /y "%remote%" "%local%" >nul 2>&1
set "last_synced=%remote_time%"
start "" "%local%"

:wait
echo Waiting for changes.
timeout /t 3 /nobreak >nul
goto loop