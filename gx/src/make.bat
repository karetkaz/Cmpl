@echo off

SET BIND=..\
SET OBJD=..\obj\
SET LIBD=..\lib\
SET NAME=gfx.exe

if exist C:\WATCOM\NUL SET WATCOM=C:\WATCOM
if exist C:\COMPILERS\WCC\NUL SET WATCOM=C:\COMPILERS\WCC

if not exist %OBJD%NUL mkdir %OBJD%

SET OBJS=%OBJD%drv_ras.obj %OBJD%g2_surf.obj %OBJD%g2_draw.obj %OBJD%main.obj
REM ~ SET OBJS=%OBJD%mcga.obj,%OBJD%drv_vbe.obj,%OBJD%drv_ras.obj,%OBJD%g2_surf.obj,%OBJD%g2_draw.obj,%OBJD%main.obj

SET INCLUDE=%WATCOM%\h;%WATCOM%\h\nt;%WATCOM%\h\win;%LIBD%libjpeg
SET CFLAGS=-zq -ei -6s -d3 -i=%INCLUDE%
REM ~ SET CFLAGS=-zq -ei -6r -s -w3 -ei -j -fp6 -fpi87 -bt=nt -bm -d0 -oaxt
SET PATH=%PATH%;%WATCOM%\BINNT;%WATCOM%\BINW

REM ~ call	3rdpart\libfixed\_makew.bat
REM ~ ren 3rdpart\libfixed\libfixed.lib debug\libfixed.lib
REM ~ ren 3rdpart\libjpeg.lib debug\libjpeg.lib

echo Assemble

wasm -q -w10 -c drv_ras.asm -fo=%OBJD%drv_ras.obj
IF ERRORLEVEL 1 GOTO exit

REM ~ wasm -q -w10 -c drv_vbe.asm -fo=%OBJD%\drv_vbe.obj
REM ~ IF ERRORLEVEL 1 GOTO exit

REM ~ wasm -q -w10 -c mcga.asm -fo=%OBJD%\mcga.obj
REM ~ IF ERRORLEVEL 1 GOTO exit

echo Compile

wcc386 %CFLAGS% g2_surf.c -fo=%OBJD%g2_surf.obj
IF ERRORLEVEL 1 GOTO exit

wcc386 %CFLAGS% g2_draw.c -fo=%OBJD%g2_draw.obj
IF ERRORLEVEL 1 GOTO exit

wcc386 %CFLAGS% g3_main.c -fo=%OBJD%main.obj
IF ERRORLEVEL 1 GOTO exit

echo Link

REM ~ SYSTEM : dos32a, dos4g, causeway, nt, (win32)
REM ~ wlink OP q Library %LIBD%libjpeg.lib LIBPath %LIBD% PATH %OBJD% FILE %OBJS% NAME %BIND%\%NAME% OP stack=512K SYSTEM dos4g
REM ~ wlink OP q LIBPath %LIBD% PATH %OBJD% FILE %OBJS% NAME %BIND%\%NAME% OP stack=512K SYSTEM nt
wcl386 -bt=dos4g -q -d3s %OBJS% %LIBD%libjpeg.lib -fe=%BIND%%NAME%

IF ERRORLEVEL 1 GOTO exit

echo Done

CD %BIND%
REM ~ wdw 
%NAME%

:exit
REM ~ pause
