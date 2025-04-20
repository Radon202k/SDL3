@echo off

REM compiler flags - debug
set cf=-FC -nologo -Z7 -W4 -wd4100 -wd4189

REM compiler flags - release
REM set cf=-FC -nologo -O2 -W4 -wd4100 -wd4189

REM sdl 3.10.2 - debug build
set sdlp=W:\libs\sdl\SDL3-devel-3.2.10-VC\SDL3-3.2.10

set sdli=-I%sdlp%\include
set sdll=-LIBPATH:%sdlp%\lib\x64

IF NOT EXIST bin mkdir bin

pushd bin

cl %cf% %sdli% ../main.c -link -SUBSYSTEM:WINDOWS %sdll% SDL3.lib opengl32.lib

popd