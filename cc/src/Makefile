OUTOBJ=-o
OUTEXE=-o
CFLAGS=-Wall -Wpadded -g3 -ggdb
CFLAGS=-g0 -Os
LIBS=-lm -ldl
CCC=gcc -m32 -c
LNK=gcc -m32

ifeq "$(OS)" "Windows_NT"
LIBS=-lm
endif

#~ OUTOBJ=-fo=
#~ OUTEXE=-fe=
#~ CFLAGS=-ei -6s -d2
#~ LIBS=
#~ CCC=wcc386 -q
#~ LNK=wcl386 -q

#~ OUTOBJ=-o
#~ OUTEXE=-o
#~ CFLAGS=
#~ LIBS=-lm -ldl
#~ CCC=tcc -c
#~ LNK=tcc

SRCDIR=./
OBJDIR=../obj/
BINDIR=../
#~ OBJDIR=../Debug/src/
#~ BINDIR=../Debug/

OBJS=\
	$(OBJDIR)code.o\
	$(OBJDIR)parser.o\
	$(OBJDIR)printer.o\
	$(OBJDIR)tree.o\
	$(OBJDIR)type.o\
	$(OBJDIR)core.o\
	$(OBJDIR)lstd.o\
	$(OBJDIR)main.o

#~ rebuild:clean build

build:$(OBJDIR) $(OBJS) main.c
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) $(LIBS)

lib:$(OBJS)
	$(LIB) $(OBJS)

clean:
	rm -f $(OBJS) $(OBJDIR)main.o
#	rm -f *.o *.obj *.err

$(OBJDIR):
	mkdir "$(OBJDIR)"

$(OBJDIR)%.o: $(SRCDIR)%.c
	$(CCC) $(CFLAGS) $< $(OUTOBJ)$@
