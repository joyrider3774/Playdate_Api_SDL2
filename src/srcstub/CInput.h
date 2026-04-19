#ifndef CINPUT_H
#define CINPUT_H

#include <SDL.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct SButtons {
	bool ButLeft, ButRight, ButUp, ButDown,
		 ButDpadLeft, ButDpadRight, ButDpadUp, ButDpadDown,
		 ButBack, ButStart, ButA, ButB,
		 ButX, ButY, ButLB, ButRB, ButFullscreen, ButQuit, ButRT, ButLT,
		 RenderReset, NextSource;
} SButtons;

typedef struct CInput {
    SDL_GameController* GameController;
	SButtons Buttons, PrevButtons;
	int JoystickDeadZone, TriggerDeadZone;
	int LeftStickX, LeftStickY;   // raw axis values for left stick (-32768..32767)
	int RightStickX, RightStickY; // raw axis values for right stick (-32768..32767)
	bool CrankUseRightStick;      // true = right stick drives crank, false = left stick
} CInput;

CInput *CInput_Create();
void CInput_Destroy(CInput* cinput);
void CInput_HandleJoystickButtonEvent(CInput *cinput, int Button, bool Value);
void CInput_HandleKeyboardEvent(CInput *cinput, int Key, bool Value);
void CInput_HandleJoystickAxisEvent(CInput *cinput, int Axis, int Value);
void CInput_Update(CInput* cinput);
void CInput_ResetButtons(CInput *cinput);
void CInput_GetPhysicalButtons(CInput* cinput, SButtons* out);

#endif