@echo off

SET OBJD=..\tmp\
SET LIBD=..\lib\
SET NAME=..\gfx.exe

SET CFLAGS=-c
if not exist %OBJD%NUL mkdir %OBJD%

SET OBJS=%OBJD%g2_surf.obj %OBJD%g2_draw.obj %OBJD%main.obj %OBJD%drv_ras.obj
 REM ~ %OBJD%g3_draw.obj
REM ~ goto link

echo Assemble

ml /nologo /coff /c /Fo"%OBJD%\drv_ras.obj" drv_ras.asm
IF ERRORLEVEL 1 GOTO exit

echo Compile

gcc %CFLAGS% g2_surf.c -o%OBJD%g2_surf.obj
IF ERRORLEVEL 1 GOTO exit

gcc %CFLAGS% g2_draw.c -o%OBJD%g2_draw.obj
IF ERRORLEVEL 1 GOTO exit

gcc %CFLAGS% g3_draw.c -o%OBJD%g3_draw.obj
IF ERRORLEVEL 1 GOTO exit

gcc %CFLAGS% g3_main.c -o%OBJD%main.obj
IF ERRORLEVEL 1 GOTO exit

:link
echo Link

gcc -m32 -o%NAME% %OBJS% -lm -lgdi32 %LIBD%libjpeg/libjpeg.a %LIBD%libpvmc/libpvmc.a
IF ERRORLEVEL 1 GOTO exit

echo Done

:exit
REM ~ pause
