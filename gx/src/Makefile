SRCDIR=./
OBJDIR=../../obj/
LIBDIR=$(OBJDIR)
BINDIR=../
BINARY=main

OUTOBJ=-o
OUTEXE=-o
#~ CFLAGS=-g3 -ggdb
CFLAGS=-g0 -Os
COPTS=-c -Wall -Wpadded
#~ COPTS=-c -Wall
CCC=gcc -m32
LNK=gcc -m32
DBG=nemiver
AS=nasm -f elf
MK=make

LIBS=$(LIBDIR)libjpeg.a $(LIBDIR)libpng.a $(LIBDIR)libpvmc.a
SYSLIBS=-lm -lX11
ifeq "$(OS)" "Windows_NT"
LIBS=$(LIBDIR)libjpeg.win.a $(LIBDIR)libpng.win.a $(LIBDIR)libpvmc.win.a
SYSLIBS=-lm -lgdi32
AS=nasm -f coff --prefix _
MK=mingw32-make
endif

OBJS=\
	$(OBJDIR)drv_ras.o\
	$(OBJDIR)drv_win.o\
	$(OBJDIR)g2_draw.o\
	$(OBJDIR)g2_surf.o\
	$(OBJDIR)g3_draw.o\
	$(OBJDIR)g3_mesh.o\
	$(OBJDIR)main.o\

#~ all:build

#~ rebuild:clean build

build:$(LIBS) $(OBJS)
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) $(LIBS) $(SYSLIBS)

clean:
	rm -f $(OBJS)
	$(MK) -C lib/libpvmc clean
#	rm -f $(LIBDIR)libpvmc.win.a

$(LIBDIR)libpvmc.a:
	$(MK) -C lib/libpvmc
$(LIBDIR)libpvmc.win.a:
	$(MK) -C lib/libpvmc

$(LIBDIR)libjpeg.a:
	$(MK) -C lib/libjpeg
$(LIBDIR)libjpeg.win.a:
	$(MK) -C lib/libjpeg

$(LIBDIR)libpng.a:
	$(MK) -C lib/libpng
$(LIBDIR)libpng.win.a:
	$(MK) -C lib/libpng


$(OBJDIR)drv_ras.o:
	$(AS) -o $(OBJDIR)drv_ras.o $(SRCDIR)drv_ras.asm

$(OBJDIR)%.o: $(SRCDIR)%.c
	$(CCC) -c $(CFLAGS) -Wall -Wpadded $< -o $@
