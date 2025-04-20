RenderBuffers
render_buffers_init(void)
{
    RenderBuffers result = {0};
    
    glGenVertexArrays(1, &result.vao);
    glBindVertexArray(result.vao);
    
    glGenBuffers(1, &result.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    
    // Upload test data
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vertices),
                 vertices,
                 GL_STATIC_DRAW);
    
    // Define vertex input layout
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          3 * sizeof(float),
                          (void*)0);
    
    glEnableVertexAttribArray(0);
    
    // Unbind VAO now that VBO is setup
    glBindVertexArray(0);
    
    return result;
}
