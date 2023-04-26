#include <stdarg.h>
#include <stdio.h>
#include <SDL.h>
#include "debug.h"
#include "defines.h"


void printfDebug(int DebugKind, const char* fmt, ...)
{
    if (DISABLEDEBUGMESSAGES != 0)
        return;

    if (!(((DebugKind == DebugTraceFunctions) && DEBUGTRACEFUNCTIONS) ||
        ((DebugKind == DebugInfo) && DEBUGINFO) ||
        ((DebugKind == DebugNotImplementedFunctions) && DEBUGNOTIMPLEMENTEDFUNCTIONS) ||
        ((DebugKind == DebugFPS) && DEBUGFPS)))
        return;

    va_list argptr;
    va_start(argptr, fmt);
    vfprintf(stdout, fmt, argptr);
    va_end(argptr);
}