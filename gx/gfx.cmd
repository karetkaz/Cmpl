@ECHO off
cd %~dp0
REM ~ start main.old.exe %*
REM ~ start 
main.exe %*
REM ~ IF ERRORLEVEL 1 PAUSE
@ECHO on
REM ~ pause
