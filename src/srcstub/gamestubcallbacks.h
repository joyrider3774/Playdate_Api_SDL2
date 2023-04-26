#ifndef gamestubcallbacks_h
#define gamestubcallbacks_h
#include <SDL.h>
#include "pd_api.h"

typedef struct LCDBitmap LCDBitmap;
typedef struct CWItemInfo CWItemInfo;
typedef struct CWCollisionInfo CWCollisionInfo;
typedef struct LCDSprite LCDSprite;
typedef struct PDSynthSignalValue PDSynthSignalValue;
typedef struct PDSynthSignal PDSynthSignal;
typedef struct SequenceTrack SequenceTrack;
typedef struct SoundSequence SoundSequence;
typedef struct SoundSource SoundSource;
typedef struct playdate_sys playdate_sys;
typedef struct playdate_display playdate_display;
typedef struct playdate_graphics playdate_graphics;
typedef struct playdate_video playdate_video;
typedef struct playdate_sound playdate_sound;
typedef struct playdate_sound_fileplayer playdate_sound_fileplayer;
typedef struct playdate_sound_sampleplayer playdate_sound_sampleplayer;
typedef struct playdate_sound_sample playdate_sound_sample;
typedef struct playdate_file playdate_file;
typedef struct playdate_sprite playdate_sprite;
typedef struct playdate_lua playdate_lua;
typedef struct playdate_scoreboards playdate_scoreboards;
typedef struct playdate_json playdate_json;
typedef struct playdate_sound_source playdate_sound_source;
typedef struct playdate_sound_signal playdate_sound_signal;
typedef struct playdate_sound_lfo playdate_sound_lfo;
typedef struct playdate_sound_envelope playdate_sound_envelope;
typedef struct playdate_sound_synth playdate_sound_synth;

extern void _pd_api_sys_fullScreenCallBack();
extern void _pd_api_sys_renderResetCallBack();
extern void _pd_api_sys_quitCallBack();

//sys
extern PDCallbackFunction* _pd_api_sys_DoUpdate;
extern void* _pd_api_sys_DoUpdateuserdata;
extern void _pd_api_sys_UpdateInput();
extern void _pd_api_sound_freeSampleList();
extern void _pd_api_gfx_freeFontList();
extern void _pd_api_gfx_drawBitmapAll(LCDBitmap* bitmap, int x, int y, float xscale, float yscale, const double angle, int centerx, int centery, LCDBitmapFlip flip);
extern SDL_Texture* _pd_api_gfx_GetSDLTextureFromBitmap(LCDBitmap* bitmap);


extern void _pd_api_sprite_cleanup_sprites(bool OnlyNotLoadedSprites);

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
extern double _AvgFps;
extern double _LastFPS;

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
extern playdate_sprite* pd_api_sprite_Create_playdate_sprite();
extern playdate_lua* pd_api_lua_Create_playdate_lua();
extern playdate_scoreboards* pd_api_scoreboards_Create_playdate_scoreboards();
extern playdate_json* pd_api_json_Create_playdate_json();
extern playdate_sound_source* pd_api_sound_Create_playdate_sound_source();

#endif