OUTOBJ=-o
OUTEXE=-o
CFLAGS=-Wall -Wpadded -g3 -ggdb
CFLAGS=-Wall -Wpadded -g0 -Os
LIBS=-lm -ldl
LNK=gcc -m32
CCC=gcc -m32 -c

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

build:$(OBJS) main.c
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(OBJDIR)main.o

lib:$(OBJS)
	$(LIB) $(OBJS)

$(OBJDIR)%.o: $(SRCDIR)%.c $(SRCDIR)*.h $(SRCDIR)*.inl
	$(CCC) $(CFLAGS) $< $(OUTOBJ)$@


end:
#~ $(OBJDIR)%.o: $(SRCDIR)%.c $(SRCDIR)api.h $(SRCDIR)pvmc.h $(SRCDIR)core.h
	#~ $(CCC) $(CFLAGS) $< $(OUTOBJ)$@
