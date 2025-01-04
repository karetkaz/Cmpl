cmake_minimum_required(VERSION 4.0)
project(CmplOld)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin>)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin>)

set(CC_HEADER_FILES
	include/cmpl.h
	src/code.h
	src/core.h
	src/tree.h
	src/type.h
	src/debug.h
	src/lexer.h
	src/parser.h
	src/printer.h
	src/utils.h
)

set(CC_SOURCE_FILES
	src/tree.c
	src/type.c
	src/typeCheck.c
	src/lexer.c
	src/parser.c

	src/code.c
	src/codeGen.c
	src/codeEmit.c
	src/codeExec.c
	src/codeFold.c

	src/compiler.c
	src/debuger.c
	src/printer.c
	src/util.c

	cmplStd/src/lstd.c
)

set(GX_HEADER_FILES
	cmplGfx/src/gx_surf.h
	cmplGfx/src/gx_color.h
	cmplGfx/src/g3_math.h
	cmplGfx/src/g3_mesh.h
	cmplGfx/src/g3_draw.h
	cmplGfx/src/gx_gui.h
)

set(GX_SOURCE_FILES
	cmplGfx/src/gx_surf.c
	cmplGfx/src/gx_color.c
	cmplGfx/src/gx_gradient.c
	cmplGfx/src/g2_draw.c
	cmplGfx/src/g3_draw.c
	cmplGfx/src/g2_image.c
	cmplGfx/src/g3_mesh.c
	cmplGfx/src/gx_cmpl.c
)

if (WIN32)
	set(CC_SOURCE_FILES ${CC_SOURCE_FILES} src/os_win32/path.c src/os_win32/time.c src/os_win32/plugin.c)
	set(GX_SOURCE_FILES ${GX_SOURCE_FILES} cmplGfx/src/os_win32/gx_gui.w32.c src/os_win32/time.c)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D NO_LIBPNG -D NO_LIBJPEG")
else ()
	set(CC_SOURCE_FILES ${CC_SOURCE_FILES} src/os_linux/path.c src/os_linux/time.c src/os_linux/plugin.c)
	set(GX_SOURCE_FILES ${GX_SOURCE_FILES} cmplGfx/src/os_etc/gx_gui.sdl.c src/os_linux/time.c)
	if (APPLE)
		include_directories(/opt/homebrew/include)
		link_directories(/opt/homebrew/lib)
	endif ()
endif ()

link_libraries(m)
include_directories(./include)

add_executable(Cmpl ${CC_SOURCE_FILES} src/main.c)

add_library(Gfx SHARED ${GX_SOURCE_FILES})
target_include_directories(Gfx PUBLIC ./src/ ./extras/libs/stb)

add_library(File SHARED cmplFile/src/file.c)

if (NOT WIN32)
	target_link_libraries(Cmpl dl)
	target_link_libraries(Gfx dl png jpeg SDL2)
endif ()

# some simple "how to use vm" demos.
add_executable(testVmEmit ${CC_SOURCE_FILES} src/test/vmMulf32.c)
add_executable(testCcLexer ${CC_SOURCE_FILES} src/test/ccLexer.c)

