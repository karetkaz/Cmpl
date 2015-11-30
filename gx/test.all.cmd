@echo off

SET logFile=out/test.mingw.log
echo>$logFile
FOR %%f IN ("tests/*.gxc") DO IF EXIST tests/%%f (
echo "**** running test: tests/%%f"
echo>>$logFile "**** running test: tests/%%f"
main.exe>>$logFile -s "tests/%%f" arg1 arg2
IF ERRORLEVEL 1 echo "******** failed: tests/%%f"
)
REM ~ pause
