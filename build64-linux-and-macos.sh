#!/bin/bash

# usage:
#
#     build64-linux-and-macos.sh
#     build64-linux-and-macos.sh example

set -e
set -x

IMGUIDIR=./lib/imgui
BUILDDIR=./build

if [[ "$1" == "example" ]]; then
	MAIN_SRC=aniplot_example.cpp
	BIN=aniplot_example
else
	MAIN_SRC=aniplot.cpp
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
	-I$IMGUIDIR \
	-I$IMGUIDIR/backends \
	$OS_INCLUDES \
	$MAIN_SRC \
	aniplotlib.cpp \
	imgui_unitybuild.cpp \
	$OS_LINKS \
	-o $BUILDDIR/$BIN

rm -Rf dist
mkdir dist
cp $BUILDDIR/$BIN ./dist/
