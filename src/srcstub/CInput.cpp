#include <SDL.h>
#include <SDL_joystick.h>
#include "CInput.h"
#include "defines.h"

CInput *CInput_Create()
{
	CInput* tmp = (CInput*) malloc(sizeof(*tmp));
	
    tmp->JoystickDeadZone = 10000;
    tmp->TriggerDeadZone = 10000;
    tmp->GameController = NULL;
    CInput_ResetButtons(tmp);
    for (int i=0; i < SDL_NumJoysticks(); i++)
    {
        if(SDL_IsGameController(i))
        {
            tmp->GameController = SDL_GameControllerOpen(i);
            SDL_GameControllerEventState(SDL_ENABLE);
            SDL_Log("Joystick Detected!\n");
            break;
        }
    }
	return tmp;
}

void CInput_Destroy(CInput* cinput)
{
    if(cinput->GameController) 
    {
        SDL_GameControllerClose(cinput->GameController);
        cinput->GameController = NULL;
    }
	free(cinput);
	cinput = NULL;
}

void CInput_Update(CInput* cinput)
{
	SDL_Event Event;
    cinput->PrevButtons = cinput->Buttons;
    cinput->Buttons.ButQuit = false;
    cinput->Buttons.RenderReset = false;
	while (SDL_PollEvent(&Event))
	{
		if (Event.type == SDL_RENDER_TARGETS_RESET)
            cinput->Buttons.RenderReset = true;

		if (Event.type == SDL_QUIT)
            cinput->Buttons.ButQuit = true;
      
		if (Event.type == SDL_JOYDEVICEADDED)
            if(cinput->GameController == NULL)
				if(SDL_IsGameController(Event.jdevice.which))
					cinput->GameController = SDL_GameControllerOpen(Event.jdevice.which);

		if (Event.type == SDL_JOYDEVICEREMOVED)
		{
            SDL_Joystick* Joystick = SDL_GameControllerGetJoystick(cinput->GameController);
			if (Joystick)
				if (Event.jdevice.which == SDL_JoystickInstanceID(Joystick))
				{
                    SDL_GameControllerClose(cinput->GameController);
                    cinput->GameController = NULL;
				}
		}

		if (Event.type == SDL_CONTROLLERAXISMOTION)
            CInput_HandleJoystickAxisEvent(cinput, Event.jaxis.axis, Event.jaxis.value);

		if (Event.type == SDL_CONTROLLERBUTTONUP)
            CInput_HandleJoystickButtonEvent(cinput, Event.cbutton.button, false);

		if (Event.type == SDL_CONTROLLERBUTTONDOWN)
            CInput_HandleJoystickButtonEvent(cinput, Event.cbutton.button, true);

		if (Event.type == SDL_KEYUP)
            CInput_HandleKeyboardEvent(cinput, Event.key.keysym.sym, false);
        
		if (Event.type == SDL_KEYDOWN)
            CInput_HandleKeyboardEvent(cinput, Event.key.keysym.sym, true);
	}

}

void CInput_ResetButtons(CInput* cinput)
{
	cinput->Buttons.ButLeft = false;
	cinput->Buttons.ButRight = false;
	cinput->Buttons.ButUp = false;
	cinput->Buttons.ButDown = false;
	cinput->Buttons.ButLB = false;
	cinput->Buttons.ButRB = false;
	cinput->Buttons.ButLT = false;
	cinput->Buttons.ButRT = false;
	cinput->Buttons.ButBack = false;
	cinput->Buttons.ButA = false;
	cinput->Buttons.ButB = false;
	cinput->Buttons.ButX = false;
	cinput->Buttons.ButY = false;
	cinput->Buttons.ButStart = false;
	cinput->Buttons.ButQuit = false;
	cinput->Buttons.ButFullscreen = false;
	cinput->Buttons.RenderReset = false;
	cinput->Buttons.ButDpadLeft = false;
	cinput->Buttons.ButDpadRight = false;
	cinput->Buttons.ButDpadUp = false;
	cinput->Buttons.ButDpadDown = false;
	cinput->Buttons.ButLeft2 = false;
	cinput->Buttons.ButRight2 = false;
	cinput->Buttons.ButUp2 = false;
	cinput->Buttons.ButDown2 = false;
	cinput->Buttons.NextSource = false;
	cinput->PrevButtons = cinput->Buttons;
}

void CInput_HandleJoystickButtonEvent(CInput *cinput, int Button, bool Value) 
{
	if(DEBUG_JOYSTICK_BUTTONS)
	{
		Api->graphics->clear(kColorWhite);
		char *text = NULL;
		Api->system->formatString(&text, "Button: %d", Button);
		Api->graphics->drawText(text, strlen(text), kASCIIEncoding, 0, 0);
		Api->graphics->display();
		SDL_Delay(1000);
	}


	switch (Button)
	{
		#ifdef TRIMUI_SMART_PRO
		case SDL_CONTROLLER_BUTTON_GUIDE:
			cinput->Buttons.ButQuit = Value;
			break;
		#endif
		case SDL_CONTROLLER_BUTTON_Y:
			cinput->Buttons.ButY = Value;
			break;
		case SDL_CONTROLLER_BUTTON_X:
			cinput->Buttons.ButX = Value;
			break;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			cinput->Buttons.ButLB = Value;
			break;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			cinput->Buttons.ButRB = Value;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			cinput->Buttons.ButDpadUp = Value;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			cinput->Buttons.ButDpadDown = Value;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			cinput->Buttons.ButDpadLeft = Value;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			cinput->Buttons.ButDpadRight = Value;
			break;
		#ifdef TRIMUI_SMART_PRO
			case SDL_CONTROLLER_BUTTON_B:
		#else
			case SDL_CONTROLLER_BUTTON_A:
		#endif
			cinput->Buttons.ButA = Value;
			break;
		#ifdef TRIMUI_SMART_PRO
			case SDL_CONTROLLER_BUTTON_A:
		#else
			case SDL_CONTROLLER_BUTTON_B:
		#endif
			cinput->Buttons.ButB = Value;
			break;
		case SDL_CONTROLLER_BUTTON_START:
			cinput->Buttons.ButStart = Value;
			break;
		case SDL_CONTROLLER_BUTTON_BACK:
			cinput->Buttons.ButBack = Value;
			break;
		default:
			break;
	}
}

void CInput_HandleKeyboardEvent(CInput *cinput, int Key, bool Value)
{
	switch (Key)
	{
		case SDLK_F4:
			cinput->Buttons.ButQuit = Value;
			break;
        case SDLK_F3:
			cinput->Buttons.NextSource = Value;
			break;
		case SDLK_f:
			cinput->Buttons.ButFullscreen = Value;
			break;
        case SDLK_F8:
			cinput->Buttons.ButRB = Value;
			break;
        case SDLK_F7:
			cinput->Buttons.ButLB = Value;
			break;
		case SDLK_UP:
			cinput->Buttons.ButUp = Value;
			break;
		case SDLK_DOWN:
			cinput->Buttons.ButDown = Value;
			break;
		case SDLK_LEFT:
			cinput->Buttons.ButLeft = Value;
			break;
		case SDLK_RIGHT:
			cinput->Buttons.ButRight = Value;
			break;
		case SDLK_RETURN:
			cinput->Buttons.ButStart = Value;
			break;
		case SDLK_ESCAPE:
			cinput->Buttons.ButBack = Value;
			break;
		case SDLK_SPACE:
		case SDLK_x:
		case SDLK_a:
		case SDLK_q:
			cinput->Buttons.ButA = Value;
			break;
		case SDLK_LCTRL:
		case SDLK_RCTRL:
		case SDLK_c:
		case SDLK_b:
		case SDLK_s:
			cinput->Buttons.ButB = Value;
			break;
		case SDLK_LALT:
		case SDLK_RALT:
		case SDLK_r:
			cinput->Buttons.ButX = Value;
			break;
		case SDLK_d:
			cinput->Buttons.ButY = Value;
			break;
		case SDLK_F5:
			cinput->Buttons.ButLT = Value;
			break;
		case SDLK_F6:
			cinput->Buttons.ButRT = Value;
			break;
		default:
			break;
	}
}

void CInput_HandleJoystickAxisEvent(CInput *cinput, int Axis, int Value)
{
	switch(Axis)
	{
		case SDL_CONTROLLER_AXIS_LEFTX:
			if (abs(Value) < cinput->JoystickDeadZone)
			{
				cinput->Buttons.ButRight = false;
				cinput->Buttons.ButLeft = false;
				return;
			}
			if(Value > 0)
				cinput->Buttons.ButRight = true;
			else
				cinput->Buttons.ButLeft = true;
			break;

		case SDL_CONTROLLER_AXIS_LEFTY:
			if (abs(Value) < cinput->JoystickDeadZone)
			{
				cinput->Buttons.ButUp = false;
				cinput->Buttons.ButDown = false;
				return;
			}
			if(Value < 0)
				cinput->Buttons.ButUp = true;
			else
				cinput->Buttons.ButDown = true;
			break;

		case SDL_CONTROLLER_AXIS_RIGHTX:
			if (abs(Value) < cinput->JoystickDeadZone)
			{
				cinput->Buttons.ButRight2 = false;
				cinput->Buttons.ButLeft2 = false;
				return;
			}
			if(Value > 0)
				cinput->Buttons.ButRight2 = true;
			else
				cinput->Buttons.ButLeft2 = true;
			break;

		case SDL_CONTROLLER_AXIS_RIGHTY:
			if (abs(Value) < cinput->JoystickDeadZone)
			{
				cinput->Buttons.ButUp2 = false;
				cinput->Buttons.ButDown2 = false;
				return;
			}
			if(Value < 0)
				cinput->Buttons.ButUp2 = true;
			else
				cinput->Buttons.ButDown2 = true;
			break;

		case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
			if (abs(Value) < cinput->TriggerDeadZone)
			{
				cinput->Buttons.ButLT = false;
				return;
			}
			cinput->Buttons.ButLT = true;
			break;

		case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
			if (abs(Value) < cinput->TriggerDeadZone)
			{
				cinput->Buttons.ButRT = false;
				return;
			}
			cinput->Buttons.ButRT = true;
			break;
	}
}