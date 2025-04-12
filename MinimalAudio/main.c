#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

typedef struct
{
    SDL_AudioSpec spec;
    SDL_AudioStream *stream;
    Uint8 *buffer;
    Uint32 length;
    
} Audio;

bool
sdl3_init(SDL_Window **window,
          SDL_Renderer **renderer)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        if (SDL_CreateWindowAndRenderer("test audio",
                                        640, 480,
                                        0,
                                        window,
                                        renderer))
        {
            return true;
        }
        else
        {
            SDL_Log("Couldn't create window/renderer: %s",
                    SDL_GetError());
        }
    }
    else
    {
        SDL_Log("Couldn't initialize SDL: %s",
                SDL_GetError());
    }
    
    return false;
}

void
audio_free(Audio *audio)
{
    SDL_DestroyAudioStream(audio->stream);
    audio->stream = 0;
}

bool
audio_init(char *fileName,
           SDL_AudioDeviceID deviceID,
           Audio *destAudio)
{
    if (SDL_LoadWAV(fileName,
                    &destAudio->spec,
                    &destAudio->buffer,
                    &destAudio->length))
    {
        destAudio->stream =
            SDL_OpenAudioDeviceStream(deviceID,
                                      &destAudio->spec,
                                      0, 0);
        
        if (destAudio->stream)
        {
            if (SDL_ResumeAudioStreamDevice(destAudio->stream))
            {
                return true;
            }
            else
            {
                SDL_Log("Couldn't play the audio stream: %s",
                        SDL_GetError());
            }
        }
        else
        {
            SDL_Log("Couldn't open audio stream: %s",
                    SDL_GetError());
        }
    }
    else
    {
        SDL_Log("Couldn't load test WAV file: %s",
                SDL_GetError());
    }
    
    return false;
}

int
main(int argc, char **argv)
{
    SDL_Window *window = 0;
    SDL_Renderer *renderer = 0;
    SDL_AudioDeviceID audioDeviceID = 0;
    SDL_AudioSpec audioSpec = {0};
    Audio audio = {0};
    bool running = true;
    bool spaceWasDown = false;
    
    // Init SDL3 and create Window and Renderer
    if (!sdl3_init(&window, &renderer))
    {
        return 1;
    }
    
    // Define audio type (stereo 44100Hz signed 16 bits)
    audioSpec = (SDL_AudioSpec){ SDL_AUDIO_S16LE, 2, 44100 };
    
    // Get audio device ID
    audioDeviceID =
        SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                            &audioSpec);
    if (!audioDeviceID)
    {
        SDL_Log("Couldn't open audio device ID: %s",
                SDL_GetError());
        return 2;
    }
    
    if (!audio_init("test.wav",
                    audioDeviceID,
                    &audio))
    {
        return 3;
    }
    
    while (running)
    {
        SDL_Event evt = {0};
        bool spacePressed = false;
        const bool *keyStates = 0;
        
        // Poll events
        while (SDL_PollEvent(&evt))
        {
            switch (evt.type)
            {
                case SDL_EVENT_QUIT:
                {
                    running = false;
                } break;
            }
        }
        
        // Get keyboard states
        keyStates = SDL_GetKeyboardState(0);
        
        // Process space key to react to it being pressed
        if (keyStates[SDL_SCANCODE_SPACE])
        {
            if (!spaceWasDown)
            {
                spacePressed = true;
            }
            
            spaceWasDown = true;
        }
        else
        {
            spaceWasDown = false;
        }
        
        // If the space key is pressed (only happens once per press)
        if (spacePressed)
        {
            // Stop audio currently playing in stream
            SDL_ClearAudioStream(audio.stream);
            
            // Play audio in stream
            if (SDL_PutAudioStreamData(audio.stream,
                                       audio.buffer,
                                       audio.length) == -1)
            {
                printf("Failed to put samples: %s\n", SDL_GetError());
                return 6;
            }
        }
        
        // Clear screen and present it (draws nothing)
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }
    
    // Free stuff (even though it's not necessary)
    audio_free(&audio);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    return 0;
}