#ifndef debug_h
#define debug_h

const int DebugTraceFunctions = 1;
const int DebugInfo = 2;
const int DebugNotImplementedFunctions = 3;
const int DebugLoadPaths = 4;
const int DebugFPS = 5;

void printfDebug(int DebugKind, const char* fmt, ...);

#endif