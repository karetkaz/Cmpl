CCSRCDIR=../../cc/src/
GXSRCDIR=./
OBJDIR=../../obj/
LIBDIR=$(OBJDIR)
BINDIR=../
BINARY=main

OUTOBJ=-o
OUTEXE=-o
CFLAGS=-Wall -Wpadded -g3 -ggdb
CFLAGS=-g0 -Os

CCC=gcc -m32
LNK=gcc -m32
DBG=nemiver
AS=nasm -f elf
MK=make

LIBS=$(LIBDIR)libjpeg.a $(LIBDIR)libpng.a
SYSLIBS=-lm -lX11
ifeq "$(OS)" "Windows_NT"
LIBS=$(LIBDIR)libjpeg.win.a $(LIBDIR)libpng.win.a
SYSLIBS=-lm -lgdi32
AS=nasm -f coff --prefix _
MK=mingw32-make
endif

OBJS=\
	$(OBJDIR)code.o\
	$(OBJDIR)clog.o\
	$(OBJDIR)parse.o\
	$(OBJDIR)tree.o\
	$(OBJDIR)type.o\
	$(OBJDIR)ccvm.o\
	$(OBJDIR)lstd.o\
	\
	$(OBJDIR)drv_ras.o\
	$(OBJDIR)drv_win.o\
	$(OBJDIR)g2_draw.o\
	$(OBJDIR)g2_surf.o\
	$(OBJDIR)g3_draw.o\
	$(OBJDIR)g3_mesh.o\
	$(OBJDIR)main.o\

#~ rebuild:clean build

build:$(LIBS) $(OBJS)
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) $(LIBS) $(SYSLIBS)

clean:
	rm -f $(OBJS)

$(LIBDIR)libjpeg.a:
	$(MK) -C lib/libjpeg
$(LIBDIR)libjpeg.win.a:
	$(MK) -C lib/libjpeg

$(LIBDIR)libpng.a:
	$(MK) -C lib/libpng
$(LIBDIR)libpng.win.a:
	$(MK) -C lib/libpng

$(OBJDIR)%.o: $(GXSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(CCSRCDIR)' '$<' -o '$@'

$(OBJDIR)%.o: $(CCSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(CCSRCDIR)' '$<' -o '$@'

$(OBJDIR)%.o: $(GXSRCDIR)%.asm
	$(AS) '$<' -o '$@'
