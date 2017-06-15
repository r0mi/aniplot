aniplot
=======

Hopes to become a fast realtime and non-realtime telemetry graph browser. Can be run for days (until out of memory) on 1kHz floating point sample frequency and maintains 60 fps while browsing/zooming/moving the graph, independent of how much data is in memory (every sample takes 24 bytes).

`aniplot.exe` listens for data streams and graph layout/style commands on UDP port 59100. To test, run `tools/udp-stream-test*.py`, and you should see this:

![](doc/aniplot-pic1.png)

`aniplot_example.exe` is a simple example on how to use the graph widget.

Build aniplot.exe
-----------------

### MacOS (tested on Sierra)

    brew install sdl2
    ./build64-linux-and-macos.sh

### Linux (tested on Debian stretch)

    apt-get install libsdl2-dev
    ./build64-linux-and-macos.sh

### Windows

Install Visual Studio 2013, 2014 or 2017 and run one of:

    build64-vc12-2013.bat
    build64-vc14-2015.bat
    build64-vc15-2017.bat

Build aniplot_example.exe
-------------------------

### MacOS (tested on Sierra)

    brew install sdl2
    ./build64-linux-and-macos.sh example

### Linux (tested on Debian stretch)

    apt-get install libsdl2-dev
    ./build64-linux-and-macos.sh example

Credits/dependencies
--------------------

All dependencies are also included in the lib folder.

  * [https://github.com/ocornut/imgui](https://github.com/ocornut/imgui) the fabulous "dear imgui"
  * [https://github.com/skaslev/gl3w](https://github.com/skaslev/gl3w) gl3w: Simple OpenGL core profile loading
  * [https://www.libsdl.org/](https://www.libsdl.org/) Simple DirectMedia Layer (SDL2)
