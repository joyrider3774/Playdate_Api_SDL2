#ifndef PD_MENU_H
#define PD_MENU_H

#include <stdbool.h>
#include <stdint.h>
#include "pd_api/pd_api_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum menu items (matches real Playdate hardware limit)
#define PD_MENU_MAX_ITEMS 3
#define PD_MENU_MAX_TITLE 64
#define PD_MENU_MAX_OPTIONS 16
#define PD_MENU_MAX_OPTION_LEN 32

typedef enum {
    PD_MENUITEM_BUTTON,
    PD_MENUITEM_CHECKMARK,
    PD_MENUITEM_OPTIONS,
} PDMenuItemType;

// Concrete PDMenuItem struct (opaque to game code via typedef in pd_api_sys.h)
struct PDMenuItem {
    PDMenuItemType type;
    char title[PD_MENU_MAX_TITLE];
    int value;                                    // checkmark: 0/1, options: index
    PDMenuItemCallbackFunction* callback;
    void* userdata;
    // options list (for options type)
    int optionCount;
    char options[PD_MENU_MAX_OPTIONS][PD_MENU_MAX_OPTION_LEN];
    bool active;                                  // slot in use
    bool interactedWith;                          // callback pending
};

// Global menu state - managed by pd_menu.cpp, read by gamestub.cpp
extern bool pd_menu_isOpen;
extern void _pd_api_sys_pauseStart(void);
extern void _pd_api_sys_pauseEnd(void);
extern void _pd_api_sys_pauseReset(void);
extern void _pd_api_sys_waitForMenuButtonRelease(void);

// Lifecycle
void pd_menu_init(void);
void pd_menu_open(void);
void pd_menu_close(void);  // fires pending callbacks then closes

// Called from runMainLoop instead of the game update when menu is open
void pd_menu_update(void); // handles up/down/A input
void pd_menu_render(void); // draws overlay via SDL/pd gfx api

// sys API implementations (called from pd_api_sys.cpp)
PDMenuItem* pd_menu_addItem(const char* title, PDMenuItemCallbackFunction* callback, void* userdata);
PDMenuItem* pd_menu_addCheckmarkItem(const char* title, int value, PDMenuItemCallbackFunction* callback, void* userdata);
PDMenuItem* pd_menu_addOptionsItem(const char* title, const char** optionTitles, int optionsCount, PDMenuItemCallbackFunction* f, void* userdata);
void        pd_menu_removeItem(PDMenuItem* item);
void        pd_menu_removeAllItems(void);
int         pd_menu_getItemValue(PDMenuItem* item);
void        pd_menu_setItemValue(PDMenuItem* item, int value);
const char* pd_menu_getItemTitle(PDMenuItem* item);
void        pd_menu_setItemTitle(PDMenuItem* item, const char* title);
void*       pd_menu_getItemUserdata(PDMenuItem* item);
void        pd_menu_setItemUserdata(PDMenuItem* item, void* ud);
void        pd_menu_setImage(LCDBitmap* bitmap, int xOffset);

#ifdef __cplusplus
}
#endif

#endif // PD_MENU_H
