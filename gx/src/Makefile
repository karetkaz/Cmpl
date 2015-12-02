CCSRCDIR=../../cc/src/
GXSRCDIR=./
OUTDIR=../out
OBJDIR=../out/obj
BINDIR=../
BINARY=main

ARH=-m32
#~ CFLAGS=-Wall -Wpadded -g0 -Os
#~ CFLAGS=-pipe -g -Wall -Wpadded -g3 -ggdb
CFLAGS=-pipe -g -Wall

CCC=gcc $(ARH)
LNK=gcc $(ARH)
AS=nasm -f elf32
MK=make
MKDIR=mkdir -p
SYSLIBS=-lm -lX11

ifneq "$(ARH)" "-m32"
	LIBDIR=../out/lnx64
else
	LIBDIR=../out/lnx32
endif

ifeq "$(OS)" "Windows_NT"
	ifneq "$(ARH)" "-m32"
		LIBDIR=../out/win64
	else
		LIBDIR=../out/win32
	endif
	SYSLIBS=-lm -lgdi32
	AS=nasm -f coff --prefix _
	MK=mingw32-make
	MKDIR=mkdir
endif

LIBS=$(LIBDIR)/libjpeg.a $(LIBDIR)/libpng.a $(LIBDIR)/raster.o

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

build:$(OBJDIR) $(OBJS)
	$(LNK) -o $(BINDIR)main $(OBJS) $(LIBS) $(SYSLIBS)

libs: $(LIBS)

clean:
	rm -f $(OBJS)

$(OBJDIR):
	$(MKDIR) "$(OBJDIR)"

$(LIBDIR):
	$(MKDIR) "$(LIBDIR)"

$(LIBDIR)/libjpeg.a: $(LIBDIR)
	$(MK) -C ../libs/libjpeg
	mv $(OUTDIR)/libjpeg.a $(LIBDIR)/libjpeg.a

$(LIBDIR)/libpng.a: $(LIBDIR)
	$(MK) -C ../libs/libpng
	mv $(OUTDIR)/libpng.a $(LIBDIR)/libpng.a

$(LIBDIR)/raster.o: $(GXSRCDIR)/raster.asm $(LIBDIR)
	$(AS) '$<' -o '$@'

$(OBJDIR)/%.o: $(GXSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '../libs/libjpeg' -I '../libs/libpng' -I '$(CCSRCDIR)' '$<' -o '$@'

$(OBJDIR)/%.o: $(CCSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(CCSRCDIR)' '$<' -o '$@'

