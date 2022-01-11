:: This builds the release version of aniplot-example.exe with Visual Studio C++.
:: Be sure to select (comment/uncomment) the installed Visual Studio version below.

@echo off

:: vcvarsall.bat will sometimes change the current working directory. prevent it.
set VSCMD_START_DIR=%cd%
:: set VSCMD_DEBUG=3

:: Visual Studio 2022 (17)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
:: Visual Studio 2019 (16)
::call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
:: Visual Studio 2017 (15)
::call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
:: Visual Studio 2015 (14)
::call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
:: Visual Studio 2013 (12)
::call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64

set SDL2DIR=.\lib\SDL2-devel-2.0.4-VC
set IMGUIDIR=.\lib\imgui
set BUILDDIR=.\build

:: look into build64-vc.bat for more comments

rmdir /S /Q build
mkdir build


cl /FC /MD /O2 /Fo%BUILDDIR%\ /Fd%BUILDDIR%\ /Fm%BUILDDIR%\ /Fe%BUILDDIR%\aniplot_example.exe ^
    aniplot_example.cpp ^
    aniplotlib.cpp ^
    imgui_unitybuild.cpp ^
    /D_HAS_EXCEPTIONS=0 /DNDEBUG ^
    /I %IMGUIDIR% ^
    /I %IMGUIDIR%\backends ^
    /I %SDL2DIR%\include ^
    SDL2.lib SDL2main.lib user32.lib opengl32.lib ws2_32.lib ^
    /link /SUBSYSTEM:CONSOLE ^
          /LIBPATH:"%SDL2DIR%\lib\x64" /OPT:REF /OPT:ICF


if %errorlevel% neq 0 goto error

rmdir /S /Q dist
mkdir dist
copy %BUILDDIR%\aniplot_example.exe .\dist\
copy %SDL2DIR%\lib\x64\SDL2.dll .\dist\

if %errorlevel% neq 0 goto error

:: compress files
if exist "upx.exe" (
    upx --lzma .\dist\aniplot_example.exe
    upx --lzma .\dist\SDL2.dll
)

echo(
echo --------------------------------------------------
echo enjoy aniplot_example in ./dist folder

goto :EOF


:error
echo(
echo ------------------------------
echo Failed with error #%errorlevel%.
exit /b %errorlevel%
