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
    pd_menu_isOpen = true;
    _selectedIndex = 0;
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

    _pd_api_sys_waitForMenuButtonRelease();
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
    if (!pd_menu_isOpen || !_pd_api_sys_input)
        return;

    bool up   = _pd_api_sys_input->Buttons.ButUp   && !_pd_api_sys_input->PrevButtons.ButUp;
    bool down = _pd_api_sys_input->Buttons.ButDown  && !_pd_api_sys_input->PrevButtons.ButDown;
    bool a    = _pd_api_sys_input->Buttons.ButA     && !_pd_api_sys_input->PrevButtons.ButA;
    bool back = _pd_api_sys_input->Buttons.ButB && !_pd_api_sys_input->PrevButtons.ButB; 
    bool left  = _pd_api_sys_input->Buttons.ButLeft  && !_pd_api_sys_input->PrevButtons.ButLeft;
    bool right = _pd_api_sys_input->Buttons.ButRight && !_pd_api_sys_input->PrevButtons.ButRight;
    
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
            item->interactedWith = true;
            switch (item->type)
            {
                case PD_MENUITEM_BUTTON:
                //     // left/right on a button item closes and fires, same as A
                //     pd_menu_close();
                    break;

                case PD_MENUITEM_CHECKMARK:
                    item->value = !item->value;
                    break;

                case PD_MENUITEM_OPTIONS:
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
    if (back)
        pd_menu_close();
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

    // --- background: draw game image on left half if set, white panel on right ---
    if (_menuImage)
        Api->graphics->drawBitmap(_menuImage, _menuImageXOff, 0, kBitmapUnflipped);

    // Right-half white panel
    const int panelX = SCREENRESX / 2;
    const int panelW = SCREENRESX / 2;
    const int panelH = SCREENRESY;

    Api->graphics->fillRect(panelX, 0, panelW, panelH, kColorWhite);
    Api->graphics->drawRect(panelX, 0, panelW, panelH, kColorBlack);

    // --- title row at top ---
    const char* title = "MENU";
    Api->graphics->drawText(title, strlen(title), kASCIIEncoding, panelX + 8, 4);

    // Separator line under title
    Api->graphics->drawLine(panelX + 2, 22, SCREENRESX - 2, 22, 1, kColorBlack);

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
            Api->graphics->fillRect(panelX + 2, itemY, panelW - 4, itemH - 2, kColorBlack);

            // Temporarily swap black->white so TTF renders font glyphs in white
            SDL_Color savedBlack = pd_api_gfx_color_black;
            pd_api_gfx_color_black = pd_api_gfx_color_white;

            if (item->type == PD_MENUITEM_OPTIONS && item->optionCount > 0)
            {
                Api->graphics->drawText(displayTitle, strlen(displayTitle), kASCIIEncoding,
                                        panelX + paddingX, itemY + 2);
                const char* optStr = item->options[item->value];
                char optDisplay[PD_MENU_MAX_OPTION_LEN + 4];
                snprintf(optDisplay, sizeof(optDisplay), "  > %s", optStr);
                Api->graphics->drawText(optDisplay, strlen(optDisplay), kASCIIEncoding,
                                        panelX + paddingX, itemY + 18);
            }
            else
            {
                Api->graphics->drawText(displayTitle, strlen(displayTitle), kASCIIEncoding,
                                        panelX + paddingX, itemY + 10);
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
                                        panelX + paddingX, itemY + 2);
                const char* optStr = item->options[item->value];
                char optDisplay[PD_MENU_MAX_OPTION_LEN + 4];
                snprintf(optDisplay, sizeof(optDisplay), "  > %s", optStr);
                Api->graphics->drawText(optDisplay, strlen(optDisplay), kASCIIEncoding,
                                        panelX + paddingX, itemY + 18);
            }
            else
            {
                Api->graphics->drawText(displayTitle, strlen(displayTitle), kASCIIEncoding,
                                        panelX + paddingX, itemY + 10);
            }
        }

        activeIdx++;
    }

    // --- hint at bottom ---
    Api->graphics->drawText("A:select  B:close", strlen("A:select  B:close"),
                            kASCIIEncoding, panelX + 4, panelH - 18);

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
