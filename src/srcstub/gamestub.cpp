#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include "gamestub.h"
#include "gamestubcallbacks.h"
#include "pd_api.h"
#include "defines.h"
#include "debug.h"
#include <math.h>
#include <thread>
#include <chrono>

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

void _pd_api_sys_fullScreenCallBack()
{
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

void _pd_api_sys_renderResetCallBack()
{
    SDL_Log("Render Reset\n");
}

void _pd_api_sys_quitCallBack()
{
    stubDoQuit = true;
}

//https://blog.bearcats.nl/accurate-sleep-function/
void preciseSleep(double seconds) {
    using namespace std;
    using namespace chrono;

    static double estimate = 5e-3;
    static double mean = 5e-3;
    static double m2 = 0;
    static int64_t count = 1;

    while (seconds > estimate) {
        auto start = high_resolution_clock::now();
        this_thread::sleep_for(milliseconds(1));
        auto end = high_resolution_clock::now();

        double observed = (end - start).count() / 1e9;
        seconds -= observed;

        ++count;
        double delta = observed - mean;
        mean += delta / count;
        m2   += delta * (observed - mean);
        double stddev = sqrt(m2 / (count - 1));
        estimate = mean + stddev;
    }

    // spin lock
    auto start = high_resolution_clock::now();
    while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
}

int main(int argv, char** args)
{
	bool useSoftwareRenderer = true;
    bool useLinear = false;
    bool useVsync = false;
    bool useFullScreenAtStartup = false;
    bool useOpenGLHint = false;
    int c;
    while ((c = getopt(argv, args, "?lafvo")) != -1) 
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
  -a: Use Acclerated Renderer\n\
  -f: Run fullscreen at startup (by default starts up windowed)\n\
  -o: Set OpenGL Hint in accelerated mode, so it will try to use opengl\n\
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
                SDL_SetRenderDrawColor(Renderer, 0xFF, 0xFF, 0xFF, 0xFF);
                SDL_RenderClear(Renderer);
                SDL_RenderPresent(Renderer);

                if(TTF_Init() == 0)
                {
                    SDL_Log("Succesfully Initialized SDL_TTF\n");

                    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) == 0) 
                    {
                        SDL_Log("Succesfully Initialized SDL_MIXER\n");
                        //initialiaze playdate api
                        Api = (PlaydateAPI*) malloc(sizeof(*Api));
                        Api->system = pd_api_sys_Create_playdate_sys();
                        Api->display = pd_api_display_Create_playdate_display();
                        Api->graphics = pd_api_gfx_Create_playdate_graphics();
                        Api->sound = pd_api_sound_Create_playdate_sound();
                        Api->file = pd_api_file_Create_playdate_file();
                        Api->sprite = pd_api_sprite_Create_playdate_sprite();
                        Api->lua = pd_api_lua_Create_playdate_lua();
                        Api->scoreboards = pd_api_scoreboards_Create_playdate_scoreboards();
                        Api->json = pd_api_json_Create_playdate_json();

                        //initialize handler
                        eventHandler(Api, kEventInit, 0);

                        Uint64 fpslogticks = SDL_GetPerformanceCounter();
                        stubDoQuit = false;
                        Uint8 r,g,b,a;
                        SDL_Texture* target;
                        double elapsed = 0.0;
                        LCDColor bgColor;
                        Uint64 _FrameCount = 0;
                        double _TotalFps = 0.0;
						SDL_Texture* TexScreen = SDL_CreateTexture(Renderer, pd_api_gfx_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, LCD_COLUMNS, LCD_ROWS);

                        while(!stubDoQuit)
                        {
                            Uint64 StartTicks = SDL_GetPerformanceCounter();
                            _pd_api_sys_UpdateInput();
                            //run the update function    
                            _pd_api_sys_DoUpdate(_pd_api_sys_DoUpdateuserdata);
                            
                            //clean sprites set as not loaded (delayed delete to prevent issues)
                            _pd_api_sprite_cleanup_sprites(true);
                            
                            //clear fullbackground of the real screen (in case display offset is used)
                            target = SDL_GetRenderTarget(Renderer);
                                            
                            SDL_SetRenderTarget(Renderer, NULL);
                            bgColor = getBackgroundDrawColor();
                            SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
							
							if(bgColor == kColorClear)
							{
								if (_DisplayInverted)
									bgColor = kColorBlack;
								else
									bgColor = kColorWhite;
							}
							else
							{
								if (_DisplayInverted)
								{
									if(bgColor == kColorBlack)
										bgColor = kColorWhite;
									else
										if(bgColor == kColorWhite)
											bgColor = kColorBlack;
								}
							}

                            switch(bgColor)
                            {                       
                                case kColorBlack:
									SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_black.r, pd_api_gfx_color_black.g, pd_api_gfx_color_black.b, pd_api_gfx_color_black.a);
                                    break;
                                case kColorWhite:
                                    SDL_SetRenderDrawColor(Renderer, pd_api_gfx_color_white.r, pd_api_gfx_color_white.g, pd_api_gfx_color_white.b, pd_api_gfx_color_white.a);
                                    break;
                            }

                            SDL_RenderClear(Renderer);
                            SDL_SetRenderDrawColor(Renderer, r, g, b, a);

                                                
                            //draw the fake screen to the real screen but loop it through custom made _pd_api_gfx_drawBitmapAll
                            //so that clear pixels (cyan's) are made transparant. 
                            //we clear the background of the real screen surface with either white or black
                            //bgColor = getBackgroundDrawColor();
                            LCDBitmap * screen = Api->graphics->newBitmap(LCD_COLUMNS, LCD_ROWS, _DisplayInverted? kColorBlack:kColorWhite);//(LCDSolidColor)bgColor == kColorClear ? _DisplayInverted? kColorBlack:kColorWhite: (LCDSolidColor)bgColor);
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
                            _pd_api_gfx_drawBitmapAll(playdatescreen, 0, 0, _DisplayScale, _DisplayScale, false, 0, 0, 0, flip);
                            Api->graphics->popContext();

                            //now we just need to blit this bitmap to the screen, but i could not access screen->tex so added another function
                            //to get the sdl texture                  
                            SDL_SetRenderTarget(Renderer, NULL);
                            SDL_Surface* Tex = _pd_api_gfx_GetSDLTextureFromBitmap(screen);
							SDL_UpdateTexture(TexScreen, NULL, Tex->pixels, Tex->pitch);
                            SDL_Rect Dest;                    
                            Dest.x = _DisplayOffsetDisplayX;
                            Dest.y = _DisplayOffsetDisplayY;
                            Dest.w = LCD_COLUMNS;
                            Dest.h = LCD_ROWS;
                            SDL_RenderCopy(Renderer, TexScreen, NULL, &Dest);
							SDL_RenderPresent(Renderer);

                            Api->graphics->freeBitmap(screen);

                            //restore render target
                            SDL_SetRenderTarget(Renderer, target);

							Api->graphics->clearClipRect();

                            //for calculating avergage
                            _FrameCount++;

                            //calculate fps & delay
                            elapsed = (((double)SDL_GetPerformanceCounter() - (double)StartTicks) / (double)SDL_GetPerformanceFrequency()) * 1000.0;
                            if ((_DisplayDesiredDelta != 0.0) && (elapsed < _DisplayDesiredDelta))
                            {
                                //SDL_Delay((Uint32)(_DisplayDesiredDelta - elapsed));
								preciseSleep((_DisplayDesiredDelta - elapsed) / 1000.0);
                            }

							elapsed = ((double)SDL_GetPerformanceCounter() - (double)StartTicks) / (double)SDL_GetPerformanceFrequency();
                            if(elapsed != 0.0)
                            {
                                _CurrentFps = 1.0 / elapsed;
                                _TotalFps += _CurrentFps;
                                _AvgFps = _TotalFps / (double)_FrameCount;
                            }
                            
                            if (((SDL_GetPerformanceFrequency() + fpslogticks)) < SDL_GetPerformanceCounter())
                            {  
                                _TotalFps = 0.0;
                                _FrameCount = 0;
                                _LastFPS = _AvgFps;
                                printfDebug(DebugFPS, "%2f\n", _LastFPS);
                                fpslogticks = SDL_GetPerformanceCounter();
                            }
                        }
                        //erase remaining sprites in memory
                        _pd_api_sprite_cleanup_sprites(false);

                        //free samples & sounds
                        _pd_api_sound_freeSampleList();

                        //free Api
                        free((void *)Api->system);
                        free((void *)Api->display); 
                        free((void *)Api->graphics->video);
                        free((void *)Api->graphics);
                        free((void *)Api->sound->source);
                        free((void *)Api->sound->fileplayer);
                        free((void *)Api->sound->sample);
                        free((void *)Api->sound->sampleplayer);
                        free((void *)Api->sound->signal);
                        free((void *)Api->sound->lfo);
                        free((void *)Api->sound->envelope);
                        free((void *)Api->sound->synth);
                        free((void *)Api->sound->controlsignal);
                        free((void *)Api->sound->instrument);
                        free((void *)Api->sound->track);
                        free((void *)Api->sound->sequence);
                        free((void *)Api->sound->channel);
                        free((void *)Api->sound->effect->bitcrusher);
                        free((void *)Api->sound->effect->delayline);
                        free((void *)Api->sound->effect->onepolefilter);
                        free((void *)Api->sound->effect->twopolefilter);
                        free((void *)Api->sound->effect->ringmodulator);
                        free((void *)Api->sound->effect->overdrive);
                        free((void *)Api->sound->effect);
                        free((void *)Api->sound);
                        free((void *)Api->file);
                        free((void *)Api->sprite);
                        free((void *)Api->lua);
                        free((void *)Api->scoreboards);
                        free((void *)Api->json);
						SDL_DestroyTexture(TexScreen);

                        Mix_CloseAudio();
                    }
                    else
                    {
                        SDL_Log("Failed to initialize SDL Mixer: %s\n", Mix_GetError());
                    }
                    _pd_api_gfx_freeFontList();
                    TTF_Quit();
                    SDL_DestroyRenderer(Renderer);
                    free(Api);
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
