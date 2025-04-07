#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <assert.h>

typedef struct
{
    SDL_GPUBuffer *vertex;
    SDL_GPUBuffer *index;
    SDL_GPUTransferBuffer *transfer;
    Uint32 indexCount;
    
} RenderBuffers;

typedef struct
{
	char *basePath;
	SDL_Window* window;
	SDL_GPUDevice* device;
	float deltaTime;
    float time;
    
    Uint32 winWidth;
    Uint32 winHeight;
    
    SDL_GPUSampler *samplerPoint;
    SDL_GPUTexture *texture;
    SDL_GPUTransferBuffer *transferBufferTexture;
    
    // Dynamic rendering
    RenderBuffers buffersDynamic;
    SDL_GPUGraphicsPipeline* pipelineDynamic;
    
    // Post-process
    SDL_GPUGraphicsPipeline* pipelinePostProcess;
    SDL_GPUTexture *texturePostProcess;
    
} Context;

typedef struct
{
    float x, y;
    float u, v;
    float r, g, b, a;
    
} Vertex;

SDL_GPUShader*
shader_load(Context* context,
            char* shaderFilename,
            SDL_GPUShaderStage stage,
            Uint32 samplerCount,
            Uint32 uniformCount)
{
    // Construct a full path with basePath and shaderFilename
	char fullPath[512] = {0};
    SDL_strlcat(fullPath, context->basePath, sizeof(fullPath));
    SDL_strlcat(fullPath, shaderFilename, sizeof(fullPath));
    
    // Load the SPIR-V "code" (these must have been compiled already)
	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	assert(code);
    
    // Create the shader
    SDL_GPUShaderCreateInfo shaderInfo =
    {
        codeSize,
        code,
        "main",
        SDL_GPU_SHADERFORMAT_SPIRV,
        stage,
        samplerCount,
        0, // storage textures
        0, // storage buffers
        uniformCount // uniform buffers
    };
    
    SDL_GPUShader* shader = SDL_CreateGPUShader(context->device, &shaderInfo);
    assert(shader);
    
    // Free the code memory and return the shader
    SDL_free(code);
    return shader;
}

SDL_GPUGraphicsPipeline *
create_pipeline(Context *context,
                SDL_GPUShader *shaderVertex,
                SDL_GPUShader *shaderFragment,
                SDL_GPUVertexBufferDescription vertexBufferDescArray[],
                Uint32 vertexBufferDescCount,
                SDL_GPUVertexAttribute vertexAttribArray[],
                Uint32 vertexAttribCount)
{
    SDL_GPUGraphicsPipeline *result = 0;
    
    SDL_GPURasterizerState rasterizerState = { SDL_GPU_FILLMODE_FILL };
    
    // The vertex input state (buffrs and layouts)
    SDL_GPUVertexInputState vertexInputState =
    {
        vertexBufferDescArray,
        vertexBufferDescCount,
        vertexAttribArray,
        vertexAttribCount
    };
    
    SDL_GPUMultisampleState multisampleState = {0};
    SDL_GPUDepthStencilState depthStencilState = {0};
    
    // The color target array
    SDL_GPUColorTargetDescription colorTargetDescArray[] =
    {
        { SDL_GetGPUSwapchainTextureFormat(context->device, context->window) }
    };
    
    // The target config (color targets, etc)
    SDL_GPUGraphicsPipelineTargetInfo targetInfo =
    {
        .color_target_descriptions = colorTargetDescArray,
        .num_color_targets = SDL_arraysize(colorTargetDescArray),
    };
    
    // The pipeline config
    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo =
    {
		shaderVertex,
		shaderFragment,
        vertexInputState,
		SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        rasterizerState,
        multisampleState,
        depthStencilState,
        targetInfo
    };
    
    result =
        SDL_CreateGPUGraphicsPipeline(context->device,
                                      &pipelineCreateInfo);
    if (!context->pipelineDynamic)
    {
        const char *error = SDL_GetError();
        SDL_Log(error);
    }
    
    assert(result);
    
    // Clean up shader resources
    SDL_ReleaseGPUShader(context->device, shaderVertex);
    SDL_ReleaseGPUShader(context->device, shaderFragment);
    
    return result;
}

void
create_pipeline_dynamic(Context *context)
{
    // Create the shaders
	SDL_GPUShader* shaderVertex = shader_load(context,
                                              "shaders/vert.spv",
                                              SDL_GPU_SHADERSTAGE_VERTEX,
                                              0, // sampler count
                                              1); // uniform count
    
	SDL_GPUShader* shaderFragment = shader_load(context,
                                                "shaders/frag.spv",
                                                SDL_GPU_SHADERSTAGE_FRAGMENT,
                                                1, // sampler count
                                                0); // uniform count
    
    SDL_GPUVertexBufferDescription vertexBufferDescArray[] =
    {
        { 0, sizeof(Vertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0 }
    };
    
    SDL_GPUVertexAttribute vertexAttribArray[] =
    {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0 },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 2 },
        { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, sizeof(float) * 4 }
    };
    
    context->pipelineDynamic =
        create_pipeline(context,
                        shaderVertex,
                        shaderFragment,
                        vertexBufferDescArray,
                        SDL_arraysize(vertexBufferDescArray),
                        vertexAttribArray,
                        SDL_arraysize(vertexAttribArray));
}

void
create_pipeline_postprocess(Context *context)
{
    // Create the shaders
	SDL_GPUShader* shaderVertex = shader_load(context,
                                              "shaders/ppvert.spv",
                                              SDL_GPU_SHADERSTAGE_VERTEX,
                                              0, // sampler count
                                              0); // uniform count
    
	SDL_GPUShader* shaderFragment = shader_load(context,
                                                "shaders/ppfrag.spv",
                                                SDL_GPU_SHADERSTAGE_FRAGMENT,
                                                1, // sampler count
                                                1); // uniform count
    
    SDL_GPUVertexBufferDescription vertexBufferDescArray[] = { 0 };
    SDL_GPUVertexAttribute vertexAttribArray[] = { 0 };
    
    context->pipelinePostProcess =
        create_pipeline(context,
                        shaderVertex,
                        shaderFragment,
                        vertexBufferDescArray,
                        SDL_arraysize(vertexBufferDescArray),
                        vertexAttribArray,
                        SDL_arraysize(vertexAttribArray));
}

void
release_buffers(Context *context,
                RenderBuffers *buffers)
{
    SDL_ReleaseGPUBuffer(context->device, buffers->vertex);
    SDL_ReleaseGPUBuffer(context->device, buffers->index);
    SDL_ReleaseGPUTransferBuffer(context->device, buffers->transfer);
}

RenderBuffers
create_buffers(Context *context,
               Uint32 maxSizeVertex,
               Uint32 maxSizeIndex)
{
    RenderBuffers result = {0};
    
    // Create vertex buffer
    result.vertex = SDL_CreateGPUBuffer(context->device,
                                        &(SDL_GPUBufferCreateInfo)
                                        {
                                            SDL_GPU_BUFFERUSAGE_VERTEX,
                                            maxSizeVertex,
                                            0
                                        });
    
    // Create index buffer
    result.index = SDL_CreateGPUBuffer(context->device,
                                       &(SDL_GPUBufferCreateInfo)
                                       {
                                           SDL_GPU_BUFFERUSAGE_INDEX,
                                           maxSizeIndex,
                                           0
                                       });
    
    // Create transfer buffer for vertex data
    result.transfer =
        SDL_CreateGPUTransferBuffer(context->device,
                                    &(SDL_GPUTransferBufferCreateInfo)
                                    {
                                        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                        maxSizeVertex + maxSizeIndex
                                    });
    
    return result;
}

void
update_buffers(Context *context,
               RenderBuffers *buffers,
               void *dataVert, Uint32 dataSizeVert,
               void *dataInd, Uint32 dataSizeInd)
{
    void* destData = SDL_MapGPUTransferBuffer(context->device,
                                              buffers->transfer,
                                              false);
    
    // copy vertex data to GPU
    memcpy(destData,
           dataVert,
           dataSizeVert);
    
    // copy index data to GPU
    memcpy((Uint8 *)destData + dataSizeVert,
           dataInd,
           dataSizeInd);
    
    SDL_UnmapGPUTransferBuffer(context->device, buffers->transfer);
    
    // Start command buffer and begin copy pass
    SDL_GPUCommandBuffer *cmdBuf = SDL_AcquireGPUCommandBuffer(context->device);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmdBuf);
    
    // Upload vertex data
    SDL_UploadToGPUBuffer(copyPass,
                          &(SDL_GPUTransferBufferLocation)
                          {
                              buffers->transfer,
                              0 // offset
                          },
                          &(SDL_GPUBufferRegion)
                          {
                              buffers->vertex, 0, dataSizeVert
                          },
                          false);
    
    // Upload index data
    SDL_UploadToGPUBuffer(copyPass,
                          &(SDL_GPUTransferBufferLocation)
                          {
                              buffers->transfer,
                              dataSizeVert // offset
                          },
                          &(SDL_GPUBufferRegion)
                          {
                              buffers->index, 0, dataSizeInd
                          },
                          false);
    
    // End pass and submit command buffer
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmdBuf);
    
    buffers->indexCount = dataSizeInd / sizeof(Uint32);
}

void
create_texture(Context *context, Uint32 width, Uint32 height)
{
    context->texture = SDL_CreateGPUTexture(context->device,
                                            &(SDL_GPUTextureCreateInfo)
                                            {
                                                SDL_GPU_TEXTURETYPE_2D,
                                                SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
                                                SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                                width,
                                                height,
                                                1, // layer count
                                                1, // mip levels
                                                SDL_GPU_SAMPLECOUNT_1
                                            });
    
    // Transfer buffer
    context->transferBufferTexture =
        SDL_CreateGPUTransferBuffer(context->device,
                                    &(SDL_GPUTransferBufferCreateInfo)
                                    {
                                        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                        width * height * sizeof(Uint32)
                                    });
}

void
update_texture(Context *context,
               SDL_GPUTexture *texture,
               SDL_GPUTransferBuffer *transfer,
               Uint32 width, Uint32 height,
               void *data)
{
    // Map and copy texture data to GPU
    Uint32* destData = SDL_MapGPUTransferBuffer(context->device,
                                                transfer,
                                                false);
    memcpy(destData, data, width * height * sizeof(Uint32));
    SDL_UnmapGPUTransferBuffer(context->device, transfer);
    
    // Start command buffer and begin copy pass
    SDL_GPUCommandBuffer *cmdBuf = SDL_AcquireGPUCommandBuffer(context->device);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmdBuf);
    
    SDL_UploadToGPUTexture(copyPass,
                           &(SDL_GPUTextureTransferInfo)
                           {
                               transfer,
                               0,
                               width,
                               height
                           },
                           &(SDL_GPUTextureRegion)
                           {
                               texture,
                               0, // mip level
                               0, // layer
                               0, // x
                               0, // y
                               0, // z
                               width,
                               height,
                               1 // depth
                           },
                           false);
    
    // End pass and submit command buffer
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmdBuf);
}

void
render_pass(Context *context,
            SDL_GPUCommandBuffer* cmdbuf,
            SDL_GPUGraphicsPipeline *pipeline,
            SDL_GPUTexture *texture,
            SDL_GPUTexture *target,
            SDL_FColor clearColor,
            RenderBuffers *buffers,
            float matrix[],
            float postProcessData[])
{
    // Setup the color target as the swapchain image
    SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
    colorTargetInfo.texture = target;
    colorTargetInfo.clear_color = clearColor;
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    
    // Begin a render pass
    SDL_GPURenderPass* renderPass =
        SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
    
    if (matrix)
    {
        SDL_PushGPUVertexUniformData(cmdbuf,
                                     0,
                                     matrix,
                                     sizeof(float) * 16);
    }
    
    if (postProcessData)
    {
        SDL_PushGPUFragmentUniformData(cmdbuf,
                                       0,
                                       postProcessData,
                                       sizeof(float) * 4);
    }
    
    // Bind our graphics pipeline and draw a triangle
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
    
    // Texture and Sampler
    SDL_BindGPUFragmentSamplers(renderPass,
                                0, // first slot
                                &(SDL_GPUTextureSamplerBinding)
                                {
                                    texture,
                                    context->samplerPoint
                                },
                                1);
    
    if (buffers)
    {
        SDL_BindGPUVertexBuffers(renderPass, 0,
                                 &(SDL_GPUBufferBinding){ buffers->vertex, 0 },
                                 1);
        
        SDL_BindGPUIndexBuffer(renderPass,
                               &(SDL_GPUBufferBinding){ buffers->index, 0 },
                               SDL_GPU_INDEXELEMENTSIZE_32BIT);
        
        SDL_DrawGPUIndexedPrimitives(renderPass, buffers->indexCount, 1, 0, 0, 0);
    }
    else
    {
        // Assume it's a post-process which has in-shader vertices
        SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);
    }
    
    // End the render pass
    SDL_EndGPURenderPass(renderPass);
}

// Main entry point
int
main(int argc, char **argv)
{
    // Context struct and some helper variables
    Context context = {0};
    bool quit = false;
    bool minimized = false;
    float lastTime = 0;
    
    // Init SDL
    assert(SDL_Init(SDL_INIT_VIDEO));
	
    // Get base path
    context.basePath = (char *)SDL_GetBasePath();
    assert(context.basePath);
    
    // Create GPU Device
    context.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV,
                                         false, 0);
    assert(context.device);
    
    // Create window
    context.winWidth = 800;
    context.winHeight = 600;
    context.window = SDL_CreateWindow("Minimal SDL3 GPU Example",
                                      context.winWidth, context.winHeight,
                                      SDL_WINDOW_VULKAN);
    assert(context.window);
    
    // Associate window with GPU Device
    assert(SDL_ClaimWindowForGPUDevice(context.device, context.window));
    
    // Create pipelines
    create_pipeline_dynamic(&context);
    create_pipeline_postprocess(&context);
    
    Uint32 maxQuadCount = 4096;
    
    // Create dynamic buffers
    context.buffersDynamic =
        create_buffers(&context,
                       sizeof(Vertex) * 4 * maxQuadCount,
                       sizeof(Uint32) * 6 * maxQuadCount);
    
    // Create Point Sampler
    context.samplerPoint =
        SDL_CreateGPUSampler(context.device,
                             &(SDL_GPUSamplerCreateInfo)
                             {
                                 SDL_GPU_FILTER_NEAREST,
                                 SDL_GPU_FILTER_NEAREST,
                                 SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
                                 SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                 SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                 SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
                                 0, // mip lod bias
                                 0, // max anisotropic
                                 SDL_GPU_COMPAREOP_GREATER,
                                 0, // min lod
                                 0, // max lod
                                 false, // enable anisotropic
                                 false // enable compare
                             });
    
    // Create Post-process Texture
    context.texturePostProcess =
        SDL_CreateGPUTexture(context.device,
                             &(SDL_GPUTextureCreateInfo)
                             {
                                 SDL_GPU_TEXTURETYPE_2D,
                                 SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
                                 SDL_GPU_TEXTUREUSAGE_SAMPLER |
                                     SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
                                 context.winWidth,
                                 context.winHeight,
                                 1, // layer count
                                 1, // mip levels
                                 SDL_GPU_SAMPLECOUNT_1
                             });
    
    // Texture
    Uint32 texWidth = 2;
    Uint32 texHeight = 2;
    create_texture(&context, texWidth, texHeight);
    
    Uint32 texData[] =
    {
        0xFF00FF00, 0xFFFF0000,
        0xFFFF0000, 0xFF00FF00
    };
    
    update_texture(&context,
                   context.texture,
                   context.transferBufferTexture,
                   texWidth, texHeight, texData);
    
    float lastMouseX = 0;
    float lastMouseY = 0;
    bool mouseLeftDown = false;
    
    // Update and render loop
    while (!quit)
    {
        // Poll events
        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            switch (evt.type)
            {
                case SDL_EVENT_QUIT:
                {
                    quit = true;
                } break;
                
                case SDL_EVENT_WINDOW_MINIMIZED:
                {
                    minimized = true;
                } break;
                
                case SDL_EVENT_WINDOW_RESTORED:
                {
                    minimized = false;
                } break;
                
                case SDL_EVENT_MOUSE_MOTION:
                {
                    lastMouseX = evt.motion.x;
                    lastMouseY = evt.motion.y;
                } break;
                
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    if (evt.button.button == 1)
                    {
                        mouseLeftDown = true;
                    }
                } break;
                
                case SDL_EVENT_MOUSE_BUTTON_UP:
                {
                    if (evt.button.button == 1)
                    {
                        mouseLeftDown = false;
                    }
                } break;
            }
        }
        
        // Break out of update and render loop if we just quit
        if (quit) break;
        
        // If the window is not minimized
        if (!minimized)
        {
            // Calculate delta time for the frame
            float newTime = SDL_GetTicks() / 1000.0f;
            context.deltaTime = newTime - lastTime;
            lastTime = newTime;
            context.time += context.deltaTime;
            
            // Update data
            {
                // Vertex data
                float s = 500.0f;
                if (!mouseLeftDown)
                {
                    Vertex vertices[] =
                    {
                        {0, 0,    0, 0,    1.0f, 1.0f, 1.0f, 1.0f},
                        {s, 0,    1, 0,    1.0f, 1.0f, 1.0f, 1.0f},
                        {s, s,    1, 1,    1.0f, 1.0f, 1.0f, 1.0f},
                        {0, s,    0, 1,    1.0f, 1.0f, 1.0f, 1.0f}
                    };
                    
                    size_t vertexSize = sizeof(vertices);
                    
                    // Index data
                    Uint32 indices[] = 
                    {
                        0, 1, 2,
                        2, 3, 0
                    };
                    
                    update_buffers(&context,
                                   &context.buffersDynamic,
                                   vertices, sizeof(vertices),
                                   indices, sizeof(indices));
                }
                else
                {
                    Vertex vertices[] =
                    {
                        {0, 0,    0, 0,    1.0f, 1.0f, 1.0f, 1.0f},
                        {s, 0,    1, 0,    1.0f, 1.0f, 1.0f, 1.0f},
                        {s, s,    1, 1,    1.0f, 1.0f, 1.0f, 1.0f},
                        {0, s,    0, 1,    1.0f, 1.0f, 1.0f, 1.0f},
                        
                        {lastMouseX + 0, lastMouseY + 0,    0, 0,    1.0f, 1.0f, 1.0f, 1.0f},
                        {lastMouseX + s, lastMouseY + 0,    1, 0,    1.0f, 1.0f, 1.0f, 1.0f},
                        {lastMouseX + s, lastMouseY + s,    1, 1,    1.0f, 1.0f, 1.0f, 1.0f},
                        {lastMouseX + 0, lastMouseY + s,    0, 1,    1.0f, 1.0f, 1.0f, 1.0f}
                    };
                    
                    size_t vertexSize = sizeof(vertices);
                    
                    // Index data
                    Uint32 indices[] = 
                    {
                        0, 1, 2, 2, 3, 0,
                        4, 5, 6, 6, 7, 4
                    };
                    
                    update_buffers(&context,
                                   &context.buffersDynamic,
                                   vertices, sizeof(vertices),
                                   indices, sizeof(indices));
                }
            }
            
            // Acquire a command buffer to render with
            SDL_GPUCommandBuffer* cmdbuf =
                SDL_AcquireGPUCommandBuffer(context.device);
            assert(cmdbuf);
            
            // Acquire a swapchain image to render into
            SDL_GPUTexture* swapchainTexture;
            assert(SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf,
                                                         context.window,
                                                         &swapchainTexture,
                                                         0, 0));
            
            // If we got a swapchain image
            if (swapchainTexture)
            {
                SDL_FColor clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                
                // Render dynamic buffers to post-process texture
                {
                    // Update uniform
                    float matrix[] =
                    {
                        2.0f / (float)context.winWidth, 0, 0, -1,
                        0, -2.0f / (float)context.winHeight, 0, 1,
                        0, 0, 1, 0,
                        0, 0, 0, 1
                    };
                    
                    // Render to texture
                    render_pass(&context,
                                cmdbuf,
                                context.pipelineDynamic,
                                context.texture, // texture
                                context.texturePostProcess, // target
                                clearColor,
                                &context.buffersDynamic,
                                matrix,
                                0); // post-process data
                }
                
                // Render post-process texture to screen
                {
                    float postProcessData[] =
                    {
                        context.time,
                        0.2f, // speed
                        8.0, // frequency
                        0.1f // amplitude
                    };
                    
                    // Render texture to screen
                    render_pass(&context,
                                cmdbuf,
                                context.pipelinePostProcess,
                                context.texturePostProcess, // texture
                                swapchainTexture, // target
                                clearColor,
                                0, // buffers
                                0, // matrix
                                postProcessData);
                }
                
                // Submit the command buffer
                SDL_SubmitGPUCommandBuffer(cmdbuf);
            }
            else
            {
                // If we didn't get a swapchain image, the window is probably
                // minimized, so we cancel the command buffer
                minimized = true;
                SDL_CancelGPUCommandBuffer(cmdbuf);
            }
        }
        else
        {
            // Window is minimized, sleep for a frame to avoid high CPU usage
            SDL_Delay(1000.0f / 60);
        }
    }
    
    // Release sampler
    SDL_ReleaseGPUSampler(context.device, context.samplerPoint);
    
    // Release framebuffer texture
    SDL_ReleaseGPUTexture(context.device, context.texturePostProcess);
    
    // Release texture
    SDL_ReleaseGPUTexture(context.device, context.texture);
    
    // Release buffers
    release_buffers(&context, &context.buffersDynamic);
    
    // Release Transfer buffers
    SDL_ReleaseGPUTransferBuffer(context.device, context.transferBufferTexture);
    
    SDL_ReleaseGPUGraphicsPipeline(context.device, context.pipelineDynamic);
    SDL_ReleaseGPUGraphicsPipeline(context.device, context.pipelinePostProcess);
    
    SDL_ReleaseWindowFromGPUDevice(context.device, context.window);
    SDL_DestroyWindow(context.window);
    SDL_DestroyGPUDevice(context.device);
    
    return 0;
}