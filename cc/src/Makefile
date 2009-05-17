SRCDIR = .
OBJDIR = temp
BINDIR = .

#~ OBJS=	$(OBJDIR)/main.o
OBJS=\
	$(OBJDIR)/clog.o\
	$(OBJDIR)/clex.o\
	$(OBJDIR)/scan.o\
	$(OBJDIR)/tree.o\
	$(OBJDIR)/type.o\
	$(OBJDIR)/main.o

include incl/Makefile.gcc
#~ include incl/Makefile.wcc

#~ run:$(BINDIR)/main
	#~ $(BINDIR)/main

clean:
	rm -f $(OBJS) $(BINDIR)/main
	#~ del *.o

rebuild:clean main
