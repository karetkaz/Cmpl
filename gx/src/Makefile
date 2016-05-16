CCSRCDIR=../../cc/src/
GXSRCDIR=./
OUTDIR=../out
OBJDIR=../out/obj
LIBDIR=../libs
BINDIR=../
BINARY=main

ARH=-m32
#~ CFLAGS=-Wall -Wpadded -g0 -Os
#~ CFLAGS=-pipe -g -Wall -Wpadded -g3 -ggdb
CFLAGS=-pipe -g -Wall

CCC=gcc $(ARH)
LNK=gcc $(ARH)

ifneq "$(OS)" "Windows_NT"
	ifneq "$(ARH)" "-m32"
		ARHDIR=lnx64
	else
		ARHDIR=lnx32
	endif

	MK=make
	MKDIR=mkdir -p
	SYSLIBS=-lm -lX11
else
	ifneq "$(ARH)" "-m32"
		ARHDIR=win64
	else
		ARHDIR=win32
	endif

	MK=mingw32-make
	MKDIR=mkdir
	SYSLIBS=-lm -lgdi32
endif

LIBS=$(LIBDIR)/$(ARHDIR)/libjpeg.a $(LIBDIR)/$(ARHDIR)/libpng.a

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
	$(OBJDIR)/g2_draw.o\
	$(OBJDIR)/g2_surf.o\
	$(OBJDIR)/g3_draw.o\
	$(OBJDIR)/g3_mesh.o\
	$(OBJDIR)/gx_script.o\
	$(OBJDIR)/gx_image.o\
	$(OBJDIR)/gx_raster.o\
	$(OBJDIR)/gx_platform.o\
	$(OBJDIR)/main.o

#~ rebuild:clean build

#~ build:$(OBJDIR) $(OBJS) $(LIBS)
build:$(OBJDIR) $(OBJS)
	$(LNK) -o $(BINDIR)main $(OBJS) $(LIBS) $(SYSLIBS)

libs: $(LIBS)

clean:
	rm -f $(OBJS)

$(OBJDIR):
	$(MKDIR) "$(OBJDIR)"

$(LIBDIR)/$(ARHDIR):
	$(MKDIR) "$(LIBDIR)/$(ARHDIR)"

$(LIBDIR)/$(ARHDIR)/libjpeg.a: $(LIBDIR)/$(ARHDIR)
	$(MK) -C $(LIBDIR)/libjpeg
	mv $(OUTDIR)/libjpeg.a $(LIBDIR)/$(ARHDIR)/libjpeg.a

$(LIBDIR)/$(ARHDIR)/libpng.a: $(LIBDIR)/$(ARHDIR)
	$(MK) -C $(LIBDIR)/libpng
	mv $(OUTDIR)/libpng.a $(LIBDIR)/$(ARHDIR)/libpng.a

$(OBJDIR)/%.o: $(GXSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(LIBDIR)/libjpeg' -I '$(LIBDIR)/libpng' -I '$(CCSRCDIR)' '$<' -o '$@'

$(OBJDIR)/%.o: $(CCSRCDIR)%.c
	$(CCC) -c $(CFLAGS) -I '$(CCSRCDIR)' '$<' -o '$@'
