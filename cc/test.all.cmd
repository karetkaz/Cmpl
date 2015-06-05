@ECHO OFF

REM ~ files to compile
REM ~ SET files="test.*.cvx"
SET files="test.it.files.cvx"

REM ~ compile flags
SET flags=-gd2 -x -asm19 -w9
REM ~ SET flags=-gd2 -x -w0

SET logFile=out/log.mingw.cvx
SET logFile32=out/log.vs32.cvx
SET logFile64=out/log.vs64.cvx
REM ~ SET vsDir=..\proj\vs.2013\Release\

echo>%logFile%
FOR %%f IN (%files%) DO IF EXIST %%f (
echo "**** running test: %%f"
echo>>%logFile% "**** running test: %%f"
main.exe -la %logFile% %flags% "%%f"
IF ERRORLEVEL 1 echo "******** failed: %%f"
)

IF %vsDir%"" == "" GOTO QUIT
IF NOT EXIST %vsDir%NUL GOTO QUIT

echo>%logFile32%
FOR %%f IN (%files%) DO IF EXIST %%f (
echo "**** running test: %%f"
echo>>%logFile32% "**** running test: %%f"
%vsDir%cc -la %logFile32% %flags% "%%f"
IF ERRORLEVEL 1 echo "******** failed: %%f"
)

echo>%logFile64%
FOR %%f IN (%files%) DO IF EXIST %%f (
echo "**** running test: %%f"
echo>>%logFile64% "**** running test: %%f"
%vsDir%cc64 -la %logFile64% %flags% "%%f"
IF ERRORLEVEL 1 echo "******** failed: %%f"
)

:QUIT
ECHO ON
