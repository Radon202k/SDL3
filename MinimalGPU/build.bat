@echo off

set sdli=-IW:\libs\sdl\3.2.8\include
set sdll=-LIBPATH:W:\libs\sdl\3.2.8\lib\x64

IF NOT EXIST bin mkdir bin

pushd bin

cl -FC -EHsc -MT -nologo -Z7 %sdli% ../main.c -link %sdll% -SUBSYSTEM:WINDOWS SDL3.lib

popd