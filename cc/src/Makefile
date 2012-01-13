OUTOBJ=-o
OUTEXE=-o
CFLAGS=-g2 -ggdb
CFLAGS=-g0 -Os
LIBS=-lm -ldl
CCC=gcc -m32
LNK=gcc -m32

ifeq "$(OS)" "Windows_NT"
LIBS=-lm
endif

SRCDIR=./
OBJDIR=../../obj/
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
	$(OBJDIR)lstd.o\
	$(OBJDIR)main.o

#~ rebuild:clean build

build:$(OBJS) main.c
	$(LNK) $(CFLAGS) $(OUTEXE)$(BINDIR)main $(OBJS) $(LIBS)

lib:$(OBJS)
	$(LIB) $(OBJS)

clean:
	rm -f $(OBJS) $(OBJDIR)main.o
#	rm -f *.o *.obj *.err

$(OBJDIR)%.o: $(SRCDIR)%.c
	$(CCC) $(CFLAGS) -c -Wall -Wpadded $< -o $@
