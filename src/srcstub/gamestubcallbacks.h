#ifndef gamestubcallbacks_h
#define gamestubcallbacks_h
#include <SDL.h>
#include "pd_api.h"

typedef struct playdate_sys playdate_sys;
typedef struct playdate_display playdate_display;
typedef struct playdate_graphics playdate_graphics;
typedef struct playdate_video playdate_video;
typedef struct playdate_sound playdate_sound;
typedef struct playdate_sound_fileplayer playdate_sound_fileplayer;
typedef struct playdate_sound_sampleplayer playdate_sound_sampleplayer;
typedef struct playdate_sound_sample playdate_sound_sample;
typedef struct playdate_file playdate_file;
typedef struct LCDBitmap LCDBitmap;

extern int pd_api_sound_MaxSimSounds;
extern void fullScreenCallBack();
extern void renderResetCallBack();
extern void quitCallBack();

extern void _pd_api_gfx_drawBitmapAll(LCDBitmap* bitmap, int x, int y, float xscale, float yscale, const double angle, int centerx, int centery, LCDBitmapFlip flip);
extern SDL_Texture* _pd_api_gfx_GetSDLTextureFromBitmap(LCDBitmap* bitmap);
extern void _pd_api_sys_UpdateInput();
extern PDCallbackFunction* DoUpdate;

extern void* DoUpdateuserdata;
extern float _DisplayFps;
extern double _DisplayDesiredDelta;
extern unsigned int _DisplayScale;
extern unsigned int _DisplayMosaicX;
extern unsigned int _DisplayMosaicY;
extern int _DisplayFlippedX;
extern int _DisplayFlippedY;
extern int _DisplayOffsetDisplayX;
extern int _DisplayOffsetDisplayY;
extern int _DisplayInverted;
extern double _CurrentFps;

//sys
extern int _CranckSoundDisabled;
extern int _CranckDocked;


extern playdate_sys* pd_api_sys_Create_playdate_sys();
extern playdate_display* pd_api_display_Create_playdate_display();
extern playdate_graphics* pd_api_gfx_Create_playdate_graphics();
extern playdate_video* pd_api_gfx_Create_playdate_video();
extern playdate_sound* pd_api_sound_Create_playdate_sound();
extern playdate_sound_fileplayer* pd_api_sound_Create_playdate_sound_fileplayer();
extern playdate_file* pd_api_file_Create_playdate_file();

#endif