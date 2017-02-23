# aniplot

Hopes to become a fast realtime and non-realtime telemetry graphs browser.

# build

MacOS (tested on Sierra):
```
brew install sdl2
./build64-linux-and-macos.sh

```
Linux (tested on Debian stretch):
```
apt-get install libsdl2-dev
./build64-linux-and-macos.sh
```
Windows:
```
build64-vc12.bat
or
build64-vc14.bat
```

# build example

MacOS (tested on Sierra):
```
brew install sdl2
./build64-linux-and-macos.sh example

```
Linux (tested on Debian stretch):
```
apt-get install libsdl2-dev
./build64-linux-and-macos.sh example
```


# credits/dependencies

All dependencies are also included in the lib folder.

  * [https://github.com/ocornut/imgui](https://github.com/ocornut/imgui) the fabulous "dear imgui"
  * [https://github.com/skaslev/gl3w](https://github.com/skaslev/gl3w) gl3w: Simple OpenGL core profile loading
  * [https://www.libsdl.org/](https://www.libsdl.org/) Simple DirectMedia Layer (SDL2)
