CCSRCDIR=../../cc/src/
GXSRCDIR=./
OBJDIR=../out/obj
LIBDIR=../out/
BINDIR=../
BINARY=main

OUTOBJ=-o
OUTEXE=-o
CFLAGS=-Wall -Wpadded -g3 -ggdb
CFLAGS=-g0 -Os

CCC=gcc
LNK=gcc
DBG=nemiver
AS=nasm -f elf
MK=make
MKDIR=mkdir -p
LIBS=$(LIBDIR)libjpeg.a $(LIBDIR)libpng.a
SYSLIBS=-lm -lX11

ifeq "$(OS)" "Windows_NT"
	LIBS=$(LIBDIR)libjpeg.win.a $(LIBDIR)libpng.win.a
	SYSLIBS=-lm -lgdi32
	AS=nasm -f coff --prefix _
	MK=mingw32-make
	MKDIR=mkdir
endif

ifeq ($(shell uname -m),x86_64)
	LIBS=$(LIBDIR)libjpeg.x64.a $(LIBDIR)libpng.x64.a
	AS=nasm -f elf64
endif

OBJS=\
	$(OBJDIR)/code.o\
	$(OBJDIR)/parser.o\
	$(OBJDIR)/printer.o\
	$(OBJDIR)/tree.o\
	$(OBJDIR)/type.o\
	$(OBJDIR)/core.o\
	$(OBJDIR)/lstd.o\
	\
	$(OBJDIR)/drv_ras.o\
	$(OBJDIR)/drv_win.o\
	$(OBJDIR)/g2_draw.o\
	$(OBJDIR)/g2_surf.o\
	$(OBJDIR)/g3_draw.o\
	$(OBJDIR)/g3_mesh.o\
	$(OBJDIR)/gx_script.o\
	$(OBJDIR)/main.o\

#~ rebuild:clean build
#~ build:$(OBJDIR)

build:$(OBJDIR) $(LIBS) $(OBJS)
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) $(LIBS) $(SYSLIBS)

clean:
	rm -f $(OBJS)

$(OBJDIR):
	$(MKDIR) "$(OBJDIR)"

$(LIBDIR)libjpeg.a:
	$(MK) -C lib/libjpeg
$(LIBDIR)libjpeg.win.a:
	$(MK) -C lib/libjpeg
$(LIBDIR)libjpeg.x64.a:
	$(MK) -C lib/libjpeg

$(LIBDIR)libpng.a:
	$(MK) -C lib/libpng
$(LIBDIR)libpng.win.a:
	$(MK) -C lib/libpng
$(LIBDIR)libpng.x64.a:
	$(MK) -C lib/libjpeg

$(OBJDIR)/%.o: $(GXSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(CCSRCDIR)' '$<' -o '$@'

$(OBJDIR)/%.o: $(CCSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(CCSRCDIR)' '$<' -o '$@'

$(OBJDIR)/%.o: $(GXSRCDIR)%.asm
	$(AS) '$<' -o '$@'
