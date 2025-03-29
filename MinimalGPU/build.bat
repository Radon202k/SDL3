@echo off

set SDL_PATH=W:/libs/sdl/3.2.8
set SDLI=-I%SDL_PATH%/include
set SDLL=-LIBPATH:%SDL_PATH%/lib/x64
set CFLAGS=-W4 -wd4100 -FC -EHsc -MT -nologo -Z7

IF NOT EXIST bin mkdir bin
pushd bin
cl %SDLI% %CFLAGS% ../main.c -link -SUBSYSTEM:WINDOWS %SDLL% SDL3.lib
popd