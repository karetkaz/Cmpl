@ECHO OFF

REM ~ files to compile
SET files="test.*.cvx"

REM ~ compile flags
SET flags=-gd2 -x -w0

SET logFile=test.all.log
echo>%logFile%
FOR %%f IN (%files%) DO IF EXIST %%f (
echo "**** running test: %%f"
echo>>%logFile% "**** running test: %%f"
.\main>>%logFile% %flags% "%%f"
IF ERRORLEVEL 1 echo "******** failed: %%f"
)

ECHO ON
