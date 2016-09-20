@ECHO off
cd ..
for /f "tokens=2-4 delims=/ " %%a in ('date /T') do (
  set year=%%c
  set month=%%a
  set day=%%b
)

REM ~ SET zipapp="C:\Program Files\7-Zip\7z.exe"
REM ~ IF NOT EXIST zipapp SET zipapp="C:\Program Files (x86)\7-Zip\7z.exe"
REM ~ IF NOT EXIST zipapp SET zipapp="tools\7z.exe"
SET zipapp="tools\7za.exe"
SET zipapp=%zipapp% a

SET zipName=..\Backup\gxcc\gxcc.%year%.%month%.%day%.7z

SET files=*.cvx;*.properties
REM ~ compiler files
SET files=%files%;cc\src\*.*;cc\*.cvx;cc\*.properties
REM ~ graphics files
SET files=%files%;gx\src\*.*;gx\*.gxc;gx\*.properties
REM ~ test files
SET files=%files%;cc\tests\*.*;gx\tests\*.*
REM ~ media files
SET files=%files%;gx\media\scripts\*.*;gx\media\fonts\*.*
SET files=%files%;gx\*.ini;gx\*.sh;gx\*.cmd;gx\tests\*.*

FOR %%f IN (%files%) DO IF EXIST %%f (
REM ~ echo **** archiving "%%f"
%zipapp% %zipName% "%%f"
IF ERRORLEVEL 1 GOTO QUIT
)

:QUIT
pause
