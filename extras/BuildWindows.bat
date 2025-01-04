@echo off

SET "MINGW_HOME=%userprofile%\Dropbox\Software\bin\MinGw"

cd "%~dp0\.."
SET CMPL_HOME=%CD%
echo cmpl home is: %CMPL_HOME%

SET "BIN=%CMPL_HOME%\.build\windows"
SET "PATH=%MINGW_HOME%\bin;%PATH%"
mingw32-make -C "%CMPL_HOME%" BINDIR="%BIN%" clean
mingw32-make -C "%CMPL_HOME%" -j 12 BINDIR="%BIN%" cmpl libFile.dll libGfx.dll

:: test the virtual machine
%BIN%\cmpl>"%BIN%-vm.dump.md" --test-vm

:: dump symbols, assembly, syntax tree and global variables
SET TEST_FLAGS=-X+steps+fold+fast-stdin-glob-offsets -api/A/d/p -asm/n/s -ast -doc -use
%BIN%\cmpl -debug/G/M %TEST_FLAGS% -log/d/15 "%BIN%.test-dump.cmpl" "cmplStd/test/test.cmpl"

:: dump symbols, assembly, syntax tree and global variables
%BIN%\cmpl -debug/G/M %TEST_FLAGS% -log/d/15 "%BIN%.test.cmpl" -dump.ast.xml "%BIN%.test-dump.xml" "%BIN%/libFile.dll" "%BIN%/libGfx.dll" "cmplStd/test/test.cmpl"
:: dump profile data in json format
%BIN%\cmpl -profile/t/P/G/M %TEST_FLAGS%  -dump.json "%BIN%.test.json" "%BIN%/libFile.dll" "%BIN%/libGfx.dll" "cmplStd/test/test.cmpl"

SET TEST_FILES=%CMPL_HOME%\cmplFile\test\*.cmpl

SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\demo\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\lang\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\math\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\text\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\time\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\tmp\done\*.cmpl

SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\demo\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\demo.procedural\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\demo.widget\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\tmp\demo\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\tmp\draw\*.cmpl

SET DUMP_FILE=%BIN%-tests.exec.cmpl
%BIN%\cmpl -log/d "%DUMP_FILE%"
FOR %%f IN (%TEST_FILES%) DO (
	pushd "%%~dpf"
	echo **** running test: %%f
	%BIN%\cmpl -X-stdin+steps -run -log/a/d "%DUMP_FILE%" "%BIN%\libFile.dll" "%BIN%\libGfx.dll" "%%f"
	IF ERRORLEVEL 1 echo ******** failed: %%f
	popd
)

pause
