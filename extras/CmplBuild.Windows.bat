@echo off

cd "%~dp0\.."
SET CMPL_HOME=%CD%
echo cmpl home is: %CMPL_HOME%

SET BIN=%CMPL_HOME%\build\windows
SET BIN_WCC=%BIN%.wcc

SET "MINGW_HOME=%userprofile%\Dropbox\Software\bin\MinGw"
SET "WATCOM=%userprofile%\Dropbox\Software\bin\Watcom\rel2"

IF EXIST "%MINGW_HOME%" (
	SET "PATH=%MINGW_HOME%\bin;%PATH%"
	mingw32-make -C "%CMPL_HOME%" BINDIR="%BIN%" clean
	mingw32-make -C "%CMPL_HOME%" -j 12 BINDIR="%BIN%" cmpl libFile.dll libGfx.dll libOpenGL.dll
)

IF EXIST "%WATCOM%" (
	SET "INCLUDE=%WATCOM%\h\nt;%WATCOM%\h"
	SET "LIB=%WATCOM%\lib386"
	SET "PATH=%WATCOM%\binnt;%PATH%"

	IF NOT EXIST "%BIN_WCC%" mkdir "%BIN_WCC%"
	pushd "%BIN_WCC%"
	owcc -xc -std=c99 -o cmpl.exe %CMPL_HOME%\src\*.c %CMPL_HOME%\cmplStd\src\*.c
	popd
)

:: test the virtual machine
%BIN_WCC%\cmpl>"%BIN_WCC%-vm.dump.md" --test-vm
%BIN%\cmpl>"%BIN%-vm.dump.md" --test-vm

:: dump symbols, assembly, syntax tree and global variables
SET TEST_FLAGS=-X+steps-stdin-offsets -asm/n/s -debug/g "%CMPL_HOME%\cmplStd\test\test.ci"
%BIN_WCC%\cmpl -log/d "%BIN_WCC%.ci" %TEST_FLAGS%
%BIN%\cmpl -log/d "%BIN%.ci" %TEST_FLAGS%

:: dump symbols, documentation, assembly, syntax tree and global variables (to be compared with previous version to test if the code is generated properly)
%BIN%\cmpl -X+steps+fold+fast-stdin-glob-offsets -debug/G/M -api/A/d/p -asm/n/s -ast -doc -log/d/15 "%BIN%-test.dump.ci" -dump.ast.xml "%BIN%-test.dump.xml" "cmplStd\test\test.ci"
:: dump symbols, documentation, assembly, syntax tree and global variables (to be compared with previous version to test if the code is generated properly)
%BIN%\cmpl -X+steps+fold+fast-stdin-glob-offsets -debug/G/M -api/A/d/p -asm/n/s -ast -doc -use -log/d "%BIN%-libs.dump.ci" "%BIN%\libFile.dll" "%BIN%\libGfx.dll"
:: dump profile data in text format including function tracing
%BIN%\cmpl -X-stdin+steps -profile/t/P/G/M -api/A/d/p -asm/g/n/s -ast/t -doc -use -log/d/15 "%BIN%-test.prof.ci" "cmplStd\test\test.ci"
:: dump profile data in json format
%BIN%\cmpl -X-stdin-steps -profile/t/P/G/M -api/A/d/p -asm/g/n/s -ast/t -doc -use -dump.json "%BIN%-test.prof.json" "cmplStd\test\test.ci"

SET TEST_FILES=%CMPL_HOME%\cmplStd\test\test.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\demo\*.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\lang\*.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\math\*.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\text\*.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\time\*.ci

SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\*.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\demo\*.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\demo.procedural\*.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\demo.widget\*.ci

SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplFile\test\*.ci
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\temp\todo.cmplGfx\demo\*.ci
@REM SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGL\test\*.ci

SET DUMP_FILE=%BIN%.dump.exec.ci
%BIN%\cmpl -log/d "%DUMP_FILE%"
FOR %%f IN (%TEST_FILES%) DO (
	pushd "%%~dpf"
	echo **** running test: %%f
	%BIN%\cmpl -X-stdin+steps -run -log/a/d "%DUMP_FILE%" "%BIN%\libFile.dll" "%BIN%\libGfx.dll" "%BIN%\libOpenGL.dll" "%%f"
	IF ERRORLEVEL 1 echo ******** failed: %%f
	popd
)

pause
