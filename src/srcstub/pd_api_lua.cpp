#include <string.h>
#include <dirent.h>
#include "pd_api/pd_api_lua.h"
#include "pd_api.h"
#include "gamestubcallbacks.h"
#include "gamestub.h"
#include "defines.h"

const char pd_api_lua_Failed[] = "Failed";

// these two return 1 on success, else 0 with an error message in outErr
int pd_api_lua_addFunction(lua_CFunction f, const char* name, const char** outErr)
{
    *outErr = pd_api_lua_Failed;
    return 0;
}

int pd_api_lua_registerClass(const char* name, const lua_reg* reg, const lua_val* vals, int isstatic, const char** outErr)
{
    *outErr = pd_api_lua_Failed;
    return 0;
}

void pd_api_lua_pushFunction(lua_CFunction f)
{

}

int pd_api_lua_indexMetatable(void)
{
    return 0;
}

void pd_api_lua_stop(void)
{

}

void pd_api_lua_start(void)
{

}

// stack operations
int pd_api_lua_getArgCount(void)
{
    return 0;
}

enum LuaType pd_api_lua_getArgType(int pos, const char** outClass)
{
    return kTypeNil;
}

int pd_api_lua_argIsNil(int pos)
{
    return 0;
}

int pd_api_lua_getArgBool(int pos)
{
    return 0;
}

int pd_api_lua_getArgInt(int pos)
{
    return 0;
}

float pd_api_lua_getArgFloat(int pos)
{
    return 0.0f;
}

const char* pd_api_lua_getArgString(int pos)
{
    return NULL;
}

const char* pd_api_lua_getArgBytes(int pos, size_t* outlen)
{
    return NULL;
}

void* pd_api_lua_getArgObject(int pos, char* type, LuaUDObject** outud)
{
    return NULL;
}

LCDBitmap* pd_api_lua_getBitmap(int pos)
{
    return NULL;
}

LCDSprite* pd_api_lua_getSprite(int pos)
{
    return NULL;
}

// for returning values back to Lua
void pd_api_lua_pushNil(void)
{

}

void pd_api_lua_pushBool(int val)
{

}

void pd_api_lua_pushInt(int val)
{

}

void pd_api_lua_pushFloat(float val)
{

}

void pd_api_lua_pushString(const char* str)
{

}

void pd_api_lua_pushBytes(const char* str, size_t len)
{

}

void pd_api_lua_pushBitmap(LCDBitmap* bitmap)
{

}

void pd_api_lua_pushSprite(LCDSprite* sprite)
{

}

LuaUDObject* pd_api_lua_pushObject(void* obj, char* type, int nValues)
{
    return NULL;
}

LuaUDObject* pd_api_lua_retainObject(LuaUDObject* obj)
{
    return NULL;
}

void pd_api_lua_releaseObject(LuaUDObject* obj)
{

}

void pd_api_lua_setUserValue(LuaUDObject* obj, unsigned int slot) // sets item on top of stack and pops it
{

} 

int pd_api_lua_getUserValue(LuaUDObject* obj, unsigned int slot) // pushes item at slot to top of stack, returns stack position
{
    return 0;
} 

// calling lua from C has some overhead. use sparingly!
void pd_api_lua_callFunction_deprecated(const char* name, int nargs)
{

}

int pd_api_lua_callFunction(const char* name, int nargs, const char** outerr)
{
    return 0;
}

playdate_lua* pd_api_lua_Create_playdate_lua()
{
	playdate_lua *Tmp = (playdate_lua*) malloc(sizeof(*Tmp));
    // these two return 1 on success, else 0 with an error message in outErr
	Tmp->addFunction = pd_api_lua_addFunction;
	Tmp->registerClass = pd_api_lua_registerClass;
	Tmp->pushFunction = pd_api_lua_pushFunction;
	Tmp->indexMetatable = pd_api_lua_indexMetatable;

	Tmp->stop = pd_api_lua_stop;
	Tmp->start = pd_api_lua_start;
	
	// stack operations
	Tmp->getArgCount = pd_api_lua_getArgCount;
	Tmp->getArgType = pd_api_lua_getArgType;

	Tmp->argIsNil = pd_api_lua_argIsNil;
	Tmp->getArgBool = pd_api_lua_getArgBool;
	Tmp->getArgInt = pd_api_lua_getArgInt;
	Tmp->getArgFloat = pd_api_lua_getArgFloat;
	Tmp->getArgString = pd_api_lua_getArgString;
	Tmp->getArgBytes = pd_api_lua_getArgBytes;
	Tmp->getArgObject = pd_api_lua_getArgObject;
	
	Tmp->getBitmap = pd_api_lua_getBitmap;
	Tmp->getSprite = pd_api_lua_getSprite;

	// for returning values back to Lua
	Tmp->pushNil = pd_api_lua_pushNil;
	Tmp->pushBool = pd_api_lua_pushBool;
	Tmp->pushInt = pd_api_lua_pushInt;
	Tmp->pushFloat = pd_api_lua_pushFloat;
	Tmp->pushString = pd_api_lua_pushString;
	Tmp->pushBytes = pd_api_lua_pushBytes;
	Tmp->pushBitmap = pd_api_lua_pushBitmap;
	Tmp->pushSprite = pd_api_lua_pushSprite;
	
	Tmp->pushObject = pd_api_lua_pushObject;
	Tmp->retainObject = pd_api_lua_retainObject;
	Tmp->releaseObject = pd_api_lua_releaseObject;
	
	Tmp->setUserValue = pd_api_lua_setUserValue;
	Tmp->getUserValue = pd_api_lua_getUserValue;

	// calling lua from C has some overhead. use sparingly!
	Tmp->callFunction_deprecated = pd_api_lua_callFunction_deprecated;
	Tmp->callFunction = pd_api_lua_callFunction;

    return Tmp;
};
