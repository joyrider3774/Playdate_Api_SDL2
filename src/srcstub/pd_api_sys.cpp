#include <SDL.h>
#include <time.h>
#include <sys/time.h>
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
unsigned int _startTime = 0;

uint32_t pd_api_sys_convertDateTimeToEpoch(struct PDDateTime* datetime);

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
		return realloc(ptr, size);
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
	return _startTime + SDL_GetTicks();
}

unsigned int pd_api_sys_getSecondsSinceEpoch(unsigned int *milliseconds)
{
    // Get current time
    time_t now = time(NULL);
    struct tm *tm_now = gmtime(&now);
    
    // Create PDDateTime from current time
    struct PDDateTime datetime;
    datetime.year = tm_now->tm_year + 1900;
    datetime.month = tm_now->tm_mon + 1;
    datetime.day = tm_now->tm_mday;
    datetime.hour = tm_now->tm_hour;
    datetime.minute = tm_now->tm_min;
    datetime.second = tm_now->tm_sec;
    
    // Get milliseconds if requested
    if (milliseconds != NULL) 
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		*milliseconds = tv.tv_usec / 1000;
    }
    
    // Convert to epoch (seconds since Jan 1, 2000)
    return pd_api_sys_convertDateTimeToEpoch(&datetime);
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
	return (SDL_GetTicks() - pd_api_sys_startElapsed) / 1000.0f;
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
    // Days since epoch for each month (non-leap year)
    static const uint16_t days_before_month[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    
    // Convert epoch seconds to days and remaining seconds
    uint32_t days = epoch / 86400;
    uint32_t seconds_today = epoch % 86400;
    
    // Calculate time components
    datetime->hour = seconds_today / 3600;
    datetime->minute = (seconds_today % 3600) / 60;
    datetime->second = seconds_today % 60;
    
    // Calculate weekday (Jan 1, 2000 was a Saturday = day 6)
    datetime->weekday = ((days + 6) % 7);
    if (datetime->weekday == 0) datetime->weekday = 7; // Sunday = 7
    
    // Calculate year (approximation then refinement)
    uint32_t year = 2000 + (days / 365);
    
    // Count leap years from 2000 to current year
    uint32_t leap_years = ((year - 1) / 4) - (1999 / 4) - ((year - 1) / 100) + (1999 / 100) + ((year - 1) / 400) - (1999 / 400);
    uint32_t days_since_2000 = (year - 2000) * 365 + leap_years;
    
    // Adjust if we overshot
    while (days_since_2000 > days) {
        year--;
        leap_years = ((year - 1) / 4) - (1999 / 4) - ((year - 1) / 100) + (1999 / 100) + ((year - 1) / 400) - (1999 / 400);
        days_since_2000 = (year - 2000) * 365 + leap_years;
    }
    
    datetime->year = year;
    
    // Calculate day of year
    uint32_t day_of_year = days - days_since_2000;
    
    // Check if leap year
    int is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    
    // Find month
    datetime->month = 1;
    for (int m = 11; m >= 0; m--) {
        uint32_t days_before = days_before_month[m];
        if (m > 1 && is_leap) days_before++; // Add leap day for March onwards
        
        if (day_of_year >= days_before) {
            datetime->month = m + 1;
            datetime->day = day_of_year - days_before + 1;
            break;
        }
    }
}

uint32_t pd_api_sys_convertDateTimeToEpoch(struct PDDateTime* datetime)
{
    // Days before each month (non-leap year)
    static const uint16_t days_before_month[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    
    // Calculate days from 2000 to start of current year
    uint32_t year = datetime->year;
    uint32_t days = (year - 2000) * 365;
    
    // Add leap days
    days += ((year - 1) / 4) - (1999 / 4);      // Years divisible by 4
    days -= ((year - 1) / 100) - (1999 / 100);  // Except years divisible by 100
    days += ((year - 1) / 400) - (1999 / 400);  // Unless years divisible by 400
    
    // Add days for months in current year
    days += days_before_month[datetime->month - 1];
    
    // Add leap day if after February in a leap year
    if (datetime->month > 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
        days++;
    }
    
    // Add days in current month
    days += datetime->day - 1;
    
    // Convert to seconds and add time components
    uint32_t epoch = days * 86400;
    epoch += datetime->hour * 3600;
    epoch += datetime->minute * 60;
    epoch += datetime->second;
    
    return epoch;
}

// 2.0
void pd_api_sys_clearICache(void)
{

}

// 2.4
void pd_api_sys_setButtonCallback(PDButtonCallbackFunction* cb, void* buttonud, int queuesize)
{

}

void pd_api_sys_setSerialMessageCallback(void (*callback)(const char* data))
{

}

int pd_api_sys_vaFormatString(char **outstr, const char *fmt, va_list args)
{
	char Buf[FORMAT_STRING_BUFLENGTH];
    int result = vsprintf(&Buf[0], fmt, args);
	if(result > 0)
	{
		*outstr = (char*) pd_api_sys_realloc(NULL,(result + 1) * sizeof(char));
		strcpy(*outstr, Buf);
	}
	return result;
}

int pd_api_sys_parseString(const char *str, const char *format, ...)
{
	return 0;
}

void pd_api_sys_delay(uint32_t milliseconds)
{

}

// 2.7
void pd_api_sys_getServerTime(void (*callback)(const char* time, const char* err))
{

}

void pd_api_sys_restartGame(const char* launchargs)
{

}

const char* pd_api_sys_getLaunchArgs(const char** outpath)
{
	return NULL;
}

bool pd_api_sys_sendMirrorData(uint8_t command, void* data, int len)
{
	return false;
}

// 3.0
const struct PDInfo* pd_api_sys_getSystemInfo(void)
{
	PDInfo* Tmp = (PDInfo*)malloc(sizeof(PDInfo));
	Tmp->language = kPDLanguageEnglish;
	Tmp->osversion = 0;
	return Tmp;
}

playdate_sys* pd_api_sys_Create_playdate_sys()
{
	unsigned int milli;
	_startTime = pd_api_sys_getSecondsSinceEpoch(&milli);
	_startTime = _startTime * 1000 + milli;
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

	// 2.4
	tmp->setButtonCallback = pd_api_sys_setButtonCallback;
	tmp->setSerialMessageCallback = pd_api_sys_setSerialMessageCallback;
	tmp->vaFormatString = pd_api_sys_vaFormatString;
	tmp->parseString = pd_api_sys_parseString;
	
	// ???
	tmp->delay = pd_api_sys_delay;

	// 2.7
	tmp->getServerTime = pd_api_sys_getServerTime;
	tmp->restartGame = pd_api_sys_restartGame;
	tmp->getLaunchArgs = pd_api_sys_getLaunchArgs;
	tmp->sendMirrorData = pd_api_sys_sendMirrorData;
	
	// 3.0
	tmp->getSystemInfo = pd_api_sys_getSystemInfo;

	return tmp;
}

