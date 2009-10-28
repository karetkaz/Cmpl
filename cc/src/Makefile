INCLUDE=
OUTOBJ=-o
OUTEXE=-o
CFLAGS=-O0 -ggdb -g3
COPTS=-c -Wall
LIBS=-lm
CCC=gcc -m32
LNK=gcc -m32

INCLUDE='$(WATCOM)\h;$(WATCOM)\h\nt'
OUTOBJ=-fo=
OUTEXE=-fe=
CFLAGS=-zq -ei -6s -d0 -oa
# -w99 -d3 -6s -oa -hd
COPTS=-i=$(INCLUDE)
LIBS=
LNK=wcl386 $(CFLAGS) 
CCC=wcc386
LIB=WLIB -q

SRCDIR=
OBJDIR=
#~ ../tmp/obj/
#~ ../tmp/
BINDIR=../
OBJS=\
	$(OBJDIR)clog.o\
	$(OBJDIR)scan.o\
	$(OBJDIR)tree.o\
	$(OBJDIR)type.o\
	$(OBJDIR)code.o\
	$(OBJDIR)pvmc.o
	#~ $(OBJDIR)main.o

main: clean $(OBJS) $(OBJDIR)main.o
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) main.o $(LIBS)

lib:$(OBJS)
	$(LIB) libpvmc.lib $(OBJS)

$(OBJDIR)main.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)main.c $(OUTOBJ)$(OBJDIR)main.o
$(OBJDIR)pvmc.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)pvmc.c $(OUTOBJ)$(OBJDIR)pvmc.o
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
	rm -f $(OBJS) $(OBJDIR)main.o
#~ rm -f $(OBJDIR)main.o

debug:
	#~ xterm -e gdb $(BINDIR)main
	#~ gdb $(BINDIR)main
	wdw $(BINDIR)main

build:clean main

