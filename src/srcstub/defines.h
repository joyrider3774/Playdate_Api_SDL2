#define MAXPATH 300             //max size of path (i used 300 but max is probably around 260 or so)
#define MAX_SIM_SOUNDS 100      //max individual sound channels (one for every unique audio asset)
#define GFX_STACK_MAX_SIZE 100  //max size of the gfx stack for push and pop context (i doubt any program exceeds 100 except if they have a bug)
#define FONT_LIST_MAX_SIZE 100  //max number of different fonts we can load (i doubt any program will have more then 100 unique fonts)
#define ARRAY_LIST_INCSIZES 1 //dynamic arrays minimum list sizes and increases
#define FORMAT_STRING_BUFLENGTH 10000