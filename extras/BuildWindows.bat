@echo off

SET "MINGW_HOME=C:\msys64"
SET "PATH=%MINGW_HOME%\usr\bin;%PATH%"
SET "PATH=%MINGW_HOME%\ucrt64\bin;%PATH%"

cd "%~dp0\.."
SET CMPL_HOME=%CD%
echo cmpl home is: %CMPL_HOME%

SET "CMPL_BIN=%CMPL_HOME%\.build\windows"
SET "CMPL_BIN=%userprofile%\Desktop\Cmpl"

make -C "%CMPL_HOME%" BINDIR="%CMPL_BIN%" clean
make -C "%CMPL_HOME%" -j 12 BINDIR="%CMPL_BIN%" cmpl libFile.dll libGfx.dll

:: test the virtual machine
%CMPL_BIN%\cmpl>"%CMPL_BIN%/vm.dump.md" --test-vm

:: dump symbols, assembly, syntax tree and global variables
SET TEST_FLAGS=-X+steps+fold+fast-stdin-glob-offsets -api/A/d/p -asm/n/s -ast -doc -use
%CMPL_BIN%\cmpl -debug/G/M %TEST_FLAGS% -log/d/15 "%CMPL_BIN%/test-dump.cmpl" "cmplStd/test/test.cmpl"

:: dump symbols, assembly, syntax tree and global variables
%CMPL_BIN%\cmpl -debug/G/M %TEST_FLAGS% -log/d/15 "%CMPL_BIN%/test.cmpl" -dump.ast.xml "%CMPL_BIN%/test-dump.xml" "%CMPL_BIN%/libFile.dll" "%CMPL_BIN%/libGfx.dll" "cmplStd/test/test.cmpl"
:: dump profile data in json format
%CMPL_BIN%\cmpl -profile/t/P/G/M %TEST_FLAGS%  -dump.json "%CMPL_BIN%/test.json" "%CMPL_BIN%/libFile.dll" "%CMPL_BIN%/libGfx.dll" "cmplStd/test/test.cmpl"

SET TEST_FILES=%CMPL_HOME%\cmplFile\test\*.cmpl

SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\demo\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\lang\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\math\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\text\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplStd\test\time\*.cmpl

SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\demo\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\draw.2d\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\draw.3d\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\games\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\image.fx\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\image.pd\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\widget\*.cmpl

SET DUMP_FILE=%CMPL_BIN%/tests.exec.cmpl
%CMPL_BIN%\cmpl -log/d "%DUMP_FILE%"
FOR %%f IN (%TEST_FILES%) DO (
	pushd "%%~dpf"
	echo **** running test: %%f
	%CMPL_BIN%\cmpl -X-stdin+steps -run -log/a/d "%DUMP_FILE%" "%CMPL_BIN%\libFile.dll" "%CMPL_BIN%\libGfx.dll" "%%f"
	IF ERRORLEVEL 1 echo ******** failed: %%f
	popd
)

pause

:: requirements to build and run on Windows
:: install: https://www.msys2.org
:: install: pacman -S make mingw-w64-ucrt-x86_64-gcc
