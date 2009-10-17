INCLUDE=
OUTOBJ=-o
OUTEXE=-o
CFLAGS=-O0 -ggdb -g3
COPTS=-c -Wall
LIBS=-lm
CCC=gcc -m32
LNK=gcc -m32

#~ INCLUDE='$(WATCOM)\h;$(WATCOM)\h\nt'
#~ OUTOBJ=-fo=
#~ OUTEXE=-fe=
#~ CFLAGS=-q -d3
#~ # -w99 -6s -oa
#~ COPTS=-i=$(INCLUDE)
#~ LIBS=
#~ LNK=wcl386 -q -d3
#~ CCC=wcc386

SRCDIR=
OBJDIR=../tmp/obj/
#~ ../tmp/
BINDIR=../
OBJS=\
	$(OBJDIR)clog.o\
	$(OBJDIR)scan.o\
	$(OBJDIR)tree.o\
	$(OBJDIR)type.o\
	$(OBJDIR)code.o\
	$(OBJDIR)main.o

main: $(OBJS)
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) $(LIBS)

$(OBJDIR)objfw.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)objfw.c $(OUTOBJ)$(OBJDIR)objfw.o

$(OBJDIR)main.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)main.c $(OUTOBJ)$(OBJDIR)main.o
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
$(OBJDIR)scan.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)scan.c $(OUTOBJ)$(OBJDIR)scan.o
$(OBJDIR)code.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)code.c $(OUTOBJ)$(OBJDIR)code.o

clean:
	rm -f $(OBJS)

debug:
	#~ xterm -e gdb main
	#~ gdb main
	wdw main

build:clean main

