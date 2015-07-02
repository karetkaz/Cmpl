@ECHO OFF

cd tests
REM ~ files to compile
SET files="test.*.cvx"

REM ~ compile flags
SET flags=-gd2 -x -apiff -astff -asmf9 -w9
SET flags=-gd2 -x -w0

SET logFile=c:/temp/log.mingw.cvx
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
