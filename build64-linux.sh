#!/bin/sh

set -x

GL3WDIR=./lib/gl3w
IMGUIDIR=./lib/imgui
BUILDDIR=./build

if ! pkg-config sdl2 --exists; then
    echo "install libsdl2-dev";
else
    rm -Rf build
    mkdir build
    g++ -O2 \
        -I$GL3WDIR -I$IMGUIDIR \
        -I$IMGUIDIR/examples/sdl_opengl3_example \
        $(pkg-config sdl2 --cflags) \
        -std=gnu++11 \
        main.cpp \
        aniplotlib.cpp \
        ./lib/imgui/examples/sdl_opengl3_example/imgui_impl_sdl_gl3.cpp ./lib/imgui/imgui.cpp ./lib/imgui/imgui_demo.cpp ./lib/imgui/imgui_draw.cpp ./lib/gl3w/GL/gl3w.c \
        -ldl -lGL \
        $(pkg-config sdl2 --libs ) \
        -o $BUILDDIR/aniplot

    rm -Rf dist
    mkdir dist
    cp $BUILDDIR/aniplot ./dist/
fi
