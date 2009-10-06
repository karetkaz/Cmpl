#! /bin/bash
LIBD=../lib
OBJD=../obj
BIND=../bin
NAME=gfx.exe

cc=wcc386
ar=wcl386

#~ export WATCOM=~/owc/
#~ export WATCOM=~/owc/
export INCLUDE=$WATCOM/h:$WATCOM/h/nt:$WATCOM/h/win:$LIBD/libjpeg
export PATH=$PATH:$WATCOM/binl

if [ ! -d $OBJD ]; then mkdir $OBJD; fi

OBJS="./$OBJD/main.obj ./$OBJD/drv_ras.obj ./$OBJD/g2_surf.obj ./$OBJD/g2_draw.obj"
LIBS=""

CFLAGS='-zq -w99 -i=$INCLUDE -6s -d3'
#~ -ox'


echo Assemble

wasm -q -w10 -c drv_ras.asm -fo=./$OBJD/drv_ras.obj
if [ $? != 0 ]; then exit; fi

#~ wasm -q -w10 -c	drv_vbe.asm -fo=./$OUTD/drv_vbe.obj
#~ if [ $? != 0 ]; then exit; fi

echo Compile
$cc $CFLAGS g2_surf.c -fo=./$OBJD/g2_surf.obj
if [ $? != 0 ]; then exit; fi

$cc $CFLAGS g2_draw.c -fo=./$OBJD/g2_draw.obj
if [ $? != 0 ]; then exit; fi

$cc $CFLAGS g3_main.c -fo=./$OBJD/main.obj
if [ $? != 0 ]; then exit; fi

echo Link

$ar -k512 -l=nt -q -fe=$BIND/$NAME $LIBS $OBJS
if [ $? != 0 ]; then exit; fi

#~ SYSTEM : dos32a, dos4g, causeway, nt, (win32)
#~ -bd  build Dynamic link library       -fm[=<map_file>]  generate map file
#~ -bm  build Multi-thread application   -k<stack_size> set stack size
#~ -br  build with dll run-time library  -l=<os> link for the specified OS
#~ -bw  build default Windowing app.     -x  make names case sensitive
#~ -bcl=<os> compile and link for OS.    @<file> additional directive file
#~ -fd[=<file>[.lnk]] write directives   "<linker directives>"
#~ -fe=<executable> name executable file

echo Done

#execute
cd $BIND; "./$NAME"
#~ cd $BIND; xterm -e "./$NAME"
