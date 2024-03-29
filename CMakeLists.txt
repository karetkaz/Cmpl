cmake_minimum_required(VERSION 3.3)
project(Cmpl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin>)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bin>)

set(CC_HEADER_FILES
	src/cmpl.h
	src/cmplVm.h
	src/cmplCc.h
	src/cmplDbg.h
	src/utils.h
	src/parser.h
	src/printer.h
	src/internal.h
)

set(CC_SOURCE_FILES
	src/cgen.c
	src/code.c
	src/core.c
	src/lexer.c
	src/parser.c
	src/platform.c
	src/printer.c
	src/tree.c
	src/type.c
	src/utils.c
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

add_executable(cmpl ${CC_SOURCE_FILES} src/main.c)

add_library(File SHARED cmplFile/src/file.c)
target_include_directories(File PUBLIC ./src/)

if (WIN32)
	add_library(Gfx SHARED ${GX_SOURCE_FILES} cmplGfx/src/os_win32/gx_gui.w32.c src/os_win32/time.c)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D NO_LIBPNG -D NO_LIBJPEG")
else ()
	if (APPLE)
		include_directories(/usr/local/include)
		link_directories(/usr/local/lib)
	else ()
		add_library(OpenGL SHARED cmplGL/src/openGL.c)
		target_include_directories(OpenGL PUBLIC ./src/)
		target_link_libraries(OpenGL GL GLU glut)
	endif ()
#	add_library(Gfx SHARED ${GX_SOURCE_FILES} cmplGfx/src/os_linux/gx_gui.x11.c src/os_linux/time.c)
#	target_link_libraries(Gfx m dl png jpeg X11)
	add_library(Gfx SHARED ${GX_SOURCE_FILES} cmplGfx/src/gx_gui.sdl.c src/os_linux/time.c)
	target_link_libraries(Gfx m dl png jpeg SDL2)
	target_link_libraries(cmpl m dl)
endif ()
target_include_directories(Gfx PUBLIC ./src/ ./libs/stb)

# a simple "how to use vm" demo code.
#add_executable(cmplVmEmit ${CC_SOURCE_FILES} src/test/vmMulf32.c)
#target_include_directories(cmplVmEmit PUBLIC ./src/)
#target_link_libraries(cmplVmEmit m dl)
