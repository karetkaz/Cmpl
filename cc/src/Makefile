include incl/Makefile.gcc
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
	$(OBJDIR)ccvm.o
	#~ $(OBJDIR)main.o

main: clean $(OBJS) $(OBJDIR)main.o
	$(LNK) $(OUTEXE)$(BINDIR)main $(OBJS) main.o $(LIBS)

lib:$(OBJS)
	$(LIB) $(LIBNAME) $(OBJS)

clean:
	rm -f $(OBJS) $(OBJDIR)main.o

debug:
	$(DBG) '$(BINDIR)main'

#~ build:main clean

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
$(OBJDIR)scan.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)scan.c $(OUTOBJ)$(OBJDIR)scan.o
$(OBJDIR)code.o:
	$(CCC) $(COPTS) $(CFLAGS) $(SRCDIR)code.c $(OUTOBJ)$(OBJDIR)code.o
