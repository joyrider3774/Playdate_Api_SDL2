#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <sys/stat.h>
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
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "pdxinfo_parser.h"

SDL_Window *SdlWindow;
SDL_Renderer *Renderer;
PlaydateAPI *Api;



Uint64 pd_api_fpslogticks;
Uint64 pd_api_FrameCount;
double pd_api_TotalFps;
double pd_api_elapsed;


//mapped value of clear must lie between black and white and its value may not be bigger than whitethreshold or lower than blackthreshold

SDL_Color  pd_api_gfx_color_clear = {128, 255, 255, SDL_ALPHA_OPAQUE};
SDL_Color  pd_api_gfx_color_white = {177, 175, 168, SDL_ALPHA_OPAQUE};
SDL_Color  pd_api_gfx_color_black = {49, 47, 40, SDL_ALPHA_OPAQUE};
SDL_Color  pd_api_gfx_color_whitetreshold = {155, 155, 155, SDL_ALPHA_OPAQUE};
SDL_Color  pd_api_gfx_color_blacktreshold = {100, 100, 100, SDL_ALPHA_OPAQUE};

bool stubDoQuit;

SDL_PixelFormatEnum pd_api_gfx_PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;
SDL_Texture* _pd_api_TexScreen;

const int MAXSOURCEDIRS = 6;

const char *_pd_alternate_source_dirs[MAXSOURCEDIRS] = {".", "Source1", "Source2", "Source3", "Source4", "Source5"};
int _pd_current_source_dir = 0;
bool _pd_do_load_next_source_dir = false;

void _pd_load_source_colors()
{
	pd_api_gfx_color_clear = {128, 255, 255, SDL_ALPHA_OPAQUE};
	pd_api_gfx_color_white = {177, 175, 168, SDL_ALPHA_OPAQUE};
	pd_api_gfx_color_black = {49, 47, 40, SDL_ALPHA_OPAQUE};
	pd_api_gfx_color_whitetreshold = {155, 155, 155, SDL_ALPHA_OPAQUE};
	pd_api_gfx_color_blacktreshold = {100, 100, 100, SDL_ALPHA_OPAQUE};
	
	FILE *fp;
	char Filename[MAXPATH];
	sprintf(Filename, "./%s/colors.ini", _pd_api_get_current_source_dir());
	fp = fopen(Filename, "r");
	if (fp)
	{
		fscanf(fp, "pd_api_gfx_color_clear_r=%hhd\n", &pd_api_gfx_color_clear.r);
		fscanf(fp, "pd_api_gfx_color_clear_g=%hhd\n", &pd_api_gfx_color_clear.g);
		fscanf(fp, "pd_api_gfx_color_clear_b=%hhd\n", &pd_api_gfx_color_clear.b);
		
		fscanf(fp, "pd_api_gfx_color_white_r=%hhd\n", &pd_api_gfx_color_white.r);
		fscanf(fp, "pd_api_gfx_color_white_g=%hhd\n", &pd_api_gfx_color_white.g);
		fscanf(fp, "pd_api_gfx_color_white_b=%hhd\n", &pd_api_gfx_color_white.b);
		
		fscanf(fp, "pd_api_gfx_color_black_r=%hhd\n", &pd_api_gfx_color_black.r);
		fscanf(fp, "pd_api_gfx_color_black_g=%hhd\n", &pd_api_gfx_color_black.g);
		fscanf(fp, "pd_api_gfx_color_black_b=%hhd\n", &pd_api_gfx_color_black.b);

		fscanf(fp, "pd_api_gfx_color_whitetreshold_r=%hhd\n", &pd_api_gfx_color_whitetreshold.r);
		fscanf(fp, "pd_api_gfx_color_whitetreshold_g=%hhd\n", &pd_api_gfx_color_whitetreshold.g);
		fscanf(fp, "pd_api_gfx_color_whitetreshold_b=%hhd\n", &pd_api_gfx_color_whitetreshold.b);

		fscanf(fp, "pd_api_gfx_color_blacktreshold_r=%hhd\n", &pd_api_gfx_color_blacktreshold.r);
		fscanf(fp, "pd_api_gfx_color_blacktreshold_g=%hhd\n", &pd_api_gfx_color_blacktreshold.g);
		fscanf(fp, "pd_api_gfx_color_blacktreshold_b=%hhd\n", &pd_api_gfx_color_blacktreshold.b);
		fclose(fp);
	}
}

void _pd_reset()
{
	eventHandler(Api, kEventTerminate, 0);
	//erase remaining sprites in memory
    _pd_api_sprite_cleanup_sprites(false);

	Mix_HaltChannel(-1);
	Mix_HaltMusic();
    //free samples & sounds
    _pd_api_sound_freeSampleList();
	_pd_api_gfx_freeFontList();
	_pd_load_source_colors();
	_pd_api_gfx_loadDefaultFonts();
	_pd_api_gfx_resetContext();
	eventHandler(Api, kEventInit, 0);
}

void _pd_api_sys_nextSourceDirCallback()
{
	_pd_do_load_next_source_dir = true;
}

void _pd_save_source_dir()
{
	SDFile *File;
	File = Api->file->open(".sdl2api.dat", kFileWrite);
	if (File)
	{
		Api->file->write(File, &_pd_current_source_dir, sizeof(_pd_current_source_dir));
		Api->file->close(File);
	}
}

void _pd_validate_source_dir()
{
	char filename[MAXPATH];
	sprintf(filename, "./%s", _pd_api_get_current_source_dir());
	struct stat lstats;
	if(stat(filename, &lstats) != 0)
		_pd_current_source_dir = 0;
}

void _pd_load_source_dir()
{
	SDFile *File;
	File = Api->file->open(".sdl2api.dat", kFileReadData);
	if (File)
	{
		Api->file->read(File, &_pd_current_source_dir, sizeof(_pd_current_source_dir));
		Api->file->close(File);
	}
	else
	{
		_pd_current_source_dir = DEFAULTSOURCEDIR;
		
	}
	_pd_validate_source_dir();
	_pd_load_source_colors();
}


void _pd_load_next_source_dir()
{
	_pd_current_source_dir++;
	if(_pd_current_source_dir >= MAXSOURCEDIRS)
		_pd_current_source_dir = 0;
	
	_pd_validate_source_dir();
	printfDebug(DebugInfo, "_pd_current_source_dir = %d\n", _pd_current_source_dir);
	_pd_save_source_dir();
	_pd_reset();
}

const char * _pd_api_get_current_source_dir()
{
	return _pd_alternate_source_dirs[_pd_current_source_dir];
}

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

void _pd_api_display()
{
	Uint8 r,g,b,a;
	SDL_Texture* target;
	LCDColor bgColor;
	
	//clear fullbackground of the real screen (in case display offset is used)
	target = SDL_GetRenderTarget(Renderer);
					
	SDL_SetRenderTarget(Renderer, NULL);
	bgColor = getBackgroundDrawColor();
	SDL_GetRenderDrawColor(Renderer, &r, &g, & b, &a);
	
	if(bgColor == kColorClear)
	{
		if (_pd_api_display_Inverted)
			bgColor = kColorBlack;
		else
			bgColor = kColorWhite;
	}
	else
	{
		if (_pd_api_display_Inverted)
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
	LCDBitmap * screen = Api->graphics->newBitmap(LCD_COLUMNS, LCD_ROWS, _pd_api_display_Inverted? kColorBlack:kColorWhite);//(LCDSolidColor)bgColor == kColorClear ? _DisplayInverted? kColorBlack:kColorWhite: (LCDSolidColor)bgColor);
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
	if (_pd_api_display_Inverted)
	{
		Api->graphics->setDrawMode(kDrawModeInverted);
	}
	//set flip mode
	LCDBitmapFlip flip = kBitmapUnflipped;
	if (_pd_api_display_FlippedX && _pd_api_display_FlippedY)
	{
		flip = kBitmapFlippedXY;
	}
	else
	{
		if (_pd_api_display_FlippedX)
		{
			flip = kBitmapFlippedX;
		}
		else
		{
			if(_pd_api_display_FlippedY)
			{
				flip = kBitmapFlippedY;
			}
		}
	}
	//draw using the newly added all around function
	_pd_api_gfx_drawBitmapAll(playdatescreen, 0, 0, _pd_api_display_Scale, _pd_api_display_Scale, false, 0, 0, 0, flip);
	Api->graphics->popContext();

	//now we just need to blit this bitmap to the screen, but i could not access screen->tex so added another function
	//to get the sdl texture                  
	SDL_SetRenderTarget(Renderer, NULL);
	SDL_Surface* Tex = _pd_api_gfx_GetSDLTextureFromBitmap(screen);
	SDL_UpdateTexture(_pd_api_TexScreen, NULL, Tex->pixels, Tex->pitch);
	SDL_Rect Dest;	
	//crop / cut out
	if (SCALINGMODE == 0)
	{
		Dest.x = _pd_api_display_OffsetDisplayX;
		Dest.y = _pd_api_display_OffsetDisplayY;
		Dest.w = LCD_COLUMNS;
		Dest.h = LCD_ROWS;
		//res hack
		Dest.x -= (LCD_COLUMNS - SCREENRESX) / 2;
		Dest.y -= (LCD_ROWS - SCREENRESY) / 2;
		SDL_RenderSetLogicalSize(Renderer, SCREENRESX, SCREENRESY);
	}
	else
	{
		//Stretch
		if(SCALINGMODE == 1)
		{
			int w, h;
			SDL_GetWindowSize(SdlWindow, &w, &h);
			Dest.x = _pd_api_display_OffsetDisplayX;
			Dest.y = _pd_api_display_OffsetDisplayY;
			Dest.w = w;
			Dest.h = h;
			//res hack
			Dest.x = _pd_api_display_OffsetDisplayX*Dest.w/LCD_COLUMNS;
			Dest.y = _pd_api_display_OffsetDisplayY*Dest.h/LCD_ROWS;
			SDL_RenderSetLogicalSize(Renderer, Dest.w, Dest.h);
		}
		//default letterboxed
		else
		{
			Dest.x = _pd_api_display_OffsetDisplayX;
			Dest.y = _pd_api_display_OffsetDisplayY;
			Dest.w = LCD_COLUMNS;
			Dest.h = LCD_ROWS;
			//res hack
			SDL_RenderSetLogicalSize(Renderer, LCD_COLUMNS, LCD_ROWS);
		}
	}
	//end res hack
	SDL_RenderCopy(Renderer, _pd_api_TexScreen, NULL, &Dest);
	SDL_RenderPresent(Renderer);
   	//reset res hack
	SDL_RenderSetLogicalSize(Renderer, LCD_COLUMNS, LCD_ROWS);
	Api->graphics->freeBitmap(screen);

	//restore render target
	SDL_SetRenderTarget(Renderer, target);
}

void runMainLoop()
{
	Uint64 StartTicks = SDL_GetPerformanceCounter();

	//update input handler
	_pd_api_sys_UpdateInput();

	//run the update function
	_pd_api_sys_DoUpdate(_pd_api_sys_DoUpdateuserdata);

	//clean sprites set as not loaded (delayed delete to prevent issues)
	_pd_api_sprite_cleanup_sprites(true);
	
	//flip / display screen
	_pd_api_display();

	//cliprects are cleared after a frame
	Api->graphics->clearClipRect();

	//for calculating avergage
	pd_api_FrameCount++;

	//calculate fps & delay
	pd_api_elapsed = (((double)SDL_GetPerformanceCounter() - (double)StartTicks) / (double)SDL_GetPerformanceFrequency()) * 1000.0;
	if ((_pd_api_display_DesiredDelta != 0.0) && (pd_api_elapsed < _pd_api_display_DesiredDelta))
	{
		//SDL_Delay((Uint32)(_DisplayDesiredDelta - elapsed));
		preciseSleep((_pd_api_display_DesiredDelta - pd_api_elapsed) / 1000.0);
	}

	pd_api_elapsed = ((double)SDL_GetPerformanceCounter() - (double)StartTicks) / (double)SDL_GetPerformanceFrequency();
	if(pd_api_elapsed != 0.0)
	{
		_pd_api_display_CurrentFps = 1.0 / pd_api_elapsed;
		pd_api_TotalFps += _pd_api_display_CurrentFps;
		_pd_api_display_AvgFps = pd_api_TotalFps / (double)pd_api_FrameCount;
	}
	
	if (((SDL_GetPerformanceFrequency() + pd_api_fpslogticks)) < SDL_GetPerformanceCounter())
	{  
		pd_api_TotalFps = 0.0;
		pd_api_FrameCount = 0;
		_pd_api_display_LastFPS = _pd_api_display_AvgFps;
		printfDebug(DebugFPS, "%2f\n", _pd_api_display_LastFPS);
		pd_api_fpslogticks = SDL_GetPerformanceCounter();
	}

	//needs to happen on the end or keventterminate or could produce problems
	//when stuff gets freed and update function (_pd_api_sys_DoUpdate) is called again with freed assets
	//so a flag is used and set with the callback but only acted upon here
	if (_pd_do_load_next_source_dir)
	{
		_pd_do_load_next_source_dir = false;
		_pd_load_next_source_dir();
	}
}

bool fileExists(const char *fname)
{
    FILE *file;
	file = fopen(fname, "r");
	if (file)
    {
        fclose(file);
        return true;
    }
    return false;
}

int main(int argv, char** args)
{
	float windowScale = WINDOWSCALE;
	bool useSoftwareRenderer = true;
    bool useLinear = false;
    bool useVsync = false;
    bool useFullScreenAtStartup = FULLSCREENATSTARTUP;
    bool useOpenGLHint = false;
	bool checkMuxFile = false;
	int c;
    while ((c = getopt(argv, args, "?lafvodtqm")) != -1) 
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
  -v: Use VSync\n\
  -d: Use Double Window Size\n\
  -t: Use Triple Window Size\n\
  -q: Use Quadruple Window Size\n");
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
			case 'd':
				windowScale = 2.0f;
				break;
			case 't':
				windowScale = 3.0f;
				break;
			case 'q':
				windowScale = 4.0f;
				break;
			case 'm':
				checkMuxFile = true;
				break;
			default:
				break;
        }
    }

#ifdef FORCE_ACCELERATED_RENDER
	useSoftwareRenderer = false;
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) == 0) 
    {
		const char * videoDriver = SDL_GetCurrentVideoDriver();
		SDL_Log("Using Video Driver:%s\n", videoDriver);
        Uint32 WindowFlags = SDL_WINDOW_RESIZABLE;
        if (useFullScreenAtStartup) {
            WindowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;            
        }

       	SdlWindow = SDL_CreateWindow("Playdate Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREENRESX*windowScale, SCREENRESY*windowScale, WindowFlags);
        
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
           
            SDL_SetCursor(SDL_DISABLE);
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
				#ifdef __EMSCRIPTEN__
    				Uint16 audio_format = MIX_DEFAULT_FORMAT;
					int audio_channels = MIX_DEFAULT_CHANNELS;
					int audio_buffers = 4096;
					int const audio_rate = EM_ASM_INT_V({
        				var context;
        				try {
            				context = new AudioContext();
        				} catch (e) {
            				context = new webkitAudioContext();
        				}
        				return context.sampleRate;
    				});
					if(Mix_OpenAudioDevice(audio_rate, audio_format, audio_channels, audio_buffers, NULL, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE) == 0)
				#else
    				int const audio_rate = 44100;
					Uint16 audio_format = AUDIO_S16SYS; 
					int audio_channels = 2;
					int audio_buffers = 256;
                    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) == 0)
				#endif
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

						//load potential saved source dir value
						_pd_load_source_dir();

                        //initialize handler
                        eventHandler(Api, kEventInit, 0);

                        pd_api_fpslogticks = SDL_GetPerformanceCounter();
                        stubDoQuit = false;
                        pd_api_elapsed = 0.0;
                        pd_api_FrameCount = 0;
                        pd_api_TotalFps = 0.0;
						_pd_api_TexScreen = SDL_CreateTexture(Renderer, pd_api_gfx_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, LCD_COLUMNS, LCD_ROWS);

						#ifdef __EMSCRIPTEN__
  							emscripten_set_main_loop([]() { runMainLoop(); }, 0, true);
						#else
     			    	while(!stubDoQuit)
						{
							runMainLoop();
							if(checkMuxFile)
								if(fileExists("/tmp/mux_done"))
									stubDoQuit = true;	
						}
						#endif
                        eventHandler(Api, kEventTerminate, 0);

						//erase remaining sprites in memory
                        _pd_api_sprite_cleanup_sprites(false);

                        //free samples & sounds
                        _pd_api_sound_freeSampleList();

						_pd_api_gfx_cleanUp();

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
						SDL_DestroyTexture(_pd_api_TexScreen);

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
