BINDIR?=.build
CC_SRC=src
GX_SRC=cmplGfx/src

CFLAGS=-Wall -Wextra -g0 -O3 -std=gnu99
EMFLAGS=-g0 -O3 -s WASM=1 -s EXPORT_ALL=0 -s INVOKE_RUN=0 -s ASSERTIONS=0
EMFLAGS+=--no-heap-copy -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_MEMORY=64MB -s STACK_SIZE=8MB
EMFLAGS+=-D NO_LIBJPEG -D NO_LIBPNG

EM_EMBED='--preload-file' 'cmplStd/lib.cmpl' '--preload-file' 'cmplGfx/lib.cmpl' '--preload-file' 'cmplFile/lib.cmpl'
EM_EMBED+=$(shell find cmplStd/lib -type f -name '*.cmpl' -not -path '*/todo/*' -exec echo '--preload-file' {} \;)
EM_EMBED+=$(shell find cmplGfx/lib -type f -name '*.cmpl' -not -path '*/todo/*' -exec echo '--preload-file' {} \;)
EM_EMBED+=$(shell find cmplFile/lib -type f -name '*.cmpl' -not -path '*/todo/*' -exec echo '--preload-file' {} \;)
EM_SIDE_MODULE=-s SIDE_MODULE=2 -s "EXPORTED_FUNCTIONS=['_cmplInit', '_cmplUnit']"
EM_MAIN_MODULE=-s MAIN_MODULE=1 -lidbfs.js

CLIBS=-lm
GXLIBS=-lm
CFLAGS+=-I extras/libs/stb -I include
EMFLAGS+=-I extras/libs/stb -I include

SRC_CC=\
	$(CC_SRC)/*.h\
	$(CC_SRC)/code.c\
	$(CC_SRC)/codeGen.c\
	$(CC_SRC)/codeEmit.c\
	$(CC_SRC)/codeExec.c\
	$(CC_SRC)/codeFold.c\
	$(CC_SRC)/tree.c\
	$(CC_SRC)/type.c\
	$(CC_SRC)/typeCheck.c\
	$(CC_SRC)/lexer.c\
	$(CC_SRC)/parser.c\
	$(CC_SRC)/compiler.c\
	$(CC_SRC)/debuger.c\
	$(CC_SRC)/printer.c\
	$(CC_SRC)/util.c\
	cmplStd/src/lstd.c

SRC_GX=\
	$(GX_SRC)/*.h\
	$(GX_SRC)/gx_surf.c\
	$(GX_SRC)/gx_color.c\
	$(GX_SRC)/gx_gradient.c\
	$(GX_SRC)/g2_draw.c\
	$(GX_SRC)/g3_draw.c\
	$(GX_SRC)/g2_image.c\
	$(GX_SRC)/g3_mesh.c\
	$(GX_SRC)/gx_cmpl.c

SRC_GX_X11=$(SRC_GX) $(GX_SRC)/os_linux/gx_gui.x11.c $(CC_SRC)/os_linux/time.c
SRC_GX_W32=$(SRC_GX) $(GX_SRC)/os_win32/gx_gui.w32.c $(CC_SRC)/os_win32/time.c
SRC_GX_SDL=$(SRC_GX) $(GX_SRC)/os_etc/gx_gui.sdl.c $(CC_SRC)/os_linux/time.c

ifeq "$(OS)" "Windows_NT"
	SRC_CC_EXE=$(SRC_CC)\
		$(CC_SRC)/os_win32/time.c\
		$(CC_SRC)/os_win32/path.c\
		$(CC_SRC)/os_win32/plugin.c\
		$(CC_SRC)/main.c

	CFLAGS+=-D NO_LIBJPEG
	CFLAGS+=-D NO_LIBPNG
else
	SRC_CC_EXE=$(SRC_CC)\
		$(CC_SRC)/os_linux/time.c\
		$(CC_SRC)/os_linux/path.c\
		$(CC_SRC)/os_linux/plugin.c\
		$(CC_SRC)/main.c

	GXLIBS+=-lpng -ljpeg
	CFLAGS+=-fPIC
	CLIBS+=-ldl
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		MKDIRF=--parents
	endif
	ifeq ($(UNAME_S),Darwin)
		CFLAGS+=-I /opt/homebrew/include
		GXLIBS+=-L /opt/homebrew/lib
		MKDIRF=-p
	endif
endif

cmpl: $(addprefix $(BINDIR)/, $(filter %.o, $(SRC_CC_EXE:%.c=%.o)))
	gcc -o $(BINDIR)/cmpl $^ $(CLIBS)

clean:
	rm -Rf $(BINDIR)

# Linux platform
libGfx.so: $(addprefix $(BINDIR)/, $(filter %.o, $(SRC_GX_X11:%.c=%.o)))
	gcc -fPIC -shared $(CFLAGS) -I src -o $(BINDIR)/libGfx.so $^ $(GXLIBS) -lX11

libGfxSdl.so: $(addprefix $(BINDIR)/, $(filter %.o, $(SRC_GX_SDL:%.c=%.o)))
	gcc -fPIC -shared $(CFLAGS) -I src -o $(BINDIR)/libGfx.so $^ $(GXLIBS) -lSDL2

libFile.so: cmplFile/src/file.c
	gcc -fPIC -shared $(CFLAGS) -I src -o $(BINDIR)/libFile.so $^


# Macos platform
libGfx11.dylib: $(addprefix $(BINDIR)/, $(filter %.o, $(SRC_GX_X11:%.c=%.o)))
	gcc -fPIC -shared $(CFLAGS) -I src -o $(BINDIR)/libGfx.dylib $^ $(GXLIBS) -lX11

libGfx.dylib: $(addprefix $(BINDIR)/, $(filter %.o, $(SRC_GX_SDL:%.c=%.o)))
	gcc -fPIC -shared $(CFLAGS) -I src -o $(BINDIR)/libGfx.dylib $^ $(GXLIBS) -lSDL2

libFile.dylib: cmplFile/src/file.c
	gcc -fPIC -shared $(CFLAGS) -I src -o $(BINDIR)/libFile.dylib $^


# Windows platform
libGfx.dll: $(addprefix $(BINDIR)/, $(filter %.o, $(SRC_GX_W32:%.c=%.o)))
	gcc -shared $(CFLAGS) -I src -o $(BINDIR)/libGfx.dll $^ $(GXLIBS) -lgdi32

libFile.dll: cmplFile/src/file.c
	gcc -shared $(CFLAGS) -I src -o $(BINDIR)/libFile.dll $^


# Webassembly platform
cmpl.js: $(SRC_CC_EXE)
	@mkdir $(MKDIRF) "$(BINDIR)" || true
	emcc $(EMFLAGS) -o $(BINDIR)/cmpl.js $(EM_MAIN_MODULE) -s USE_SDL=2 $(filter %.c, $^) $(EM_EMBED)

libFile.wasm: cmplFile/src/file.c
	@mkdir $(MKDIRF) "$(BINDIR)" || true
	emcc $(EMFLAGS) -o $(BINDIR)/libFile.wasm -I src $(EM_SIDE_MODULE) $(filter %.c, $^)

libGfx.wasm: $(SRC_GX) $(GX_SRC)/os_etc/gx_gui.sdl.c $(CC_SRC)/os_linux/time.c
	@mkdir $(MKDIRF) "$(BINDIR)" || true
	emcc $(EMFLAGS) -o $(BINDIR)/libGfx.wasm -I src $(EM_SIDE_MODULE) $(filter %.c, $^)

cmpl.dbg.js: $(SRC_CC_EXE)
	@mkdir $(MKDIRF) "$(BINDIR)" || true
	emcc -g3 -O0 -s WASM=0 $(filter %.c, $^) -o $(BINDIR)/cmpl.dbg.js


$(BINDIR)/$(CC_SRC)/%.o: $(CC_SRC)/%.c $(filter-out %.c, $(SRC_CC_EXE))
	@mkdir $(MKDIRF) "$(@D)" || true
	gcc $(CFLAGS) -o "$@" -c "$<"

$(BINDIR)/$(CC_SRC)/runtime/%.o: $(CC_SRC)/%.c $(filter-out %.c, $(SRC_CC_EXE))
	@mkdir $(MKDIRF) "$(@D)" || true
	gcc $(CFLAGS) -o "$@" -c "$<"

$(BINDIR)/$(CC_SRC)/compiler/%.o: $(CC_SRC)/%.c $(filter-out %.c, $(SRC_CC_EXE))
	@mkdir $(MKDIRF) "$(@D)" || true
	gcc $(CFLAGS) -o "$@" -c "$<"

$(BINDIR)/$(GX_SRC)/%.o: $(GX_SRC)/%.c $(filter-out %.c, $(SRC_CC_EXE))
	@mkdir $(MKDIRF) "$(@D)" || true
	gcc $(CFLAGS) -I src -o "$@" -c "$<"

$(BINDIR)/cmplStd/src/%.o: cmplStd/src/%.c $(filter-out %.c, $(SRC_CC_EXE))
	@mkdir $(MKDIRF) "$(@D)" || true
	gcc $(CFLAGS) -o "$@" -c "$<"
