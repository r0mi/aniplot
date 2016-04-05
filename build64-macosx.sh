set -x

GL3WDIR=./lib/gl3w
IMGUIDIR=./lib/imgui
BUILDDIR=./build

rm -R build
mkdir build

g++ -O2 \
    -I/usr/local/include/SDL2/ \
    -I/usr/local/include/ \
    -I$GL3WDIR -I$IMGUIDIR \
    -I$IMGUIDIR/examples/sdl_opengl3_example \
    -lSDL2 -lSDL2main -framework OpenGL -framework CoreFoundation -lstdc++ \
    main.cpp \
    ./lib/imgui/examples/sdl_opengl3_example/imgui_impl_sdl_gl3.cpp ./lib/imgui/imgui.cpp ./lib/imgui/imgui_demo.cpp ./lib/imgui/imgui_draw.cpp ./lib/gl3w/GL/gl3w.c \
    -o $BUILDDIR/aniplot

rm -R dist
mkdir dist
cp $BUILDDIR/aniplot ./dist/
