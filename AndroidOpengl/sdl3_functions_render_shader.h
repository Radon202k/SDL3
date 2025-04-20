unsigned int
shader_compile(GLenum type,
               const char *source)
{
    unsigned int result =
        glCreateShader(type);
    
    glShaderSource(result,
                   1,
                   &source,
                   0);
    
    glCompileShader(result);
    
    int success = 0;
    char infoLog[512] = {0};
    glGetShaderiv(result,
                  GL_COMPILE_STATUS,
                  &success);
    
    if(!success)
    {
        glGetShaderInfoLog(result,
                           512,
                           0,
                           infoLog);
        
        SDL_Log("Shader compilation failed:\n%s\n",
                infoLog);
    }
    
    assert(success);
    
    return result;
}

unsigned int
shader_link(unsigned int vert,
            unsigned int frag)
{
    unsigned int result =
        glCreateProgram();
    
    glAttachShader(result, vert);
    glAttachShader(result, frag);
    glLinkProgram(result);
    
    int success = 0;
    glGetProgramiv(result,
                   GL_LINK_STATUS,
                   &success);
    
    char infoLog[512] = {0};
    if(!success)
    {
        glGetProgramInfoLog(result,
                            512,
                            0,
                            infoLog);
        
        SDL_Log("Shader link failed:\n%s\n",
                infoLog);
    }
    
    return result;
}

RenderShader
shader_init(const char *sourceVert,
            const char *sourceFrag)
{
    RenderShader result = {0};
    
    unsigned int vert =
        shader_compile(GL_VERTEX_SHADER,
                       sourceVert);
    
    unsigned int frag =
        shader_compile(GL_FRAGMENT_SHADER,
                       sourceFrag);
    
    result.program =
        shader_link(vert, frag);
    
    glDeleteShader(vert);
    glDeleteShader(frag);
    
    return result;
}