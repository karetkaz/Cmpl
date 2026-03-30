@echo off

SET "MINGW_HOME=C:\msys64"
SET "PATH=%MINGW_HOME%\usr\bin;%PATH%"
SET "PATH=%MINGW_HOME%\ucrt64\bin;%PATH%"

cd "%~dp0\.."
SET CMPL_HOME=%CD%
echo cmpl home is: %CMPL_HOME%

SET "BIN=%CMPL_HOME%\.build\windows"
make -C "%CMPL_HOME%" BINDIR="%BIN%" clean
make -C "%CMPL_HOME%" -j 12 BINDIR="%BIN%" cmpl libFile.dll libGfx.dll

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

SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\demo\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\draw.2d\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\draw.3d\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\games\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\image.fx\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\image.pd\*.cmpl
SET TEST_FILES=%TEST_FILES%;%CMPL_HOME%\cmplGfx\test\widget\*.cmpl

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

:: requirements to build and run on Windows
:: install: https://www.msys2.org
:: install: pacman -S mingw-w64-ucrt-x86_64-gcc
:: install: pacman -S make
