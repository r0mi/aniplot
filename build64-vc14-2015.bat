:: this builds the release version

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
:: /Gy : Enables function-level linking. Prevents unused functions from being included.
:: /Ox : Uses maximum optimization (/Ob2gity /Gs).Uses maximum optimization (/Ob2gity /Gs).
:: /OPT:REF : eliminates functions and data that are never referenced (depends on /Gy, but seems to do nothing for this program)
::

:: TODO: how should i have known to use /MD? SDL2 has written nowhere how it was compiled.

:: https://www.libsdl.org/release/SDL2-devel-2.0.4-VC.zip
:: https://sourceforge.net/projects/glew/files/glew/1.13.0/glew-1.13.0-win32.zip/download

:: well whadoyouknow! ending even a fucking COMMENT with a caret does something weird

set SDL2DIR=.\lib\SDL2-devel-2.0.4-VC
set GL3WDIR=.\lib\gl3w
set IMGUIDIR=.\lib\imgui
set BUILDDIR=.\build

:: vcvarsall.bat will sometimes change the current working directory. prevent it.
set VSCMD_START_DIR=%cd%
:: set VSCMD_DEBUG=3
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

rmdir /S /Q build
mkdir build


cl /FC /MD /Ox /Gy /Fo%BUILDDIR%\ /Fd%BUILDDIR%\ /Fm%BUILDDIR%\ /Fe%BUILDDIR%\aniplot.exe ^
    aniplot.cpp ^
    aniplotlib.cpp ^
    %IMGUIDIR%\examples\sdl_opengl3_example\imgui_impl_sdl_gl3.cpp ^
    %IMGUIDIR%\imgui.cpp %IMGUIDIR%\imgui_draw.cpp ^
    %IMGUIDIR%\imgui_demo.cpp ^
    %GL3WDIR%\GL\gl3w.c ^
    /D_HAS_EXCEPTIONS=0 /DNDEBUG ^
    /I %IMGUIDIR% ^
    /I %IMGUIDIR%\examples\sdl_opengl3_example ^
    /I %SDL2DIR%\include ^
    /I %GL3WDIR% ^
    SDL2.lib SDL2main.lib user32.lib opengl32.lib ws2_32.lib ^
    /link /SUBSYSTEM:CONSOLE ^
          /LIBPATH:"%SDL2DIR%\lib\x64" /OPT:REF /OPT:ICF

if %errorlevel% neq 0 goto error

rmdir /S /Q dist
mkdir dist
copy %BUILDDIR%\aniplot.exe .\dist\
copy %SDL2DIR%\lib\x64\SDL2.dll .\dist\

if %errorlevel% neq 0 goto error

if exist "upx.exe" (
    upx --lzma .\dist\aniplot.exe
    upx --lzma .\dist\SDL2.dll
)

echo(
echo ------------------------------
echo enjoy aniplot in ./dist folder

goto :EOF


:error
echo(
echo ------------------------------
echo Failed with error #%errorlevel%.
exit /b %errorlevel%
