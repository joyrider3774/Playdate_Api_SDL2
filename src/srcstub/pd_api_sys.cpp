#include <SDL.h>
#include "pd_api/pd_api_sys.h"
#include "CInput.h"
#include "gamestubcallbacks.h"
#include "defines.h"
#include "gamestub.h"

CInput *_pd_api_sys_input = NULL;
PDCallbackFunction* _pd_api_sys_DoUpdate;
void* _pd_api_sys_DoUpdateuserdata;

int _CranckSoundDisabled = 0;
int _CranckDocked = 0;
Uint32 pd_api_sys_startElapsed = 0;
float _CranckChange = 0.0f;
float _CranckAngle = 0.0f;

void _pd_api_sys_UpdateInput()
{
	if(_pd_api_sys_input == NULL)
	{
		_pd_api_sys_input = CInput_Create();
	}
	
	CInput_Update(_pd_api_sys_input);

	_CranckChange = 0.0f;

	if (_pd_api_sys_input->Buttons.ButLB)
	{
		_CranckDocked = false;
		_CranckChange = -5.0f * (30.0f / (((float) _pd_api_display_AvgFps) == 0.0f ? 30.0f : (float) _pd_api_display_AvgFps));
		_CranckAngle = _CranckAngle + _CranckChange;
		if (_CranckAngle < 0.0f)
			_CranckAngle = 360.0f + _CranckAngle;
	}

	if (_pd_api_sys_input->Buttons.ButRB)
	{
		_CranckDocked = false;
		_CranckChange = 5.0f * (30.0f / (((float) _pd_api_display_AvgFps) == 0.0f ? 30.0f : (float) _pd_api_display_AvgFps));
		_CranckAngle = _CranckAngle + _CranckChange;
		if (_CranckAngle > 359.0f)
			_CranckAngle =  _CranckAngle - 360.0f;
	}

	if (_pd_api_sys_input->Buttons.ButLT)
		_CranckDocked = true;

	if (_pd_api_sys_input->Buttons.ButRT)
		_CranckDocked = false;

	if ((_pd_api_sys_input->Buttons.ButFullscreen) && (!_pd_api_sys_input->PrevButtons.ButFullscreen))
		_pd_api_sys_fullScreenCallBack();

	if (_pd_api_sys_input->Buttons.RenderReset)
		_pd_api_sys_renderResetCallBack();

	if (_pd_api_sys_input->Buttons.ButQuit)
		_pd_api_sys_quitCallBack();
	
	if ((_pd_api_sys_input->Buttons.ButX || _pd_api_sys_input->Buttons.NextSource) && 
		!(_pd_api_sys_input->PrevButtons.ButX || _pd_api_sys_input->PrevButtons.NextSource))
		_pd_api_sys_nextSourceDirCallback();
}

void pd_api_sys_setUpdateCallback(PDCallbackFunction* update, void* userdata)
{
	_pd_api_sys_DoUpdate = update;
	_pd_api_sys_DoUpdateuserdata = userdata;
}

void pd_api_sys_getButtonState(PDButtons* current, PDButtons* pushed, PDButtons* released)
{
	if(current != NULL)
	{
		int *curr = (int*)current;
		*curr = 0;
		if (_pd_api_sys_input->Buttons.ButLeft || _pd_api_sys_input->Buttons.ButDpadLeft)
			*curr |= kButtonLeft;

		if (_pd_api_sys_input->Buttons.ButRight || _pd_api_sys_input->Buttons.ButDpadRight)
			*curr |= kButtonRight;

		if (_pd_api_sys_input->Buttons.ButUp || _pd_api_sys_input->Buttons.ButDpadUp)
			*curr |= kButtonUp;

		if (_pd_api_sys_input->Buttons.ButDown || _pd_api_sys_input->Buttons.ButDpadDown)
			*curr |= kButtonDown;

		if (_pd_api_sys_input->Buttons.ButA)
			*curr |= kButtonA;

		if (_pd_api_sys_input->Buttons.ButB)
			*curr |= kButtonB;		
	}
	
	if(pushed != NULL)
	{
		
		int *push = (int*)pushed;
		*push = 0;
		if (((_pd_api_sys_input->Buttons.ButLeft) && (!_pd_api_sys_input->PrevButtons.ButLeft)) ||
			((_pd_api_sys_input->Buttons.ButDpadLeft) && (!_pd_api_sys_input->PrevButtons.ButDpadLeft)))
			*push |= kButtonLeft;

		if (((_pd_api_sys_input->Buttons.ButRight) && (!_pd_api_sys_input->PrevButtons.ButRight)) ||
			((_pd_api_sys_input->Buttons.ButDpadRight) && (!_pd_api_sys_input->PrevButtons.ButDpadRight)))
			*push |= kButtonRight;

		if (((_pd_api_sys_input->Buttons.ButUp) && (!_pd_api_sys_input->PrevButtons.ButUp)) ||
			((_pd_api_sys_input->Buttons.ButDpadUp) && (!_pd_api_sys_input->PrevButtons.ButDpadUp)))
			*push |= kButtonUp;

		if (((_pd_api_sys_input->Buttons.ButDown) && (!_pd_api_sys_input->PrevButtons.ButDown)) ||
			((_pd_api_sys_input->Buttons.ButDpadDown) && (!_pd_api_sys_input->PrevButtons.ButDpadDown)))
			*push |= kButtonDown;

		if ((_pd_api_sys_input->Buttons.ButA) && (!_pd_api_sys_input->PrevButtons.ButA))
			*push |= kButtonA;

		if ((_pd_api_sys_input->Buttons.ButB) && (!_pd_api_sys_input->PrevButtons.ButB))
			*push |= kButtonB;
	}
	
	if (released != NULL)
	{
		int *rel = (int *)released;
		*rel = 0;
		if (((!_pd_api_sys_input->Buttons.ButLeft) && (_pd_api_sys_input->PrevButtons.ButLeft)) ||
			((!_pd_api_sys_input->Buttons.ButDpadLeft) && (_pd_api_sys_input->PrevButtons.ButDpadLeft)))
			*rel |= kButtonLeft;

		if (((!_pd_api_sys_input->Buttons.ButRight) && (_pd_api_sys_input->PrevButtons.ButRight)) ||
			((!_pd_api_sys_input->Buttons.ButDpadRight) && (_pd_api_sys_input->PrevButtons.ButDpadRight)))
			*rel |= kButtonRight;

		if (((!_pd_api_sys_input->Buttons.ButUp) && (_pd_api_sys_input->PrevButtons.ButUp)) ||
			((!_pd_api_sys_input->Buttons.ButDpadUp) && (_pd_api_sys_input->PrevButtons.ButDpadUp)))
			*rel |= kButtonUp;
		
		if (((!_pd_api_sys_input->Buttons.ButDown) && (_pd_api_sys_input->PrevButtons.ButDown)) ||
			((!_pd_api_sys_input->Buttons.ButDpadDown) && (_pd_api_sys_input->PrevButtons.ButDpadDown)))
			*rel |= kButtonDown;

		if ((!_pd_api_sys_input->Buttons.ButA) && (_pd_api_sys_input->PrevButtons.ButA))
			*rel |= kButtonA;

		if ((!_pd_api_sys_input->Buttons.ButB) && (_pd_api_sys_input->PrevButtons.ButB))
			*rel |= kButtonB;
	}
}

void pd_api_sys_drawFPS(int x, int y)
{
	_pd_api_gfx_drawFPS(x,y);
}

//cranck

float pd_api_sys_getCrankChange(void)
{
	return _CranckChange;
}

float pd_api_sys_getCrankAngle(void)
{
	return _CranckAngle;
}

int pd_api_sys_isCrankDocked(void)
{
	return _CranckDocked;
}

int pd_api_sys_setCrankSoundsDisabled(int flag) // returns previous setting
{
	_CranckSoundDisabled = flag;
	return 1;
}


void pd_api_sys_logToConsole(const char* fmt, ...)
{
	char* fmtnewline = (char*)malloc(strlen(fmt) + 2);
	strcpy(fmtnewline, fmt);
	strcat(fmtnewline, "\n");

	va_list argptr;
    va_start(argptr, fmt);
    vfprintf(stdout, fmtnewline, argptr);
    va_end(argptr);
	free(fmtnewline);
}

void pd_api_sys_error(const char* fmt, ...)
{
	char* fmtnewline = (char*)malloc(strlen(fmt) + 2);
	strcpy(fmtnewline, fmt);
	strcat(fmtnewline, "\n");

	va_list argptr;
    va_start(argptr, fmt);
    vfprintf(stdout, fmtnewline, argptr);
    va_end(argptr);
	free(fmtnewline);
}

void* pd_api_sys_realloc(void* ptr, size_t size) // ptr = NULL -> malloc, size = 0 -> free
{
	if(size == 0)
	{
		free(ptr);
	}
	else
	{
		return malloc(size);
	}
	return NULL;
}

int pd_api_sys_formatString(char **ret, const char *fmt, ...)
{
	char Buf[FORMAT_STRING_BUFLENGTH];
	va_list argptr;
    va_start(argptr, fmt);
    vsprintf(&Buf[0], fmt, argptr);
    va_end(argptr);
	*ret = (char*) pd_api_sys_realloc(NULL,(strlen(Buf) + 1) * sizeof(char));
	strcpy(*ret, Buf);
	return 0;

}
PDLanguage pd_api_sys_getLanguage(void)
{
	return kPDLanguageEnglish;
}
unsigned int pd_api_sys_getCurrentTimeMilliseconds(void)
{
	return SDL_GetTicks();
}
unsigned int pd_api_sys_getSecondsSinceEpoch(unsigned int *milliseconds)
{
	return SDL_GetTicks() / 1000;
}

void pd_api_sys_setPeripheralsEnabled(PDPeripherals mask)
{

}

void pd_api_sys_getAccelerometer(float* outx, float* outy, float* outz)
{

}


int pd_api_sys_getFlipped(void)
{
	return 0;
}
void pd_api_sys_setAutoLockDisabled(int disable)
{

}

void pd_api_sys_setMenuImage(LCDBitmap* bitmap, int xOffset)
{

}

PDMenuItem* pd_api_sys_addMenuItem(const char *title, PDMenuItemCallbackFunction* callback, void* userdata)
{
	return NULL;
}

PDMenuItem* pd_api_sys_addCheckmarkMenuItem(const char *title, int value, PDMenuItemCallbackFunction* callback, void* userdata)
{
	return NULL;
}

PDMenuItem* pd_api_sys_addOptionsMenuItem(const char *title, const char** optionTitles, int optionsCount, PDMenuItemCallbackFunction* f, void* userdata)
{
	return NULL;
}

void pd_api_sys_removeAllMenuItems(void)
{

}

void pd_api_sys_removeMenuItem(PDMenuItem *menuItem)
{

}

int pd_api_sys_getMenuItemValue(PDMenuItem *menuItem)
{
	return 0;
}

void pd_api_sys_setMenuItemValue(PDMenuItem *menuItem, int value)
{

}

const char* pd_api_sys_getMenuItemTitle(PDMenuItem *menuItem)
{
	return NULL;
}

void pd_api_sys_setMenuItemTitle(PDMenuItem *menuItem, const char *title)
{

}

void* pd_api_sys_getMenuItemUserdata(PDMenuItem *menuItem)
{
	return NULL;
}

void pd_api_sys_setMenuItemUserdata(PDMenuItem *menuItem, void *ud)
{

}
	
int pd_api_sys_getReduceFlashing(void)
{
	return 0;
}
	
// 1.1
float pd_api_sys_getElapsedTime(void)
{
	return SDL_GetTicks() - pd_api_sys_startElapsed;
}
void pd_api_sys_resetElapsedTime(void)
{
	pd_api_sys_startElapsed = SDL_GetTicks();
}

// 1.4
float pd_api_sys_getBatteryPercentage(void)
{
	return 100.0f;
}

float pd_api_sys_getBatteryVoltage(void)
{
	return 5.0f;
}

// 1.13
int32_t pd_api_sys_getTimezoneOffset(void)
{
	return 0;
}

int pd_api_sys_shouldDisplay24HourTime(void)
{
	return 0;
}

void pd_api_sys_convertEpochToDateTime(uint32_t epoch, struct PDDateTime* datetime)
{

}

uint32_t pd_api_sys_convertDateTimeToEpoch(struct PDDateTime* datetime)
{
	return 0;
}

// 2.0
void pd_api_sys_clearICache(void)
{

}

playdate_sys* pd_api_sys_Create_playdate_sys()
{
	playdate_sys* tmp = (playdate_sys *) malloc(sizeof(*tmp));
	tmp->getButtonState = pd_api_sys_getButtonState;
	tmp->setUpdateCallback = pd_api_sys_setUpdateCallback;
	tmp->drawFPS = pd_api_sys_drawFPS;
	tmp->setCrankSoundsDisabled = pd_api_sys_setCrankSoundsDisabled;
	tmp->isCrankDocked = pd_api_sys_isCrankDocked;
	tmp->getCrankChange = pd_api_sys_getCrankChange;
	tmp->getCrankAngle = pd_api_sys_getCrankAngle;
	tmp->error = pd_api_sys_error;
	tmp->logToConsole = pd_api_sys_logToConsole;

	tmp->realloc = pd_api_sys_realloc;
	tmp->formatString = pd_api_sys_formatString;
	tmp->getLanguage = pd_api_sys_getLanguage;
	tmp->getCurrentTimeMilliseconds = pd_api_sys_getCurrentTimeMilliseconds;
	tmp->getSecondsSinceEpoch = pd_api_sys_getSecondsSinceEpoch;
	tmp->setPeripheralsEnabled = pd_api_sys_setPeripheralsEnabled;
	tmp->getAccelerometer = pd_api_sys_getAccelerometer;
	tmp->getFlipped = pd_api_sys_getFlipped;
	tmp->setAutoLockDisabled = pd_api_sys_setAutoLockDisabled;
	tmp->setMenuImage = pd_api_sys_setMenuImage;
	tmp->addMenuItem = pd_api_sys_addMenuItem;
	tmp->addCheckmarkMenuItem = pd_api_sys_addCheckmarkMenuItem;
	tmp->addOptionsMenuItem = pd_api_sys_addOptionsMenuItem;
	tmp->removeAllMenuItems = pd_api_sys_removeAllMenuItems;
	tmp->removeMenuItem = pd_api_sys_removeMenuItem;
	tmp->getMenuItemValue = pd_api_sys_getMenuItemValue;
	tmp->setMenuItemValue = pd_api_sys_setMenuItemValue;
	tmp->getMenuItemTitle = pd_api_sys_getMenuItemTitle;
	tmp->setMenuItemTitle = pd_api_sys_setMenuItemTitle;
	tmp->getMenuItemUserdata = pd_api_sys_getMenuItemUserdata;
	tmp->setMenuItemUserdata = pd_api_sys_setMenuItemUserdata;
	tmp->getReduceFlashing = pd_api_sys_getReduceFlashing;
	// 1.1
	tmp->getElapsedTime = pd_api_sys_getElapsedTime;
	tmp->resetElapsedTime = pd_api_sys_resetElapsedTime;
	// 1.4
	tmp->getBatteryPercentage = pd_api_sys_getBatteryPercentage;
	tmp->getBatteryVoltage = pd_api_sys_getBatteryVoltage;

	// 1.13
	tmp->getTimezoneOffset = pd_api_sys_getTimezoneOffset;
	tmp->shouldDisplay24HourTime = pd_api_sys_shouldDisplay24HourTime;
	tmp->convertEpochToDateTime = pd_api_sys_convertEpochToDateTime;
	tmp->convertDateTimeToEpoch = pd_api_sys_convertDateTimeToEpoch;

	// 2.0
	tmp->clearICache = pd_api_sys_clearICache;

	return tmp;
}

