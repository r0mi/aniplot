#!/bin/bash
set -e
set -x

GL3WDIR=./lib/gl3w
IMGUIDIR=./lib/imgui
BUILDDIR=./build

if [[ "$1" == "example" ]]; then
	MAIN_SRC=aniplot_example.cpp
	BIN=aniplot_example
else
	MAIN_SRC=main.cpp
	BIN=aniplot
fi

if [[ "$(uname)" == "Linux" ]]; then
	if ! pkg-config sdl2 --exists; then
		echo "install libsdl2-dev";
		exit
	else
		OS_INCLUDES="$(pkg-config sdl2 --cflags)"
		OS_LINKS="-ldl -lGL $(pkg-config sdl2 --libs)"
	fi
fi

if [[ "$(uname)" == "Darwin" ]]; then
	OS_INCLUDES="-I/usr/local/include/SDL2/ -I/usr/local/include/"
	OS_LINKS="-L/usr/local/lib/ -lSDL2 -lSDL2main -framework OpenGL -framework CoreFoundation -lstdc++ -x c++"
fi

rm -Rf $BUILDDIR
mkdir $BUILDDIR

g++ -O2 -std=gnu++11 \
	-I$GL3WDIR -I$IMGUIDIR \
	-I$IMGUIDIR/examples/sdl_opengl3_example \
	$OS_INCLUDES \
	$MAIN_SRC \
	aniplotlib.cpp \
	$IMGUIDIR/examples/sdl_opengl3_example/imgui_impl_sdl_gl3.cpp $IMGUIDIR/imgui.cpp $IMGUIDIR/imgui_demo.cpp $IMGUIDIR/imgui_draw.cpp $GL3WDIR/GL/gl3w.c \
	$OS_LINKS \
	-o $BUILDDIR/$BIN

rm -Rf dist
mkdir dist
cp $BUILDDIR/$BIN ./dist/
