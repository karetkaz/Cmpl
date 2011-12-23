OUTOBJ=-o
OUTEXE=-o
CFLAGS=-g2 -ggdb
#~ CFLAGS=-g0 -O2
#~ CFLAGS=-g2
LIBS=-lm -ldl
CCC=gcc -m32
LNK=gcc -m32
LIB=ar rcs ../../gx/lib/libpvmc/libpvmc.linux.a
LIBGL=-lGL -lglut
DBG=nemiver

ifeq "$(OS)" "Windows_NT"
LIB=ar rcs ../../gx/lib/libpvmc/libpvmc.windows.a
LIBGL=-lglut32 -lopengl32
#~ LIB=ar rcs ../../lib/libpvmc/libpvmc.windows.a
LIBS=-lm
DBG=
endif

SRCDIR=
OBJDIR=
BINDIR=../
#~ OBJDIR=../Debug/src/
#~ BINDIR=../Debug/

OBJS=\
	$(OBJDIR)code.o\
	$(OBJDIR)clog.o\
	$(OBJDIR)parse.o\
	$(OBJDIR)tree.o\
	$(OBJDIR)type.o\
	$(OBJDIR)ccvm.o\
	$(OBJDIR)lstd.o

#~ rebuild:clean build

#~ buildgl:$(OBJS) gl.c
#~ 	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) gl.c $(LIBS) $(LIBGL)

build:$(OBJS) main.c
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) main.c $(LIBS)

lib:$(OBJS)
	$(LIB) $(OBJS)

clean:
	rm -f $(OBJS) $(OBJDIR)main.o
	rm -f *.obj

debug:
	$(DBG) "$(BINDIR)main"

$(OBJDIR)%.o: $(SRCDIR)%.c
	$(CCC) -c $(CFLAGS) -Wall -Wpadded $< -o $@
