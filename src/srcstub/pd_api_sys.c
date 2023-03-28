#include <SDL.h>
#include "pd_api/pd_api_sys.h"
#include "CInput.h"
#include "gamestubcallbacks.h"

CInput *pd_api_sys_input = NULL;
PDCallbackFunction* DoUpdate;
void* DoUpdateuserdata;

int _CranckSoundDisabled = 0;
int _CranckDocked = 0;
Uint32 pd_api_sys_startElapsed = 0;

void _pd_api_sys_UpdateInput()
{
	if(pd_api_sys_input == NULL)
	{
		pd_api_sys_input = CInput_Create();
	}
	
	CInput_Update(pd_api_sys_input);

	if ((pd_api_sys_input->Buttons.ButFullscreen) && (!pd_api_sys_input->PrevButtons.ButFullscreen))
		fullScreenCallBack();

	if (pd_api_sys_input->Buttons.RenderReset)
		renderResetCallBack();

	if (pd_api_sys_input->Buttons.ButQuit)
		quitCallBack();
}

void pd_api_sys_setUpdateCallback(PDCallbackFunction* update, void* userdata)
{
	DoUpdate = update;
	DoUpdateuserdata = userdata;
}

void pd_api_sys_getButtonState(PDButtons* current, PDButtons* pushed, PDButtons* released)
{
	if(current != NULL)
	{
		*current = 0;
		if (pd_api_sys_input->Buttons.ButLeft)
			*current |= kButtonLeft;

		if (pd_api_sys_input->Buttons.ButRight)
			*current |= kButtonRight;

		if (pd_api_sys_input->Buttons.ButUp)
			*current |= kButtonUp;

		if (pd_api_sys_input->Buttons.ButDown)
			*current |= kButtonDown;

		if (pd_api_sys_input->Buttons.ButA)
			*current |= kButtonA;

		if (pd_api_sys_input->Buttons.ButB)
			*current |= kButtonB;		
	}
	
	if(pushed != NULL)
	{
		*pushed = 0;
		if ((pd_api_sys_input->Buttons.ButLeft) && (!pd_api_sys_input->PrevButtons.ButLeft))
			*pushed |= kButtonLeft;

		if ((pd_api_sys_input->Buttons.ButRight) && (!pd_api_sys_input->PrevButtons.ButRight))
			*pushed |= kButtonRight;

		if ((pd_api_sys_input->Buttons.ButUp) && (!pd_api_sys_input->PrevButtons.ButUp))
			*pushed |= kButtonUp;

		if ((pd_api_sys_input->Buttons.ButDown) && (!pd_api_sys_input->PrevButtons.ButDown))
			*pushed |= kButtonDown;

		if ((pd_api_sys_input->Buttons.ButA) && (!pd_api_sys_input->PrevButtons.ButA))
			*pushed |= kButtonA;

		if ((pd_api_sys_input->Buttons.ButB) && (!pd_api_sys_input->PrevButtons.ButB))
			*pushed |= kButtonB;
	}
	
	if (released != NULL)
	{
		*released = 0;
		if ((!pd_api_sys_input->Buttons.ButLeft) && (pd_api_sys_input->PrevButtons.ButLeft))
			*released |= kButtonLeft;

		if ((!pd_api_sys_input->Buttons.ButRight) && (pd_api_sys_input->PrevButtons.ButRight))
			*released |= kButtonRight;

		if ((!pd_api_sys_input->Buttons.ButUp) && (pd_api_sys_input->PrevButtons.ButUp))
			*released |= kButtonUp;
		
		if ((!pd_api_sys_input->Buttons.ButDown) && (pd_api_sys_input->PrevButtons.ButDown))
			*released |= kButtonDown;

		if ((!pd_api_sys_input->Buttons.ButA) && (pd_api_sys_input->PrevButtons.ButA))
			*released |= kButtonA;

		if ((!pd_api_sys_input->Buttons.ButB) && (pd_api_sys_input->PrevButtons.ButB))
			*released |= kButtonB;
	}
}

void pd_api_sys_drawFPS(int x, int y)
{

}

//cranck

float pd_api_sys_getCrankChange(void)
{
	return 0.0f;
}

float pd_api_sys_getCrankAngle(void)
{
	return 0.0f;
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
	char* fmtnewline = malloc(strlen(fmt) + 2);
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
	char* fmtnewline = malloc(strlen(fmt) + 2);
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
	char Buf[10000];
	va_list argptr;
    va_start(argptr, fmt);
    vsprintf(&Buf[0], fmt, argptr);
    va_end(argptr);
	*ret = pd_api_sys_realloc(NULL,(strlen(Buf) + 1) * sizeof(char));
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


	return tmp;
}

