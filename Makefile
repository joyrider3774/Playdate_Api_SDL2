# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

DEBUG = 0
CPP_BUILD ?= 0
EMSCRIPTEN_BUILD ?= 0

SRC_CPP_DIR = src/srcstub/sdl_rotate src/srcstub/gfx_primitives_surface src/srcstub/bump src/srcstub/bump/src src/srcstub src/srcstub/pd_api
SRC_C_DIR = src/srcgame
OBJ_DIR = ./obj
OUT_DIR = ./Source
SOURCE_DIR = Source
EXE=game

ifeq ($(CPP_BUILD), 1)
SRC_CPP_DIR += src/srcgame
SRC_C_DIR =
endif

INC = $(wildcard *.h $(foreach fd, $(SRC_C_DIR), $(fd)/*.h)) $(wildcard *.hpp $(foreach fd, $(SRC_CPP_DIR), $(fd)/*.hpp))
SRC = $(wildcard *.c $(foreach fd, $(SRC_C_DIR), $(fd)/*.c)) 
SRCCPP = $(wildcard *.cpp $(foreach fd, $(SRC_CPP_DIR), $(fd)/*.cpp)) 
NODIR_SRC = $(notdir $(SRC_C_DIR)) $(notdir $(SRC_CPP_DIR))
OBJSC = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o)) 
OBJSCPP = $(addprefix $(OBJ_DIR)/, $(SRCCPP:.cpp=.o))
OBJS = $(OBJSC) $(OBJSCPP)
INC_DIRS = -I./ $(addprefix -I, $(SRC_C_DIR)) $(addprefix -I, $(SRC_CPP_DIR))

OPT_LEVEL ?= -O2
CC = gcc
CPP = g++
CPP_VERSION = c++17
OUTPUT_ASSETS_DIR =
CFLAGS = -D_USE_MATH_DEFINES -DSDL2API -DTARGET_EXTENSION -Wall -Wextra -Wno-unused-parameter  `sdl2-config --cflags` #-g # -Wdouble-promotion # -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE
LDLIBS = `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx -lm

ifeq ($(EMSCRIPTEN_BUILD), 1)
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

#reset Libs
LDLIBS = 
#might be needed to prevent stutters or use sleep statements in emscripten
ifeq ($(EMSCRIPTEN_ASYNCIFY), 1)
LDLIBS += -sASYNCIFY
endif
#provide memory and assets folder
LDLIBS += -sINITIAL_MEMORY=$(EMSCRIPTEN_MEMORY_SIZE) $(CFLAGS) --preload-file $(OUTPUT_ASSETS_DIR)@

#assets folder where a copy will be made with music and sound converted to .ogg (using ffmpeg)
OUTPUT_ASSETS_DIR = assets


#used for converting sound to ogg with ffmpeg
ALL_SOUND_MUSIC_WAV = $(call rwildcard, $(SOURCE_DIR)/,*.wav)
ALL_SOUND_MUSIC_OGG_SOURCE = $(ALL_SOUND_MUSIC_WAV:.wav=.ogg)
ALL_SOUND_MUSIC_OGG_ASSETS = $(subst $(SOURCE_DIR)/,$(OUTPUT_ASSETS_DIR)/,$(ALL_SOUND_MUSIC_OGG_SOURCE))
endif

#when debugging in windows need to supply -mconsole to see output of log statements
ifeq ($(DEBUG), 1)
ifeq ($(OS),Windows_NT)
LDLIBS += -mconsole
endif
CFLAGS += -g
OPT_LEVEL =
endif

#MINGW does not have X11 and does not require it
#dont know about cygwin
ifneq ($(OS),Windows_NT)
ifeq ($(EMSCRIPTEN_BUILD),0)
LDLIBS += -lX11
endif
endif

.PHONY: all clean

all: $(EXE)

$(EXE): $(OUTPUT_ASSETS_DIR) $(ALL_SOUND_MUSIC_OGG_ASSETS) $(OBJS) $(INC)
	mkdir -p $(OUT_DIR)
	$(CPP) -o $(OUT_DIR)/$@ -std=$(CPP_VERSION) $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CPP) -o $@ -std=$(CPP_VERSION) $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -o $@ $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)

$(ALL_SOUND_MUSIC_OGG_ASSETS):
	mkdir -p "$(dir $@)"
	ffmpeg -y -i "$(subst .ogg,.wav,$(subst $(OUTPUT_ASSETS_DIR)/,$(SOURCE_DIR)/,$@))" "$@"

$(OUTPUT_ASSETS_DIR):
	cp -r $(SOURCE_DIR) $@
	find $@ -name '*.wav' -delete

clean:
	$(RM) -rv *~ $(OBJS) $(EXE) $(OUTPUT_ASSETS_DIR)
