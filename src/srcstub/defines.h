#define MAXPATH 300             //max size of path (i used 300 but max is probably around 260 or so)
#define ARRAY_LIST_INCSIZES 1   //dynamic arrays minimum list sizes and increases
#define FORMAT_STRING_BUFLENGTH 10000
#define DISABLEDEBUGMESSAGES 1
#define DEBUGTRACEFUNCTIONS 0
#define DEBUGNOTIMPLEMENTEDFUNCTIONS 0
#define DEBUGINFO 0
#define DEBUGFPS 1
#define DRAWBUMPRECTS 0
#define DEBUG_JOYSTICK_BUTTONS 0
#define COLORCONVERSIONALPHATHRESHOLD 128 //255 = fully opaque
#define COLORCONVERSIONWHITETHRESHOLD 128 //255 = fully white 0 = fully black
#define ENABLEBITMAPMASKS true // THEY DO NOT WORK FULLY ! i only added it for my puzzleland and retrotime game which is basic usage (it only creates a masked image when calling setmask)
#ifndef SCREENRESX
	#define SCREENRESX 400
#endif
#ifndef SCREENRESY
	#define SCREENRESY 240
#endif
#ifndef WINDOWSCALE
	#define WINDOWSCALE 1
#endif
#ifndef DEFAULTSOURCEDIR
	#define DEFAULTSOURCEDIR 0
#endif
#ifndef FULLSCREENATSTARTUP
	#define FULLSCREENATSTARTUP false
#endif