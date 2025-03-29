#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <assert.h>

typedef struct
{
	char *basePath;
	SDL_Window* window;
	SDL_GPUDevice* device;
	float deltaTime;
    SDL_GPUGraphicsPipeline* pipeline;

} Context;

SDL_GPUShader*
shader_load(Context* context, char* shaderFilename, SDL_GPUShaderStage stage)
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
        .code = code,
        .code_size = codeSize,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage
    };
    
    SDL_GPUShader* shader = SDL_CreateGPUShader(context->device, &shaderInfo);
    assert(shader);
    
    // Free the code memory and return the shader
    SDL_free(code);
    return shader;
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
    context.window = SDL_CreateWindow("Minimal SDL3 GPU Example", 800, 600,
                                      SDL_WINDOW_VULKAN);
    assert(context.window);
    
    // Associate window with GPU Device
    assert(SDL_ClaimWindowForGPUDevice(context.device, context.window));
    
    // Create the shaders
	SDL_GPUShader* vertexShader = shader_load(&context, "shaders/vert.spv",
                                              SDL_GPU_SHADERSTAGE_VERTEX);
    
	SDL_GPUShader* fragmentShader = shader_load(&context, "shaders/frag.spv",
                                                SDL_GPU_SHADERSTAGE_FRAGMENT);
	
    // Create the pipeline based on shaders
	SDL_GPUTextureFormat textureFormat =
        SDL_GetGPUSwapchainTextureFormat(context.device, context.window);
    
    SDL_GPURasterizerState rasterState =
    {
        .fill_mode = SDL_GPU_FILLMODE_FILL
    };
    
    SDL_GPUColorTargetDescription colorTargetDescArray[] =
    {
        { .format = textureFormat }
    };
    
    SDL_GPUGraphicsPipelineTargetInfo targetInfo =
    {
        .color_target_descriptions = colorTargetDescArray,
        .num_color_targets = SDL_arraysize(colorTargetDescArray),
    };
    
	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo =
    {
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = rasterState,
        .target_info = targetInfo,
    };
    
	context.pipeline = SDL_CreateGPUGraphicsPipeline(context.device,
                                                     &pipelineCreateInfo);
	assert(context.pipeline);
	
	// Clean up shader resources
	SDL_ReleaseGPUShader(context.device, vertexShader);
	SDL_ReleaseGPUShader(context.device, fragmentShader);
    
    // Update and render loop
    while (!quit)
    {
        // Poll events
        SDL_Event evt;
		while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
            else if (evt.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                minimized = true;
            }
            else if (evt.type == SDL_EVENT_WINDOW_RESTORED)
            {
                minimized = false;
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
                
                // Bind our graphics pipeline and draw a triangle
                SDL_BindGPUGraphicsPipeline(renderPass, context.pipeline);
                SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);                
                
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
	SDL_DestroyGPUDevice(context.device);
    
    return 0;
}