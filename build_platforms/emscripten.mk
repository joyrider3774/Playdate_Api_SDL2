#output to the html folder where an index.html will be that loads game.js
OUT_DIR = ./html
EXE = game.js
CC = emcc
CPP = em++
OPT_LEVEL = -O3
#emscripten uses their own libs only ogg seemed to play fine browser
CFLAGS = -sUSE_OGG=1 -sUSE_VORBIS=1 -sUSE_SDL=2 -sUSE_SDL_TTF=2 -sUSE_SDL_GFX=2 -sUSE_SDL_MIXER=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -D_USE_MATH_DEFINES -DSDL2API -DTARGET_EXTENSION -Wall -Wextra -Wno-unused-parameter
#memory size for game i took about 500 MB but some games require more
EMSCRIPTEN_MEMORY_SIZE=536870912
#no X11 lib
LDUSEX11 = 0

#reset Libs
LDFLAGS = 
#might be needed to prevent stutters or use sleep statements in emscripten
ifeq ($(EMSCRIPTEN_ASYNCIFY), 1)
LDFLAGS += -sASYNCIFY -lidbfs.js -s FORCE_FILESYSTEM
endif
#provide memory and assets folder
LDFLAGS += -sINITIAL_MEMORY=$(EMSCRIPTEN_MEMORY_SIZE) $(CFLAGS) --preload-file $(OUTPUT_ASSETS_DIR)@
#assets folder where a copy will be made with music and sound converted to .ogg (using ffmpeg)
OUTPUT_ASSETS_DIR = assets