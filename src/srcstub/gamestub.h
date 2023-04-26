#ifndef gamestub_h
#define gamestub_h

#include <SDL.h>
#include <stdbool.h>
#include "pd_api.h"


extern SDL_Window *SdlWindow;
extern SDL_Renderer *Renderer;
extern PlaydateAPI *Api;
extern LCDBitmap* _Playdate_Screen;
extern bool stubDoQuit;
extern SDL_Texture* getPlaydateScreenTexture();
extern LCDColor getBackgroundDrawColor();
extern SDL_PixelFormatEnum pd_api_gfx_PIXELFORMAT;
extern SDL_Color pd_api_gfx_color_clear; 
extern SDL_Color pd_api_gfx_color_white; 
extern SDL_Color pd_api_gfx_color_black; 
extern SDL_Color pd_api_gfx_color_whitetreshold;
extern SDL_Color pd_api_gfx_color_blacktreshold;
int main(int argv, char** args);

#endif