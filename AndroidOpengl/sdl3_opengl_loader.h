#ifdef __ANDROID__
#define GL_GET_PROC_ADDRESS SDL_EGL_GetProcAddress
#else
#define GL_GET_PROC_ADDRESS SDL_GL_GetProcAddress
#endif

#define GL_FUNCTIONS(X) \
X(PFNGLGENBUFFERSPROC, glGenBuffers) \
X(PFNGLBINDBUFFERPROC, glBindBuffer) \
X(PFNGLBUFFERDATAPROC, glBufferData) \
X(PFNGLCREATEBUFFERSPROC, glCreateBuffers) \
X(PFNGLNAMEDBUFFERSTORAGEPROC, glNamedBufferStorage) \
X(PFNGLCREATESHADERPROC, glCreateShader) \
X(PFNGLSHADERSOURCEPROC, glShaderSource) \
X(PFNGLCOMPILESHADERPROC, glCompileShader) \
X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
X(PFNGLATTACHSHADERPROC, glAttachShader) \
X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
X(PFNGLDELETESHADERPROC, glDeleteShader) \
X(PFNGLUSEPROGRAMPROC, glUseProgram) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)

// Helper line to copy paste when adding new functions
// X(PFNPROC, ) \

#define X(type, name) static type name;
GL_FUNCTIONS(X)
#undef X

#define STR2(x) #x
#define STR(x) STR2(x)

void
sdl3_load_opengl_funcs(SDL3Context *sdl)
{
#define X(type, name) name = (type)GL_GET_PROC_ADDRESS(STR(name));
    GL_FUNCTIONS(X)
#undef X
}


