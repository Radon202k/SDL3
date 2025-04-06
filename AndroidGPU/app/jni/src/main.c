#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct
{
    char *basePath;
    SDL_Window* window;
    SDL_GPUDevice* device;
    float deltaTime;
    SDL_GPUGraphicsPipeline* pipeline;
    
    SDL_GPUBuffer *vertexBuf;
    SDL_GPUBuffer *indexBuf;
    SDL_GPUTransferBuffer *transBufVert;
    SDL_GPUTransferBuffer *transBufInd;
    
    SDL_GPUTexture *texture;
    SDL_GPUTransferBuffer *transBufTex;
    
    SDL_GPUSampler *sampler;
    
} Context;

typedef struct
{
    float x, y;
    float u, v;
    float r, g, b, a;
    
} Vertex;

SDL_GPUShader*
shader_load(Context* context,
            const char* shaderFilename,
            SDL_GPUShaderStage stage,
            Uint32 samplerCount,
            Uint32 uniformCount)
{
    // Try loading the shader from assets
    SDL_IOStream* file = SDL_IOFromFile(shaderFilename, "rb");
    if (!file)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Couldn't load the shader file (%s)",
                     shaderFilename);
    }
    
    // Get the file size
    SDL_SeekIO(file, 0, SEEK_END);
    size_t codeSize = SDL_TellIO(file);
    SDL_SeekIO(file, 0, SEEK_SET);
    
    // Allocate memory and read the file
    void* code = SDL_malloc(codeSize);
    if (!code)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Couldn't load the shader code (%s)",
                     SDL_GetError());
    }
    
    SDL_ReadIO(file, code, codeSize);
    
    SDL_CloseIO(file);
    
    // Create the shader
    SDL_GPUShaderCreateInfo shaderInfo = {
        codeSize,
        code,
        "main",
        SDL_GPU_SHADERFORMAT_SPIRV,
        stage,
        samplerCount,
        0, // storage textures
        0, // storage buffers
        uniformCount
    };
    
    SDL_GPUShader* shader = SDL_CreateGPUShader(context->device, &shaderInfo);
    if (!shader)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Create GPU shader failed (%s)",
                     SDL_GetError());
    }
    
    // Free the loaded code and return the shader
    SDL_free(code);
    return shader;
}

void
create_buffers(Context *context,
               Uint32 maxSizeVertex,
               Uint32 maxSizeIndex)
{
    // Create vertex buffer
    context->vertexBuf = SDL_CreateGPUBuffer(context->device,
                                             &(SDL_GPUBufferCreateInfo)
                                             {
                                                 SDL_GPU_BUFFERUSAGE_VERTEX,
                                                 maxSizeVertex,
                                                 0
                                             });
    
    // Create index buffer
    context->indexBuf = SDL_CreateGPUBuffer(context->device,
                                            &(SDL_GPUBufferCreateInfo)
                                            {
                                                SDL_GPU_BUFFERUSAGE_INDEX,
                                                maxSizeIndex,
                                                0
                                            });
    
    // Create transfer buffer for vertex data
    context->transBufVert =
        SDL_CreateGPUTransferBuffer(context->device,
                                    &(SDL_GPUTransferBufferCreateInfo)
                                    {
                                        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                        maxSizeVertex
                                    });
    
    // Create transfer buffer for index data
    context->transBufInd =
        SDL_CreateGPUTransferBuffer(context->device,
                                    &(SDL_GPUTransferBufferCreateInfo)
                                    {
                                        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                        maxSizeIndex
                                    });
}

void
update_buffers(Context *context,
               void *dataVert, Uint32 dataSizeVert,
               void *dataInd, Uint32 dataSizeInd)
{
    // Map and copy vertex data to GPU
    Vertex* destDataVert = SDL_MapGPUTransferBuffer(context->device,
                                                    context->transBufVert,
                                                    false);
    memcpy(destDataVert, dataVert, dataSizeVert);
    SDL_UnmapGPUTransferBuffer(context->device, context->transBufVert);
    
    // Map and copy index data to GPU
    Uint32* destDataInd = SDL_MapGPUTransferBuffer(context->device,
                                                   context->transBufInd,
                                                   false);
    memcpy(destDataInd, dataInd, dataSizeInd);
    SDL_UnmapGPUTransferBuffer(context->device, context->transBufInd);
    
    // Start command buffer and begin copy pass
    SDL_GPUCommandBuffer *cmdBuf = SDL_AcquireGPUCommandBuffer(context->device);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmdBuf);
    
    // Upload vertex data
    SDL_UploadToGPUBuffer(copyPass,
                          &(SDL_GPUTransferBufferLocation)
                          {
                              context->transBufVert, 0
                          },
                          &(SDL_GPUBufferRegion)
                          {
                              context->vertexBuf, 0, dataSizeVert
                          },
                          false);
    
    // Upload index data
    SDL_UploadToGPUBuffer(copyPass,
                          &(SDL_GPUTransferBufferLocation)
                          {
                              context->transBufInd, 0
                          },
                          &(SDL_GPUBufferRegion)
                          {
                              context->indexBuf, 0, dataSizeInd
                          },
                          false);
    
    
    // End pass and submit command buffer
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmdBuf);
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
    context->transBufTex =
        SDL_CreateGPUTransferBuffer(context->device,
                                    &(SDL_GPUTransferBufferCreateInfo)
                                    {
                                        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                        width * height * sizeof(Uint32)
                                    });
}

void
update_texture(Context *context,
               Uint32 width, Uint32 height,
               void *data)
{
    // Map and copy texture data to GPU
    Uint32* destData = SDL_MapGPUTransferBuffer(context->device,
                                                context->transBufTex,
                                                false);
    memcpy(destData, data, width * height * sizeof(Uint32));
    SDL_UnmapGPUTransferBuffer(context->device, context->transBufTex);
    
    // Start command buffer and begin copy pass
    SDL_GPUCommandBuffer *cmdBuf = SDL_AcquireGPUCommandBuffer(context->device);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmdBuf);
    
    SDL_UploadToGPUTexture(copyPass,
                           &(SDL_GPUTextureTransferInfo)
                           {
                               context->transBufTex,
                               0,
                               width,
                               height
                           },
                           &(SDL_GPUTextureRegion)
                           {
                               context->texture,
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
create_pipeline(Context *context)
{
    // Create the shaders
	SDL_GPUShader* vertexShader = shader_load(context,
                                              "vert.spv",
                                              SDL_GPU_SHADERSTAGE_VERTEX,
                                              0, 1);
    
	SDL_GPUShader* fragmentShader = shader_load(context,
                                                "frag.spv",
                                                SDL_GPU_SHADERSTAGE_FRAGMENT,
                                                1, 0);
	
    SDL_GPURasterizerState rasterizerState = { SDL_GPU_FILLMODE_FILL };
    
    // The vertex buffers array
    SDL_GPUVertexBufferDescription vertexBufferDescArray[] =
    {
        { 0, sizeof(Vertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0 }
    };
    
    // The vertex buffer input layout
    SDL_GPUVertexAttribute vertexAttribArray[] =
    {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0 },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float) * 2 },
        { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, sizeof(float) * 4 }
    };
    
    // The vertex input state (buffrs and layouts)
    SDL_GPUVertexInputState vertexInputState =
    {
        vertexBufferDescArray,
        SDL_arraysize(vertexBufferDescArray),
        vertexAttribArray,
        SDL_arraysize(vertexAttribArray)
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
		vertexShader,
		fragmentShader,
        vertexInputState,
		SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        rasterizerState,
        multisampleState,
        depthStencilState,
        targetInfo
    };
    
    context->pipeline = SDL_CreateGPUGraphicsPipeline(context->device,
                                                      &pipelineCreateInfo);
    if (!context->pipeline)
    {
        const char *error = SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", error);
    }
    
    assert(context->pipeline);
    
    // Clean up shader resources
    SDL_ReleaseGPUShader(context->device, vertexShader);
    SDL_ReleaseGPUShader(context->device, fragmentShader);
} 

int main(int argc, char *argv[])
{
    // Context struct and some helper variables
    Context context = {0};
    bool quit = false;
    bool minimized = false;
    float lastTime = 0;
    
    (void)argc;
    (void)argv;
    if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_Init failed (%s)",
                     SDL_GetError());
        return 1;
    }
    
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    SDL_Rect bounds;
    
    SDL_GetDisplayBounds(display, &bounds);
    //SDL_GetDisplayUsableBounds(display, &bounds);
    int winWidth = bounds.w;
    int winHeight = bounds.h;
    
    // Get primary window
    context.window = SDL_CreateWindow("Minimal SDL3 GPU Example",
                                      winWidth, winHeight,
                                      SDL_WINDOW_VULKAN);
    if (!context.window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to get primary window (%s)",
                     SDL_GetError());
    }
    
    SDL_SetWindowFullscreen(context.window, true);
    
    // Create GPU Device
    context.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, 0);
    if (!context.device)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to create GPU device (%s)",
                     SDL_GetError());
    }
    
    // Associate window with GPU Device
    if (!SDL_ClaimWindowForGPUDevice(context.device, context.window))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to associate window with GPU device (%s)",
                     SDL_GetError());
    }
    
    // Create pipeline
    create_pipeline(&context);
    
    Uint32 maxQuadCount = 4096;
    
    create_buffers(&context,
                   sizeof(Vertex) * 4 * maxQuadCount,
                   sizeof(Uint32) * 6 * maxQuadCount);
    
    // Sampler
    context.sampler = SDL_CreateGPUSampler(context.device,
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
    
    // Texture
    Uint32 texWidth = 2;
    Uint32 texHeight = 2;
    create_texture(&context, texWidth, texHeight);
    
    Uint32 texData[] =
    {
        0xFF00FF00, 0xFFFF0000,
        0xFFFF0000, 0xFF00FF00
    };
    
    update_texture(&context, texWidth, texHeight, texData);
    
    
    
    // Update and render loop
    while (!quit)
    {
        float lastTouchX;
        float lastTouchY;
        
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
                
                case SDL_EVENT_TERMINATING:
                {
                    // Handle Android termination properly (e.g., save game state)
                    quit = true;
                } break;
                
                case SDL_EVENT_FINGER_DOWN:
                {
                    lastTouchX = evt.tfinger.x * winWidth;
                    lastTouchY = evt.tfinger.y * winHeight;
                } break;
                
                case SDL_EVENT_FINGER_MOTION:
                {
                    lastTouchX = evt.tfinger.x * winWidth;
                    lastTouchY = evt.tfinger.y * winHeight;
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
            
            // Vertex data
            float s = 100.0f;
            Vertex vertices[] =
            {
                {lastTouchX, lastTouchY,          0,0,    1.0f, 1.0f, 1.0f, 1.0f},
                {lastTouchX + s, lastTouchY,      1,0,    1.0f, 1.0f, 1.0f, 1.0f},
                {lastTouchX + s, lastTouchY + s,  1,1,    1.0f, 1.0f, 1.0f, 1.0f},
                {lastTouchX, lastTouchY + s,      0,1,    1.0f, 1.0f, 1.0f, 1.0f}
            };
            
            size_t vertexSize = sizeof(vertices);
            
            // Index data
            Uint32 indices[] = 
            {
                0, 1, 2,
                2, 3, 0
            };
            
            update_buffers(&context,
                           vertices, sizeof(vertices),
                           indices, sizeof(indices));
            
            
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
                
                // Setup the color target as the swapchain image
                SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
                colorTargetInfo.texture = swapchainTexture;
                colorTargetInfo.clear_color = clearColor;
                colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
                colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
                
                // Begin a render pass
                SDL_GPURenderPass* renderPass =
                    SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
                
                // Update uniform
                float matrix[] =
                {
                    2.0f / (float)winWidth, 0, 0, -1,
                    0, -2.0f / (float)winHeight, 0, 1,
                    0, 0, 1, 0,
                    0, 0, 0, 1
                };
                SDL_PushGPUVertexUniformData(cmdbuf,
                                             0,
                                             matrix,
                                             sizeof(matrix));
                
                // Bind our graphics pipeline and draw a triangle
                SDL_BindGPUGraphicsPipeline(renderPass, context.pipeline);
                
                // Texture and Sampler
                SDL_BindGPUFragmentSamplers(renderPass,
                                            0, // first slot
                                            &(SDL_GPUTextureSamplerBinding)
                                            {
                                                context.texture,
                                                context.sampler
                                            },
                                            1);
                
                // Buffers
                SDL_BindGPUVertexBuffers(renderPass, 0,
                                         &(SDL_GPUBufferBinding){ context.vertexBuf, 0 },
                                         1);
                
                SDL_BindGPUIndexBuffer(renderPass,
                                       &(SDL_GPUBufferBinding){ context.indexBuf, 0 },
                                       SDL_GPU_INDEXELEMENTSIZE_32BIT);
                
                SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
                
                // End the render pass
                SDL_EndGPURenderPass(renderPass);
                
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
            SDL_Delay((Uint32)(1000.0f / 60));
        }
    }
    
    // Clean up resources, even though I think these are not needed anyway
    SDL_ReleaseGPUGraphicsPipeline(context.device, context.pipeline);
    SDL_ReleaseWindowFromGPUDevice(context.device, context.window);
    SDL_DestroyWindow(context.window);
    SDL_Quit();
    return 0;
}
