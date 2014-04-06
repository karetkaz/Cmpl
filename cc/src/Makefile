CFLAGS=-Wall -Wpadded -g3 -ggdb
CFLAGS=-Wall -Wpadded -g0 -Os
ARH=-m32
LIBS=-lm -ldl
MKDIR=mkdir -p

ifeq "$(OS)" "Windows_NT"
	LIBS=-lm
	MKDIR=mkdir
endif

SRCDIR=./
OBJDIR=../out/obj
BINDIR=../

OBJS=\
	$(OBJDIR)/tree.o\
	$(OBJDIR)/type.o\
	$(OBJDIR)/parser.o\
	$(OBJDIR)/printer.o\
	$(OBJDIR)/lstd.o\
	$(OBJDIR)/code.o\
	$(OBJDIR)/cgen.o\
	$(OBJDIR)/core.o\
	$(OBJDIR)/main.o

build:$(OBJDIR) $(OBJS)
	gcc $(ARH) -o $(BINDIR)main $(OBJS) $(LIBS)

$(OBJDIR):
	$(MKDIR) "$(OBJDIR)"

$(OBJDIR)/%.o: $(SRCDIR)%.c $(SRCDIR)*.h $(SRCDIR)*.inl
	gcc $(ARH) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(OBJDIR)main.o
