#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "gamestub.h"
#include "gamestubcallbacks.h"
#include "pd_api.h"

SDL_Window *SdlWindow;
SDL_Renderer *Renderer;
PlaydateAPI *Api;

int audio_rate = 44100; 
Uint16 audio_format = AUDIO_S16SYS; 
int audio_channels = 2; 
int audio_buffers = 256;


//mapped value of clear must lie between black and white and its value may not be bigger than whitethreshold or lower than blackthreshold

SDL_Color  pd_api_gfx_color_clear = {128, 255, 255, SDL_ALPHA_OPAQUE};
SDL_Color  pd_api_gfx_color_white = {255, 255, 255, SDL_ALPHA_OPAQUE};
SDL_Color  pd_api_gfx_color_black = {0, 0, 0, SDL_ALPHA_OPAQUE};
SDL_Color  pd_api_gfx_color_whitetreshold = {155, 155, 155, SDL_ALPHA_OPAQUE};
SDL_Color  pd_api_gfx_color_blacktreshold = {100, 100, 100, SDL_ALPHA_OPAQUE};

bool stubDoQuit;
SDL_PixelFormatEnum pd_api_gfx_PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;

void fullScreenCallBack()
{
    //return;
    Uint32 FullscreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;
    bool IsFullscreen = SDL_GetWindowFlags(SdlWindow) & FullscreenFlag;
    // reset window size first before we go fullscreen
    // it will give more fps on the rpi if we for example scaled the window
    // first    
    //if (!IsFullscreen)
    //  SDL_SetWindowSize(SdlWindow, LCD_COLUMNS, LCD_ROWS);
    SDL_SetWindowFullscreen(SdlWindow, IsFullscreen ? 0 : FullscreenFlag);
    IsFullscreen = SDL_GetWindowFlags(SdlWindow) & FullscreenFlag;
    if (!IsFullscreen) 
    {
        //SDL_SetWindowSize(SdlWindow, LCD_COLUMNS, LCD_ROWS);
        SDL_SetWindowPosition(SdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

void renderResetCallBack()
{
    SDL_Log("Render Reset\n");
}

void quitCallBack()
{
    stubDoQuit = true;
}

int main(int argc, char *argv[])
{
	bool useSoftwareRenderer = true;
    bool useLinear = false;
    bool useVsync = false;
    bool useFullScreenAtStartup = false;
    bool useOpenGLHint = false;
    int c;
    while ((c = getopt(argc, argv, "?lafvo")) != -1) 
    {
        switch (c) 
        {
            case '?':
                // i use sdl log because on windows printf did not show up
                SDL_Log("\nPlaydate Game Stub\n\n\
Usage: Game.exe [Options]\n\n\
Possible options are:\n\
  -?: show this help message\n\
  -l: enable linear filtering (only works when hardware renderer is used)\n\
  -a: Use Acclerated Renderer (can cause issues with Directx like blackscreen on window resizes)\n\
  -u: When using Acclerated Renderer set hint to use OpenGl\n\
  -f: Run fullscreen at startup (by default starts up windowed)\n\
  -v: Use VSync\n");
                exit(0);
                break;
            case 'l':
                useLinear = true;
                break;
            case 'a':
                useSoftwareRenderer = false;
                break;
            case 'f':
                useFullScreenAtStartup = true;
                break;
            case 'o':
                useOpenGLHint = true;
                break;
            case 'v':
                useVsync = true;
                break;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) == 0) 
    {
        Uint32 WindowFlags = SDL_WINDOW_RESIZABLE;
        if (useFullScreenAtStartup) {
            WindowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;            
        }

        SdlWindow = SDL_CreateWindow("Playdate Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LCD_COLUMNS, LCD_ROWS, WindowFlags);
        
        if (SdlWindow) 
        {
            Uint32 flags = 0;
            if (useSoftwareRenderer) 
                flags |= SDL_RENDERER_SOFTWARE;
            else
            {
                flags |= SDL_RENDERER_ACCELERATED;
                if (useOpenGLHint)
                    ////try opengl first so we don't loose device context when resizing the window
                    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");                                 
            }
            if (useVsync)
                flags |= SDL_RENDERER_PRESENTVSYNC;

            SDL_Log("Succesfully Set %dx%d\n", LCD_COLUMNS, LCD_ROWS);
           
            
            Renderer = SDL_CreateRenderer(SdlWindow, -1, flags);
            if (Renderer) 
            {
                SDL_RendererInfo rendererInfo;
                SDL_GetRendererInfo(Renderer, &rendererInfo);
                SDL_Log("Using Renderer:%s\n", rendererInfo.name);

                if (useLinear)
                    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

                SDL_RenderSetLogicalSize(Renderer, LCD_COLUMNS, LCD_ROWS);
                SDL_Log("Succesfully Created Buffer\n");

                SDL_SetRenderTarget(Renderer, NULL);
                SDL_SetRenderDrawColor(Renderer, 0x00, 0x00, 0x00, 0xFF);
                SDL_RenderClear(Renderer);
                SDL_RenderPresent(Renderer);

                if(TTF_Init() == 0)
                {
                    SDL_Log("Succesfully Initialized SDL_TTF\n");

                    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) == 0) 
                    {
                        SDL_Log("Succesfully Initialized SDL_MIXER\n");
                        printf("Allocated %d individual channels for playblack\n", Mix_AllocateChannels(pd_api_sound_MaxSimSounds));
                        //initialiaze playdate api
                        Api = (PlaydateAPI*) malloc(sizeof(*Api));
                        Api->system = pd_api_sys_Create_playdate_sys();
                        Api->display = pd_api_display_Create_playdate_display();
                        Api->graphics = pd_api_gfx_Create_playdate_graphics();
                        Api->sound = pd_api_sound_Create_playdate_sound();
                        Api->file = pd_api_file_Create_playdate_file();

                        //initialize handler
                        eventHandler(Api, kEventInit, 0);

                        Uint64 Delta = 0;
                        Uint64 Delta2 = 0;
                        Uint64 fpslogticks = SDL_GetPerformanceCounter();
                        stubDoQuit = false;
                        Uint8 r,g,b,a;
                        SDL_Texture* target;
                        double elapsed = 0.0;
                        LCDColor bgColor;
                        while(!stubDoQuit)
                        {
                            Uint64 StartTicks = SDL_GetPerformanceCounter();
                            _pd_api_sys_UpdateInput();
                            //run the update function    
                            DoUpdate(DoUpdateuserdata);
                            
                            //clear fullbackground of the real screen (in case display offset is used)
                            target = SDL_GetRenderTarget(Renderer);
                                            
                            SDL_SetRenderTarget(Renderer, NULL);
                            bgColor = getBackgroundDrawColor();
                            SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
                            switch(bgColor)
                            {                       
                                case kColorWhite:
                                    SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
                                    break;
                                case kColorClear:
                                    if(_DisplayInverted)
                                        SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
                                    else
                                        SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
                                    break;
                                case kColorBlack:
                                    SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
                                    break;
                            }
                            SDL_RenderClear(Renderer);
                            SDL_SetRenderDrawColor(Renderer, r, g, b, a);

                                                
                            //draw the fake screen to the real screen but loop it through custom made _pd_api_gfx_drawBitmapAll
                            //so that clear pixels (cyan's) are made transparant. 
                            //we clear the background of the real screen surface with either white or black
                            //bgColor = getBackgroundDrawColor();
                            LCDBitmap * screen = Api->graphics->newBitmap(LCD_COLUMNS, LCD_ROWS, kColorClear);
                            //get the playdate screen bitmap
                            LCDBitmap * playdatescreen = Api->graphics->getDisplayBufferBitmap();
                            //and push it as a context so we can pop it later
                            Api->graphics->pushContext(screen);
                            //also reset all current option to normal
                            //so it does not affect this screen bitmap drawing
                            Api->graphics->setDrawMode(kDrawModeCopy);
                            Api->graphics->clearClipRect();
                            Api->graphics->setDrawOffset(0,0);
                            //apply inverted mode
                            if (_DisplayInverted)
                            {
                                Api->graphics->setDrawMode(kDrawModeInverted);
                            }
                            //set flip mode
                            LCDBitmapFlip flip = kBitmapUnflipped;
                            if (_DisplayFlippedX && _DisplayFlippedY)
                            {
                                flip = kBitmapFlippedXY;
                            }
                            else
                            {
                                if (_DisplayFlippedX)
                                {
                                    flip = kBitmapFlippedX;
                                }
                                else
                                {
                                    if(_DisplayFlippedY)
                                    {
                                        flip = kBitmapFlippedY;
                                    }
                                }
                            }
                            //draw using the newly added all around function
                            _pd_api_gfx_drawBitmapAll(playdatescreen, 0, 0, _DisplayScale, _DisplayScale, 0, 0, 0, flip);
                            Api->graphics->popContext();

                            //now we just need to blit this bitmap to the screen, but i could not access screen->tex so added another function
                            //to get the sdl texture                  
                            SDL_SetRenderTarget(Renderer, NULL);
                            SDL_Texture* Tex = _pd_api_gfx_GetSDLTextureFromBitmap(screen);
                            SDL_Rect Dest;                    
                            Dest.x = _DisplayOffsetDisplayX;
                            Dest.y = _DisplayOffsetDisplayY;
                            Dest.w = LCD_COLUMNS;
                            Dest.h = LCD_ROWS;
                            SDL_RenderCopy(Renderer, Tex, NULL, &Dest);
                            SDL_RenderPresent(Renderer);

                            Api->graphics->freeBitmap(screen);

                            //restore render target
                            SDL_SetRenderTarget(Renderer, target);

                            //calculate fps & delay
                            elapsed = (((double)SDL_GetPerformanceCounter() - (double)StartTicks) / (double)SDL_GetPerformanceFrequency()) * 1000.0;
                            if ((_DisplayDesiredDelta != 0.0) && (elapsed < _DisplayDesiredDelta))
                            {
                                SDL_Delay((Uint32)(_DisplayDesiredDelta - (elapsed)));
                            }

                            elapsed = ((double)SDL_GetPerformanceCounter() - (double)StartTicks) / (double)SDL_GetPerformanceFrequency();
                            if(elapsed != 0.0)
                                _CurrentFps = 1 / elapsed;
                            
                            if (((SDL_GetPerformanceFrequency() + fpslogticks)) < SDL_GetPerformanceCounter())
                            {                        
                                printf("%2f\n", _CurrentFps);
                                fpslogticks = SDL_GetPerformanceCounter();
                            }                    
                        }
                        Mix_CloseAudio();
                    }
                    else
                    {
                        SDL_Log("Failed to initialize SDL Mixer: %s\n", Mix_GetError());
                    }
                    TTF_Quit();
                    SDL_DestroyRenderer(Renderer);
                } 
                else 
                {
                    SDL_Log("Failed to initialize SDL TTF: %s\n", TTF_GetError());
                }
            }
            else 
            {
                SDL_Log("Failed to create renderer: %s\n", SDL_GetError());    
            }

        } 
        else 
        {
            SDL_Log("Failed to Set Videomode (%dx%d): %s\n", LCD_COLUMNS, LCD_ROWS, SDL_GetError());
        }
        SDL_DestroyWindow(SdlWindow);
        SDL_Quit();
    } 
    else 
        SDL_Log("Failed to initialise SDL: %s\n", SDL_GetError());
    
    return 0;
}