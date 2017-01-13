#!/usr/bin/env bash
set -x

GL3WDIR=./lib/gl3w
IMGUIDIR=./lib/imgui
BUILDDIR=./build

rm -R build
mkdir build

g++ -O2 -std=c++11 \
    -I/usr/local/include/SDL2/ \
    -L/usr/local/lib/ \
    -I/usr/local/include/ \
    -I$GL3WDIR -I$IMGUIDIR \
    -I$IMGUIDIR/examples/sdl_opengl3_example \
    -lSDL2 -lSDL2main -framework OpenGL -framework CoreFoundation -lstdc++ -x c++ \
    main.cpp \
    aniplotlib.cpp \
    $IMGUIDIR/examples/sdl_opengl3_example/imgui_impl_sdl_gl3.cpp $IMGUIDIR/imgui.cpp $IMGUIDIR/imgui_demo.cpp $IMGUIDIR/imgui_draw.cpp $GL3WDIR/GL/gl3w.c \
    -o $BUILDDIR/aniplot

rm -R dist
mkdir dist
cp $BUILDDIR/aniplot ./dist/
