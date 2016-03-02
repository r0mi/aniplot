:: this builds the debug version?

@echo off

::
:: https://msdn.microsoft.com/en-us/library/fwkeyyhe.aspx : compiler options
:: https://msdn.microsoft.com/en-us/library/y0zzbyt4.aspx : linker options
:: https://msdn.microsoft.com/en-us/library/f1cb223a.aspx : Output-File (/F) Options
::
:: /FC : Causes the compiler to display the full path of source code files passed to the compiler in diagnostics.
:: /Zi : Select the type of debugging information created for your program and whether this information is kept in object (.obj) files or in a program database (PDB).
::       Produces a program database (PDB) that contains type information and symbolic debugging information for use with the debugger.
:: /MD : Causes the application to use the multithread-specific and DLL-specific version of the run-time library. Defines _MT and _DLL and causes the compiler to place the library name MSVCRT.lib into the .obj file.
::

:: TODO: how should i have known to use /MD? SDL2 has written nowhere how it was compiled.
:: well whadoyouknow! ending even a fucking COMMENT with a caret does something weird

::cl /FC /Zi /MD
::cl /FC /Zi /Ox /MD

:: https://www.libsdl.org/release/SDL2-devel-2.0.4-VC.zip
:: https://sourceforge.net/projects/glew/files/glew/1.13.0/glew-1.13.0-win32.zip/download


set SDL2DIR=.\lib\SDL2-devel-2.0.4-VC
set GL3WDIR=.\lib\gl3w
set IMGUIDIR=.\lib\imgui
set BUILDDIR=.\build

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

rmdir build
mkdir build

rem    /I %IMGUIDIR%\examples\sdl_opengl3_example ^
rem    .\imgui_impl_sdl_gl3.cpp ^

cl /FC /Zi /MD /Fo%BUILDDIR%\ /Fd%BUILDDIR%\ /Fm%BUILDDIR%\ /Fe%BUILDDIR%\aniplot.exe ^
    main.cpp ^
    .\lib\imgui\examples\sdl_opengl3_example\imgui_impl_sdl_gl3.cpp ^
    .\lib\imgui\imgui.cpp .\lib\imgui\imgui_demo.cpp .\lib\imgui\imgui_draw.cpp ^
    .\lib\gl3w\GL\gl3w.c ^
    /I %IMGUIDIR% ^
    /I %IMGUIDIR%\examples\sdl_opengl3_example ^
    /I %SDL2DIR%\include ^
    /I %GL3WDIR% ^
    /D_HAS_EXCEPTIONS=0 ^
    SDL2.lib SDL2main.lib user32.lib opengl32.lib ws2_32.lib ^
    /link /SUBSYSTEM:CONSOLE ^
          /LIBPATH:"%SDL2DIR%\lib\x64"

rmdir dist
mkdir dist
copy %BUILDDIR%\aniplot.exe .\dist\
copy %SDL2DIR%\lib\x64\SDL2.dll .\dist\
