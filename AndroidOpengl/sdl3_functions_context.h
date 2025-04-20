SDL3Context
sdl3_init(SDL_InitFlags initFlags,
          int winWidth,
          int winHeight,
          char *winTitle)
{
    SDL3Context result = {0};
    
    // Init SDL
    assert(SDL_Init(initFlags));
	
#ifdef __ANDROID__
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    SDL_Rect bounds;
    
    SDL_GetDisplayBounds(display, &bounds);
    result.winWidth = bounds.w;
    result.winHeight = bounds.h;
    
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles");
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    
#else
    result.winWidth = winWidth;
    result.winHeight = winHeight;
#endif
    
    // Create window
    result.window =
        SDL_CreateWindow(winTitle,
                         result.winWidth,
                         result.winHeight,
                         SDL_WINDOW_OPENGL);
    
    assert(result.window);
    
#ifdef __ANDROID__
    SDL_SetWindowFullscreen(result.window, true);
#endif
    
    // Create opengl context
    result.glContext =
        SDL_GL_CreateContext(result.window);
    
    assert(result.glContext);
    
    assert(SDL_GL_MakeCurrent(result.window,
                              result.glContext));
    
    assert(SDL_GL_SetSwapInterval(1));
    
    int interval = 0;
    assert(SDL_GL_GetSwapInterval(&interval));
    
    assert(interval == 1);
    
    // Load Modern Opengl functions
    sdl3_load_opengl_funcs(&result);
    
    
    const char* versionStr = (const char*)glGetString(GL_VERSION);
    SDL_Log("OpenGL ES version: %s\n",
            versionStr);
    
    return result;
}