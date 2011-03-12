OUTOBJ=-o
OUTEXE=-o
CFLAGS=-g3 -ggdb
#~ CFLAGS=-g0 -O2
COPTS=-c -Wall -Wpadded
LIBS=-lm
CCC=gcc -m32
LNK=gcc -m32
#~ -g3 -ggdb
LIB=ar rcs ../../gx/lib/libpvmc/libpvmc.linux.a
DBG=nemiver

ifeq "$(OS)" "Windows_NT"
LIB=ar rcs ../../gx/lib/libpvmc/libpvmc.windows.a
#~ LIB=ar rcs ../../lib/libpvmc/libpvmc.windows.a
DBG=
endif

#~ include incl/Makefile.wcc

SRCDIR=
OBJDIR=
BINDIR=../
#~ OBJDIR=..\Debug\src\\
#~ BINDIR=..\Debug\\
#~ OBJDIR=../Debug/src/
#~ BINDIR=../Debug/
OBJS=\
	$(OBJDIR)code.o\
	$(OBJDIR)clog.o\
	$(OBJDIR)parse.o\
	$(OBJDIR)tree.o\
	$(OBJDIR)type.o\
	$(OBJDIR)ccvm.o

#~ all:build

rebuild:clean build

build:$(OBJS) $(OBJDIR)main.o
	$(LNK) $(OUTEXE)$(BINDIR)ccvm $(OBJS) $(OBJDIR)main.o $(LIBS)

lib:$(OBJS)
	$(LIB) $(OBJS)

clean:
	rm -f $(OBJS) $(OBJDIR)main.o
	rm -f *.obj

debug:
	$(DBG) "$(BINDIR)main"

$(OBJDIR)main.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)main.c $(OUTOBJ)$(OBJDIR)main.o
$(OBJDIR)ccvm.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)ccvm.c $(OUTOBJ)$(OBJDIR)ccvm.o
$(OBJDIR)clog.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)clog.c $(OUTOBJ)$(OBJDIR)clog.o
$(OBJDIR)buff.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)buff.c $(OUTOBJ)$(OBJDIR)buff.o
$(OBJDIR)tree.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)tree.c $(OUTOBJ)$(OBJDIR)tree.o
$(OBJDIR)type.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)type.c $(OUTOBJ)$(OBJDIR)type.o
$(OBJDIR)clex.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)clex.c $(OUTOBJ)$(OBJDIR)clex.o
$(OBJDIR)parse.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)parse.c $(OUTOBJ)$(OBJDIR)parse.o
$(OBJDIR)code.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)code.c $(OUTOBJ)$(OBJDIR)code.o
