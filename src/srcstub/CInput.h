#ifndef CINPUT_H
#define CINPUT_H

#include <SDL.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct SButtons {
    bool ButLeft, ButRight, ButUp, ButDown, ButBack, ButStart, ButA, ButB, 
        ButX, ButY, ButLB, ButRB, ButFullscreen, ButQuit, ButRT, ButLT,
        RenderReset;
} SButtons;

typedef struct CInput {
    SDL_GameController* GameController;
    SButtons Buttons, PrevButtons;
    int JoystickDeadZone, TriggerDeadZone;
} CInput;

CInput *CInput_Create();
void CInput_Destroy(CInput* cinput);
void CInput_HandleJoystickButtonEvent(CInput *cinput, int Button, bool Value);
void CInput_HandleKeyboardEvent(CInput *cinput, int Key, bool Value);
void CInput_HandleJoystickAxisEvent(CInput *cinput, int Axis, int Value);
void CInput_Update(CInput* cinput);
void CInput_ResetButtons(CInput *cinput);

#endif