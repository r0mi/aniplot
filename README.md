aniplot
=======

Hopes to become a fast realtime and non-realtime telemetry graph browser.

Can be run for days (until out of memory) on 1kHz floating point sample frequency and maintains 60 fps while browsing/zooming/moving the graph, independent of how much data is in memory (every sample takes 24 bytes). Maximum and minimum values of the graph at every pixel column are always visible on any zoom level - this way it's easy to spot trends in the data and impossible to miss any spikes, no matter how far zoomed out the graph is.

Drag to move, scroll to zoom horizontally, scroll while holding down ctrl or right mouse button to zoom vertically. Automatically anchors to the right side (automatic horizontal scrolling) if the newest sample becomes visible.

`aniplot.exe` listens for data streams and graph layout/style commands on UDP port 59100. To test, run `tools/udp-stream-test*.py`, and you should see this:

![](doc/aniplot-pic1.png)

`aniplot_example.exe` is a simpler example on how to use/embed the graph widget in imgui programs.

Build aniplot.exe and aniplot_example.exe
-----------------------------------------

### MacOS (tested on Sierra and Mojave (needs to be Mojave from 2019-04 or newer))

    brew install sdl2
    ./build64-linux-and-macos.sh
    ./build64-linux-and-macos.sh example

### Linux (tested on Debian stretch)

    apt-get install libsdl2-dev
    ./build64-linux-and-macos.sh
    ./build64-linux-and-macos.sh example

### Windows

Method 1: Install Visual Studio (2013..2022), and run:

    (edit the files to select the installed Visual Studio version)

    build64-vc.bat
    build64-vc-example.bat

Method 2: Install Visual Studio 2017 or later and use CMake support:

    File -> Open -> Folder...  select the aniplot folder.

Credits/dependencies
--------------------

All dependencies are also included in the lib folder.

  * [https://github.com/ocornut/imgui](https://github.com/ocornut/imgui) the fabulous "dear imgui"
  * [https://www.libsdl.org/](https://www.libsdl.org/) Simple DirectMedia Layer (SDL2)
