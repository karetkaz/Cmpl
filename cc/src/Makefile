SRCDIR = .
OBJDIR = temp
BINDIR = .

#~ clog.c : logger & output
#~ clex.c : lexer & input
#~ scan.c : scanner
#~ tree.c : 
#~ type.c : 
#~ code.c : virtual machine
#~ main.c : main program

OBJS=	$(OBJDIR)/clog.o\
		$(OBJDIR)/clex.o\
		$(OBJDIR)/scan.o\
		$(OBJDIR)/tree.o\
		$(OBJDIR)/type.o\
		$(OBJDIR)/main.o


include incl/Makefile.gcc
#include incl/Makefile.wcc

run:$(BINDIR)/main
	$(BINDIR)/main

clean:
	rm -f $(OBJS) $(BINDIR)/main
	rm -f *.err *.o *.obj
