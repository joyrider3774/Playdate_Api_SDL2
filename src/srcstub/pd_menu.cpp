#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include "pd_menu.h"
#include "pd_api.h"
#include "gamestub.h"
#include "gamestubcallbacks.h"
#include "defines.h"
#include "debug.h"
#include <SDL_mixer.h>
#include "CInput.h"

extern void pd_api_sound_resumeAllFilePlayers(void);
extern void pd_api_sound_pauseAllFilePlayers(void);

// ============================================================
//  State
// ============================================================

bool pd_menu_isOpen = false;

static PDMenuItem   _items[PD_MENU_MAX_ITEMS];
static int          _selectedIndex = 0;
static LCDBitmap*   _menuImage     = NULL;
static LCDBitmap*   _screenSnapshot  = NULL;
static int          _menuImageXOff = 0;

// Tracks whether the menu button was held last frame (for edge detection)
static bool         _prevMenuButton = false;

// Forward-declared here; defined in pd_api_sys.cpp
extern struct CInput* _pd_api_sys_input;

// Separate CInput instance used exclusively by the menu.
// Created on menu open, destroyed on menu close.
// Keeps menu navigation input isolated from the game's input state so that
// when the menu closes, the main _pd_api_sys_input can be fully reset —
// matching the real Playdate firmware which presents clean input state to
// the game after the system menu closes.
static CInput* _pd_menu_input = NULL;

// ============================================================
//  Init
// ============================================================

void pd_menu_init(void)
{
    memset(_items, 0, sizeof(_items));
    _selectedIndex  = 0;
    _menuImage      = NULL;
    _screenSnapshot = NULL;
    _menuImageXOff  = 0;
    pd_menu_isOpen  = false;
    _prevMenuButton = false;
    _pd_api_sys_pauseReset();
}

// ============================================================
//  Open / close
// ============================================================

void pd_menu_open(void)
{
    if (pd_menu_isOpen)
        return;
    _pd_api_sys_pauseStart();
    Mix_Pause(-1);   // pause all SDL_mixer channels
    pd_api_sound_pauseAllFilePlayers(); // freeze FilePlayer wall-clock timers
    pd_menu_isOpen = true;
    _selectedIndex = 0;
    // Create a fresh input instance for menu navigation, isolated from game input
    if (_pd_menu_input)
        CInput_Destroy(_pd_menu_input);
    _pd_menu_input = CInput_Create();
    // Prime with one update so PrevButtons reflects currently-held buttons.
    // Without this, any button held at menu-open time would appear as a fresh
    // press on the first pd_menu_update call and immediately fire/close the menu.
    CInput_Update(_pd_menu_input);
    // Sync PrevButtons = Buttons so no pushed events fire on the first update.
    _pd_menu_input->PrevButtons = _pd_menu_input->Buttons;
    // Snapshot current screen so we can redraw it when the menu closes
    if (_screenSnapshot)
        Api->graphics->freeBitmap(_screenSnapshot);
    _screenSnapshot = Api->graphics->copyBitmap(_pd_api_gfx_Playdate_Screen);
    // Fire Pause system event so the game knows (same as real Playdate)
    eventHandler(Api, kEventPause, 0);
    printfDebug(DebugInfo, "pd_menu: opened\n");    
}

void pd_menu_close(void)
{
    if (!pd_menu_isOpen)
        return;

    // Destroy menu input instance
    if (_pd_menu_input)
    {
        CInput_Destroy(_pd_menu_input);
        _pd_menu_input = NULL;
    }

    // Reset the main game input state after menu close.
    // Zero Buttons, but then restore any direction keys that are still
    // physically held (via SDL_GetKeyboardState) so no spurious released
    // event fires for them — matching real Playdate firmware behaviour.
    if (_pd_api_sys_input)
    {
        memset(&_pd_api_sys_input->Buttons, 0, sizeof(_pd_api_sys_input->Buttons));

        // Re-apply physically-held direction/action keys so they don't
        // appear as released on the first post-menu frame.
        const Uint8* ks = SDL_GetKeyboardState(NULL);
        if (ks[SDL_SCANCODE_LEFT])  _pd_api_sys_input->Buttons.ButLeft  = true;
        if (ks[SDL_SCANCODE_RIGHT]) _pd_api_sys_input->Buttons.ButRight = true;
        if (ks[SDL_SCANCODE_UP])    _pd_api_sys_input->Buttons.ButUp    = true;
        if (ks[SDL_SCANCODE_DOWN])  _pd_api_sys_input->Buttons.ButDown  = true;
        if (ks[SDL_SCANCODE_SPACE] || ks[SDL_SCANCODE_X] || ks[SDL_SCANCODE_A] || ks[SDL_SCANCODE_Q])
            _pd_api_sys_input->Buttons.ButA = true;
        if (ks[SDL_SCANCODE_LCTRL] || ks[SDL_SCANCODE_RCTRL] || ks[SDL_SCANCODE_C] || ks[SDL_SCANCODE_B] || ks[SDL_SCANCODE_S])
            _pd_api_sys_input->Buttons.ButB = true;

        // Also check gamepad physical state
        SDL_GameController* gc = _pd_api_sys_input->GameController;
        if (gc)
        {
            if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  _pd_api_sys_input->Buttons.ButDpadLeft  = true;
            if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) _pd_api_sys_input->Buttons.ButDpadRight = true;
            if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP))    _pd_api_sys_input->Buttons.ButDpadUp    = true;
            if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  _pd_api_sys_input->Buttons.ButDpadDown  = true;
#if defined(TRIMUI_SMART_PRO)
            if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B)) _pd_api_sys_input->Buttons.ButA = true;
            if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A)) _pd_api_sys_input->Buttons.ButB = true;
#else
            if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A)) _pd_api_sys_input->Buttons.ButA = true;
            if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B)) _pd_api_sys_input->Buttons.ButB = true;
#endif
        }

        _pd_api_sys_skipPrevButtonUpdate = true;
    }
    // Need to redraw here before firing the callbacks as the callbacks could draw something as well
    if (_screenSnapshot)
    {
        Api->graphics->pushContext(NULL);
        Api->graphics->setDrawOffset(0,0);
        Api->graphics->drawBitmap(_screenSnapshot,0,0,kBitmapUnflipped);
        Api->graphics->freeBitmap(_screenSnapshot);
        _screenSnapshot = NULL;
        Api->graphics->popContext();
    }   
    // Fire callbacks for any items that were interacted with
    for (int i = 0; i < PD_MENU_MAX_ITEMS; i++)
    {
        if (!_items[i].active || !_items[i].interactedWith)
            continue;
        _items[i].interactedWith = false;
        if (_items[i].callback)
            _items[i].callback(_items[i].userdata);
    }
    
    pd_menu_isOpen = false;
        
    Mix_Resume(-1);   // resume all SDL_mixer channels  
    pd_api_sound_resumeAllFilePlayers(); // fix up FilePlayer wall-clock pause accounting
    _pd_api_sys_pauseEnd();
    Api->system->resetElapsedTime();
    // Fire Resume system event 
    eventHandler(Api, kEventResume, 0);
    printfDebug(DebugInfo, "pd_menu: closed\n");
}

// ============================================================
//  Count active items
// ============================================================

static int activeItemCount(void)
{
    int n = 0;
    for (int i = 0; i < PD_MENU_MAX_ITEMS; i++)
        if (_items[i].active) n++;
    return n;
}

// Map slot index to nth active item
static PDMenuItem* nthActiveItem(int n)
{
    int seen = 0;
    for (int i = 0; i < PD_MENU_MAX_ITEMS; i++)
    {
        if (!_items[i].active) continue;
        if (seen == n) return &_items[i];
        seen++;
    }
    return NULL;
}

// ============================================================
//  Update (called from runMainLoop when menu is open)
//  Input is read from _pd_api_sys_input (CInput) directly.
//  Edge-detection: only act on newly pressed buttons.
// ============================================================

void pd_menu_update(void)
{
    if (!pd_menu_isOpen || !_pd_menu_input)
        return;

    CInput_Update(_pd_menu_input);

    bool up   = _pd_menu_input->Buttons.ButUp   && !_pd_menu_input->PrevButtons.ButUp;
    bool down = _pd_menu_input->Buttons.ButDown  && !_pd_menu_input->PrevButtons.ButDown;
    bool a    = _pd_menu_input->Buttons.ButA     && !_pd_menu_input->PrevButtons.ButA;
    bool back = _pd_menu_input->Buttons.ButB     && !_pd_menu_input->PrevButtons.ButB;
    bool start = _pd_menu_input->Buttons.ButStart && !_pd_menu_input->PrevButtons.ButStart;
    bool left  = _pd_menu_input->Buttons.ButLeft  && !_pd_menu_input->PrevButtons.ButLeft;
    bool right = _pd_menu_input->Buttons.ButRight && !_pd_menu_input->PrevButtons.ButRight;
    
    int count = activeItemCount();

    if (up && _selectedIndex > 0)
        _selectedIndex--;

    if (down && _selectedIndex < count - 1)
        _selectedIndex++;
    
    if ((left || right) && count > 0)
    {
        PDMenuItem* item = nthActiveItem(_selectedIndex);
        if (item)
        {            
            switch (item->type)
            {
                case PD_MENUITEM_BUTTON:
                //     // left/right on a button item closes and fires, same as A
                //     pd_menu_close();
                    return;

                case PD_MENUITEM_CHECKMARK:
                    item->interactedWith = true;
                    item->value = !item->value;
                    break;

                case PD_MENUITEM_OPTIONS:
                    item->interactedWith = true;
                    if (item->optionCount > 0)
                    {
                        if (right)
                            item->value = (item->value + 1) % item->optionCount;
                        else
                            item->value = (item->value - 1 + item->optionCount) % item->optionCount;
                    }
                    break;
            }
        }
    }
    
    if (a && count > 0)
    {
        PDMenuItem* item = nthActiveItem(_selectedIndex);
        if (item)
        {
            item->interactedWith = true;
            switch (item->type)
            {
                case PD_MENUITEM_BUTTON:
                    // Close immediately and fire callback
                    pd_menu_close();
                    return;

                case PD_MENUITEM_CHECKMARK:
                    item->value = !item->value;
                    break;

                case PD_MENUITEM_OPTIONS:
                    if (item->optionCount > 0)
                        item->value = (item->value + 1) % item->optionCount;
                    break;
            }
        }
    }

    // B or menu key closes without firing callbacks
    if (back || start)
    {
        pd_menu_close();
        return; // _pd_menu_input is now NULL — don't access it further
    }

    // System keys work even during menu
    if (_pd_menu_input->Buttons.ButQuit)
        _pd_api_sys_quitCallBack();

    if (_pd_menu_input->Buttons.ButFullscreen && !_pd_menu_input->PrevButtons.ButFullscreen)
        _pd_api_sys_fullScreenCallBack();

    if (_pd_menu_input->Buttons.RenderReset)
        _pd_api_sys_renderResetCallBack();

    if ((_pd_menu_input->Buttons.ButX || _pd_menu_input->Buttons.NextSource) &&
        !(_pd_menu_input->PrevButtons.ButX || _pd_menu_input->PrevButtons.NextSource))
        _pd_api_sys_nextSourceDirCallback();
}

// ============================================================
//  Render
//  Draws a simple overlay on the right half of the screen,
//  matching the real Playdate menu layout.
// ============================================================

void pd_menu_render(void)
{
    if (!pd_menu_isOpen)
        return;

    // Push a clean graphics context; popContext at the end restores everything
    Api->graphics->pushContext(NULL);
    Api->graphics->setFont(NULL);
    Api->graphics->setDrawMode(kDrawModeCopy);

    // Right-half white panel
    
    const int panelX = LCD_COLUMNS / 2;
    const int panelW = LCD_COLUMNS / 2;
    #if SCALINGMODE == 0
    const int panelH = LCD_ROWS <= SCREENRESY ? LCD_ROWS: LCD_ROWS - (LCD_ROWS - SCREENRESY);
    const int panelY = LCD_ROWS <= SCREENRESY ? 0 : (LCD_ROWS - SCREENRESY) / 2;
    #else
    const int panelH = LCD_ROWS;
    const int panelY = 0;
    #endif 

    // --- background: draw game image on left half if set, white panel on right ---
    if (_menuImage)
        Api->graphics->drawBitmap(_menuImage, _menuImageXOff, 0, kBitmapUnflipped);

    Api->graphics->fillRect(panelX, panelY, panelW, panelH, kColorWhite);
    Api->graphics->drawRect(panelX, panelY, panelW, panelH, kColorBlack);

    // --- title row at top ---
    const char* title = "MENU";
    Api->graphics->drawText(title, strlen(title), kASCIIEncoding, panelX + 8, panelY + 4);

    // Separator line under title
    Api->graphics->drawLine(panelX + 2, panelY + 22, LCD_COLUMNS - 2, panelY + 22, 1, kColorBlack);

    // --- items ---
    const int itemH    = 40;
    const int startY   = 26;
    const int paddingX = 8;

    int activeIdx = 0;
    for (int i = 0; i < PD_MENU_MAX_ITEMS; i++)
    {
        if (!_items[i].active)
            continue;

        PDMenuItem* item = &_items[i];
        bool selected = (activeIdx == _selectedIndex);
        int itemY = startY + activeIdx * itemH;

        // Build display title
        char displayTitle[PD_MENU_MAX_TITLE + 4];
        snprintf(displayTitle, sizeof(displayTitle), "%s", item->title);

        // For checkmark: append [X] or [ ]
        if (item->type == PD_MENUITEM_CHECKMARK)
        {
            char combined[PD_MENU_MAX_TITLE + 8];
            snprintf(combined, sizeof(combined), "%s [%c]", item->title, item->value ? 'X' : ' ');
            strncpy(displayTitle, combined, sizeof(displayTitle) - 1);
        }

        if (selected)
        {
            // Fill selection bar black
            Api->graphics->fillRect(panelX + 2, panelY + itemY, panelW - 4, itemH - 2, kColorBlack);

            // Temporarily swap black->white so TTF renders font glyphs in white
            SDL_Color savedBlack = pd_api_gfx_color_black;
            pd_api_gfx_color_black = pd_api_gfx_color_white;

            if (item->type == PD_MENUITEM_OPTIONS && item->optionCount > 0)
            {
                Api->graphics->drawText(displayTitle, strlen(displayTitle), kASCIIEncoding,
                                        panelX + paddingX, panelY + itemY + 2);
                const char* optStr = item->options[item->value];
                char optDisplay[PD_MENU_MAX_OPTION_LEN + 4];
                snprintf(optDisplay, sizeof(optDisplay), "  > %s", optStr);
                Api->graphics->drawText(optDisplay, strlen(optDisplay), kASCIIEncoding,
                                        panelX + paddingX, panelY + itemY + 18);
            }
            else
            {
                Api->graphics->drawText(displayTitle, strlen(displayTitle), kASCIIEncoding,
                                        panelX + paddingX, panelY + itemY + 10);
            }

            // Restore
            pd_api_gfx_color_black = savedBlack;
        }
        else
        {
            // Unselected: plain black text on white background
            if (item->type == PD_MENUITEM_OPTIONS && item->optionCount > 0)
            {
                Api->graphics->drawText(displayTitle, strlen(displayTitle), kASCIIEncoding,
                                        panelX + paddingX, panelY + itemY + 2);
                const char* optStr = item->options[item->value];
                char optDisplay[PD_MENU_MAX_OPTION_LEN + 4];
                snprintf(optDisplay, sizeof(optDisplay), "  > %s", optStr);
                Api->graphics->drawText(optDisplay, strlen(optDisplay), kASCIIEncoding,
                                        panelX + paddingX, panelY + itemY + 18);
            }
            else
            {
                Api->graphics->drawText(displayTitle, strlen(displayTitle), kASCIIEncoding,
                                        panelX + paddingX, panelY + itemY + 10);
            }
        }

        activeIdx++;
    }

    // --- hint at bottom ---
    Api->graphics->drawText("A:select  B:close", strlen("A:select  B:close"),
                            kASCIIEncoding, panelX + 4, panelY + panelH - 18);

    // Restore full graphics context (font, draw mode, clip rect, etc.)
    Api->graphics->popContext();
}

// ============================================================
//  sys API implementations
// ============================================================

static PDMenuItem* allocSlot(void)
{
    for (int i = 0; i < PD_MENU_MAX_ITEMS; i++)
        if (!_items[i].active)
            return &_items[i];
    printfDebug(DebugInfo, "pd_menu: no free slots (max %d items)\n", PD_MENU_MAX_ITEMS);
    return NULL;
}

PDMenuItem* pd_menu_addItem(const char* title, PDMenuItemCallbackFunction* callback, void* userdata)
{
    PDMenuItem* item = allocSlot();
    if (!item) return NULL;
    memset(item, 0, sizeof(*item));
    item->active   = true;
    item->type     = PD_MENUITEM_BUTTON;
    item->callback = callback;
    item->userdata = userdata;
    strncpy(item->title, title ? title : "", PD_MENU_MAX_TITLE - 1);
    return item;
}

PDMenuItem* pd_menu_addCheckmarkItem(const char* title, int value, PDMenuItemCallbackFunction* callback, void* userdata)
{
    PDMenuItem* item = allocSlot();
    if (!item) return NULL;
    memset(item, 0, sizeof(*item));
    item->active   = true;
    item->type     = PD_MENUITEM_CHECKMARK;
    item->value    = value ? 1 : 0;
    item->callback = callback;
    item->userdata = userdata;
    strncpy(item->title, title ? title : "", PD_MENU_MAX_TITLE - 1);
    return item;
}

PDMenuItem* pd_menu_addOptionsItem(const char* title, const char** optionTitles, int optionsCount, PDMenuItemCallbackFunction* f, void* userdata)
{
    PDMenuItem* item = allocSlot();
    if (!item) return NULL;
    memset(item, 0, sizeof(*item));
    item->active      = true;
    item->type        = PD_MENUITEM_OPTIONS;
    item->value       = 0;
    item->callback    = f;
    item->userdata    = userdata;
    item->optionCount = optionsCount < PD_MENU_MAX_OPTIONS ? optionsCount : PD_MENU_MAX_OPTIONS;
    strncpy(item->title, title ? title : "", PD_MENU_MAX_TITLE - 1);
    for (int i = 0; i < item->optionCount; i++)
        strncpy(item->options[i], optionTitles[i] ? optionTitles[i] : "", PD_MENU_MAX_OPTION_LEN - 1);
    return item;
}

void pd_menu_removeItem(PDMenuItem* item)
{
    if (!item) return;
    item->active = false;
    // Clamp selectedIndex so it never goes out of range
    int count = activeItemCount();
    if (_selectedIndex >= count && count > 0)
        _selectedIndex = count - 1;
    if (count == 0)
        _selectedIndex = 0;
}

void pd_menu_removeAllItems(void)
{
    for (int i = 0; i < PD_MENU_MAX_ITEMS; i++)
        _items[i].active = false;
    _selectedIndex = 0;
}

int pd_menu_getItemValue(PDMenuItem* item)
{
    return item ? item->value : 0;
}

void pd_menu_setItemValue(PDMenuItem* item, int value)
{
    if (!item) return;
    if (item->type == PD_MENUITEM_OPTIONS && item->optionCount > 0)
        item->value = value % item->optionCount;
    else
        item->value = value;
}

const char* pd_menu_getItemTitle(PDMenuItem* item)
{
    return item ? item->title : NULL;
}

void pd_menu_setItemTitle(PDMenuItem* item, const char* title)
{
    if (!item || !title) return;
    strncpy(item->title, title, PD_MENU_MAX_TITLE - 1);
    item->title[PD_MENU_MAX_TITLE - 1] = '\0';
}

void* pd_menu_getItemUserdata(PDMenuItem* item)
{
    return item ? item->userdata : NULL;
}

void pd_menu_setItemUserdata(PDMenuItem* item, void* ud)
{
    if (item) item->userdata = ud;
}

void pd_menu_setImage(LCDBitmap* bitmap, int xOffset)
{
    _menuImage     = bitmap;
    _menuImageXOff = xOffset;
}
