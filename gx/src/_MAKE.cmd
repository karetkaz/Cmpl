@echo off

REM ~ set up environment variables

SET OUTD=DEBUG
SET NAME=gfx

REM ~ SET WATCOM=

if exist D:\COMPILERS\WCC\NUL SET WATCOM=D:\COMPILERS\WCC
if exist C:\COMPILERS\WCC\NUL SET WATCOM=C:\COMPILERS\WCC

REM ~ SET WATCOM=G:\Temp\Compilers\WCC

SET INCLUDE=%WATCOM%\H;%WATCOM%\H\NT;%WATCOM%\H\WIN;.\libs\freetype2\include
REM ~ SET WCFLAGS=-zq -w10 -i=%INCLUDE% -5r -d0
SET WCFLAGS=-zq -w10 -i=%INCLUDE% -6s -d0
SET PATH=%PATH%;%WATCOM%\BINW
goto make

:MAKE

REM ~ echo Build libraries
call %OUTD%\_CLEAN.BAT
REM ~ call	3rdpart\libfixed\_makew.bat
REM ~ ren 3rdpart\libfixed\libfixed.lib debug\libfixed.lib
REM ~ ren 3rdpart\libjpeg.lib debug\libjpeg.lib

REM ~ echo Assemble files

%WATCOM%\BINNT\WASM -q -w10 -c	DRV_RAS.ASM
	REM ~ IF ERRORLEVEL 1 GOTO ERROR
%WATCOM%\BINNT\WASM -q -w10 -c	DRV_VBE.ASM
	REM ~ IF ERRORLEVEL 1 GOTO ERROR
%WATCOM%\BINNT\WASM -q -w10 -c	MCGA.ASM
	REM ~ IF ERRORLEVEL 1 GOTO ERROR

REM ~ echo Compile files

%WATCOM%\BINNT\WCC386 %WCFLAGS% GX_SURF.C
	REM ~ IF ERRORLEVEL 1 GOTO ERROR
%WATCOM%\BINNT\WCC386 %WCFLAGS% GX_DRAW.C
	REM ~ IF ERRORLEVEL 1 GOTO ERROR
REM ~ %WATCOM%\BINNT\WCC386 %WCFLAGS% GX_TEXT.C
	REM ~ IF ERRORLEVEL 1 GOTO ERROR
%WATCOM%\BINNT\WCC386 %WCFLAGS% GX_COLOR.C
	REM ~ IF ERRORLEVEL 1 GOTO ERROR

%WATCOM%\BINNT\WCC386 %WCFLAGS% IMAGE.C
	REM ~ IF ERRORLEVEL 1 GOTO ERROR
%WATCOM%\BINNT\WCC386 %WCFLAGS% MAIN.C
	REM ~ IF ERRORLEVEL 1 GOTO ERROR


REM ~ IF NOT EXIST %OUTD%\NUL MD %OUTD%
IF EXIST *.OBJ MOVE *.OBJ %OUTD% > NUL
IF EXIST *.O MOVE *.O %OUTD% > NUL
CD %OUTD%

REM ~ goto exit
REM ~ echo Link files
REM ~ SYSTEM : dos32a, dos4g, causeway, nt, (win32)
%WATCOM%\BINNT\WLINK OP q NAME %NAME% OP stack=2M SYSTEM dos4g @../WMAKE.LNK
REM ~ %WATCOM%\BINNT\WLINK OP q NAME %NAME% OP stack=512K SYSTEM win32 @../WMAKE.LNK
	if errorlevel 1 goto error

echo Execution Comlete
pause

%NAME%
REM ~ >out.txt
REM ~ lut.bmp
REM ~ out.bmp
REM ~ cop.bmp 
REM ~ tmp.bmp
goto exit

:error
echo Error compiling
pause

:exit

REM ~ %CC%		colorop.c
REM ~ %WATCOM%/BINNT/wdis gx_cmat.obj -l=disasm.asm -s
REM ~ %WATCOM%/BINNT/wdis image.obj -l=disasm.asm -s
REM ~ disasm.asm
rem clean up environment variables

pause
