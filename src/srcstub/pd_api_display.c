#include "pd_api/pd_api_display.h"
#include "gamestubcallbacks.h"

float _DisplayFps = 50.0f;
double _DisplayDesiredDelta = 1000.0 / 50.0;
unsigned int _DisplayScale = 1;
unsigned int _DisplayMosaicX = 0;
unsigned int _DisplayMosaicY = 0;
int _DisplayFlippedX = 0;
int _DisplayFlippedY = 0;
int _DisplayOffsetDisplayX = 0;
int _DisplayOffsetDisplayY = 0;
int _DisplayInverted = 0;
double _CurrentFps = 0.0f;

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
    if(tmp > 50)
        tmp = 50;
    _DisplayFps = tmp;
    if (tmp > 0)
        _DisplayDesiredDelta = 1000.0 / (double)_DisplayFps;
    else
        _DisplayDesiredDelta = 0.0;
}

void pd_api_display_setInverted(int flag)
{
    _DisplayInverted = flag;
}

void pd_api_display_setScale(unsigned int s)
{
    _DisplayScale = s;
}

void pd_api_display_setMosaic(unsigned int x, unsigned int y)
{
    _DisplayMosaicX = x;
    _DisplayMosaicY = y;
}

void pd_api_display_setFlipped(int x, int y)
{
    _DisplayFlippedX = x;
    _DisplayFlippedY = y;
}

void pd_api_display_setDisplayOffset(int x, int y)
{
    _DisplayOffsetDisplayX = x;
    _DisplayOffsetDisplayY = y;
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
    return Tmp;
}