#include "pd_api/pd_api_display.h"
#include "gamestubcallbacks.h"

float _pd_api_display_Fps = 30.0f;
double _pd_api_display_DesiredDelta = 1000.0 / 30.0;
unsigned int _pd_api_display_Scale = 1;
unsigned int _pd_api_display_MosaicX = 0;
unsigned int _pd_api_display_MosaicY = 0;
int _pd_api_display_FlippedX = 0;
int _pd_api_display_FlippedY = 0;
int _pd_api_display_OffsetDisplayX = 0;
int _pd_api_display_OffsetDisplayY = 0;
int _pd_api_display_Inverted = 0;
double _pd_api_display_CurrentFps = 0.0;
double _pd_api_display_AvgFps = 0.0;
double _pd_api_display_LastFPS = 0.0;

int pd_api_display_getWidth(void)
{
    return LCD_COLUMNS;
}

int pd_api_display_getHeight(void)
{
    return LCD_ROWS;
}
	
void pd_api_display_setRefreshRate(float rate)
{
    float tmp = rate;
    if(tmp > 50.0f)
        tmp = 50.0f;
    _pd_api_display_Fps = tmp;
    if (tmp > 0)
        _pd_api_display_DesiredDelta = 1000.0 / (double)_pd_api_display_Fps;
    else
        _pd_api_display_DesiredDelta = 0.0;
}

void pd_api_display_setInverted(int flag)
{
    _pd_api_display_Inverted = flag;
}

void pd_api_display_setScale(unsigned int s)
{
    _pd_api_display_Scale = s;
}

void pd_api_display_setMosaic(unsigned int x, unsigned int y)
{
    _pd_api_display_MosaicX = x;
    _pd_api_display_MosaicY = y;
}

void pd_api_display_setFlipped(int x, int y)
{
    _pd_api_display_FlippedX = x;
    _pd_api_display_FlippedY = y;
}

void pd_api_display_setDisplayOffset(int x, int y)
{
    _pd_api_display_OffsetDisplayX = x;
    _pd_api_display_OffsetDisplayY = y;
}

// 2.7

float pd_api_display_getRefreshRate(void)
{
    return _pd_api_display_Fps;
}

float pd_api_display_getFPS(void)
{
    return (float)_pd_api_display_LastFPS;
}

playdate_display* pd_api_display_Create_playdate_display()
{
    playdate_display *Tmp = (playdate_display*) malloc(sizeof(*Tmp));
    
    Tmp->getHeight = pd_api_display_getHeight;
    Tmp->getWidth = pd_api_display_getWidth;
    Tmp->setRefreshRate = pd_api_display_setRefreshRate;
    Tmp->setInverted = pd_api_display_setInverted;
    Tmp->setScale = pd_api_display_setScale;
    Tmp->setMosaic = pd_api_display_setMosaic;
    Tmp->setFlipped = pd_api_display_setFlipped;
    Tmp->setOffset = pd_api_display_setDisplayOffset;
    Tmp->getRefreshRate = pd_api_display_getRefreshRate;
    Tmp->getFPS = pd_api_display_getFPS;
    return Tmp;
}