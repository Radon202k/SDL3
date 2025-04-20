#include "sdl3_structs_context.h"
#include "sdl3_structs_render_shader.h"
#include "sdl3_structs_render_buffers.h"

// Needed to load modern opengl functions
#include "sdl3_opengl_loader.h"

#include "sdl3_functions_context.h"
#include "sdl3_functions_render_shader.h"
#include "sdl3_functions_render_buffers.h"

typedef struct
{
    SDL3Context sdl;
    RenderShader testShader;
    RenderBuffers testBuffers;
    Uint64 lastTicks;
    int quit;
    
} Platform;

void
platform_shutdown(Platform *plat)
{
    SDL_GL_DestroyContext(plat->sdl.glContext);
    SDL_DestroyWindow(plat->sdl.window);
    
    SDL_Quit();
}

Platform
platform_init(void)
{
    Platform result = {0};
    
    result.sdl =
        sdl3_init(SDL_INIT_EVENTS | SDL_INIT_VIDEO,
                  800,
                  600,
                  "Mah Windou");
    
    char *shaderPrependVert = 0;
    char *shaderPrependFrag = 0;
    
#ifdef __ANDROID__
    shaderPrependVert = "#version 320 es\n";
    shaderPrependFrag =
        "#version 320 es\n"
        "precision mediump float;\n";
#else
    shaderPrependVert = "#version 330 core\n";
    shaderPrependFrag = "#version 330 core\n";
#endif
    
    const char *shaderBodyVert =
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\n";
    
    const char *shaderBodyFrag =
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\n";
    
    char shaderSourceVert[4096] = {0};
    char shaderSourceFrag[4096] = {0};
    
    // Build full vertex shader
    SDL_strlcat(shaderSourceVert, shaderPrependVert, 4096);
    SDL_strlcat(shaderSourceVert, shaderBodyVert, 4096);
    
    // Build full fragment shader
    SDL_strlcat(shaderSourceFrag, shaderPrependFrag, 4096);
    SDL_strlcat(shaderSourceFrag, shaderBodyFrag, 4096);
    
    result.testShader =
        shader_init(shaderSourceVert,
                    shaderSourceFrag);
    
    result.testBuffers =
        render_buffers_init();
    
    result.lastTicks = SDL_GetTicks();
    
    return result;
}

void
plat_frame_begin(Platform *plat)
{
    // Poll events
    SDL_Event evt;
    while (SDL_PollEvent(&evt))
    {
        switch (evt.type)
        {
            case SDL_EVENT_QUIT:
            {
                plat->quit = 1;
            } break;
        }
    }
    
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void
plat_frame_end(Platform *plat)
{
    // Unbind
    glUseProgram(0);
    glBindVertexArray(0);
    
    SDL_GL_SwapWindow(plat->sdl.window);
    
    Uint64 currentTicks = SDL_GetTicks();
    Uint64 deltaTicks = currentTicks - plat->lastTicks;
    plat->lastTicks = currentTicks;
}

#if 0

// You can use these for debugging in android

SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
             "SDL_ShowSimpleMessageBox failed (%s)",
             SDL_GetError());

SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                         "Hello World",
                         "!! Your SDL project successfully runs on Android !!",
                         0);

#endif