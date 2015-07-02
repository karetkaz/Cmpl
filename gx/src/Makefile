CCSRCDIR=../../cc/src/
#~ CCSRCDIR=../../cc/src/
GXSRCDIR=./
OBJDIR=../out/obj
LIBDIR=../out/
BINDIR=../
BINARY=main

OUTEXE=-o
#~ CFLAGS=-Wall -Wpadded -g3 -ggdb
CFLAGS=-Wall -Wpadded -g0 -Os

ARH=-m32
CCC=gcc $(ARH)
LNK=gcc $(ARH)
#~ DBG=nemiver
AS=nasm -f elf
MK=make
MKDIR=mkdir -p
LIBS=$(LIBDIR)libjpeg.a $(LIBDIR)libpng.a $(LIBDIR)raster.o
SYSLIBS=-lm -lX11

ifeq "$(OS)" "Windows_NT"
	LIBS=$(LIBDIR)libjpeg.win.a $(LIBDIR)libpng.win.a $(LIBDIR)raster.win.o
	SYSLIBS=-lm -lgdi32
	AS=nasm -f coff --prefix _
	MK=mingw32-make
	MKDIR=mkdir
endif

#~ ifeq ($(shell uname -m),x86_64)
	#~ LIBS=$(LIBDIR)libjpeg.x64.a $(LIBDIR)libpng.x64.a
	#~ AS=nasm -f elf64
#~ endif

OBJS=\
	$(OBJDIR)/tree.o\
	$(OBJDIR)/type.o\
	$(OBJDIR)/lexer.o\
	$(OBJDIR)/parser.o\
	$(OBJDIR)/printer.o\
	$(OBJDIR)/code.o\
	$(OBJDIR)/cgen.o\
	$(OBJDIR)/core.o\
	$(OBJDIR)/lstd.o\
	\
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

$(LIBDIR)raster.o: $(GXSRCDIR)/raster.asm
	$(AS) '$<' -o '$@'
$(LIBDIR)raster.win.o: $(GXSRCDIR)/raster.asm
	$(AS) '$<' -o '$@'

$(OBJDIR)/%.o: $(GXSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(CCSRCDIR)' '$<' -o '$@'

$(OBJDIR)/%.o: $(CCSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(CCSRCDIR)' '$<' -o '$@'

