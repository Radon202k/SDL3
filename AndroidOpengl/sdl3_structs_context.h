#define SDL_ASSERT_LEVEL 2
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>

#ifndef __ANDROID__
#define assert(e) SDL_assert(e)
#endif

typedef struct
{
    SDL_Window *window;
    SDL_GLContext glContext;
    int winWidth;
    int winHeight;
    
} SDL3Context;
