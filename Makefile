# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

DEBUG = 0
CPP_BUILD ?= 0
EMSCRIPTEN_BUILD ?= 0
#if you provide other source folders this sets the number which one to use by default (for example to set colored assets as default)
DEFAULTSOURCEDIR ?= 0
#Resolution default is 400x240 if other values than 400x240 are given the scaling mode option below defines whats happens
SCREENRESX ?= 400 
SCREENRESY ?= 240
#to set the window's default size it's the resolution times this value 
WINDOWSCALE ?= 1
#SET SCALING MODE
# 0: Lower Resolutions than 400x240 cut out / crop the screen and center it, Higher resolution will center the 400x240 screen from playdate 
#   can be handy if a game is not using the full 400x240 resolution and draws everything in the center

# 1: Scale to fit window size (distortions will happen !)

# 2: Render using default SDL2 way it will letterbox if resolutions are not same aspect ration of 400x240
SCALINGMODE ?= 2
FULLSCREENATSTARTUP ?= 0

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
SDL2CONFIG = sdl2-config
CC = gcc
CPP = g++
CPP_VERSION = c++17
OUTPUT_ASSETS_DIR =
CFLAGS = -D_USE_MATH_DEFINES -DSDL2API -DTARGET_EXTENSION -Wall -Wextra -Wno-unused-parameter
LDFLAGS = -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx
CFLAGS_EXTRA = 
LDFLAGS_EXTRA =
LDUSEX11 = 1

ifneq ($(PLATFORM),)
include build_platforms/$(PLATFORM).mk
endif

PLATFORM=msys_mingw

CFLAGS += `$(SDL2CONFIG) --cflags` $(CFLAGS_EXTRA)
LDFLAGS += `$(SDL2CONFIG) --libs` $(LDFLAGS_EXTRA)

#provide OUTPUT_ASSETS_DIR in <platform>.mk to convert audio to ogg
ifneq ($(OUTPUT_ASSETS_DIR),)
#used for converting sound to ogg with ffmpeg
ALL_SOUND_MUSIC_WAV = $(call rwildcard, $(SOURCE_DIR)/,*.wav)
ALL_SOUND_MUSIC_OGG_SOURCE = $(ALL_SOUND_MUSIC_WAV:.wav=.ogg)
ALL_SOUND_MUSIC_OGG_ASSETS = $(subst $(SOURCE_DIR)/,$(OUTPUT_ASSETS_DIR)/,$(ALL_SOUND_MUSIC_OGG_SOURCE))
endif

ifeq ($(DEBUG), 1)
CFLAGS += -g
OPT_LEVEL =
endif

ifeq ($(LDUSEX11), 1)
LDFLAGS += -lX11
endif

CFLAGS += -DSCALINGMODE=$(SCALINGMODE) -DFULLSCREENATSTARTUP=$(FULLSCREENATSTARTUP) -DDEFAULTSOURCEDIR=$(DEFAULTSOURCEDIR) -DSCREENRESX=$(SCREENRESX) -DSCREENRESY=$(SCREENRESY) -DWINDOWSCALE=$(WINDOWSCALE)

.PHONY: all clean

all: $(EXE)

$(EXE): $(OUTPUT_ASSETS_DIR) $(ALL_SOUND_MUSIC_OGG_ASSETS) $(OBJS) $(INC)
	mkdir -p $(OUT_DIR)
	$(CPP) -o $(OUT_DIR)/$@ -std=$(CPP_VERSION) $(OBJS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CPP) -o $@ -std=$(CPP_VERSION) $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -o $@ $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)

$(ALL_SOUND_MUSIC_OGG_ASSETS):
	mkdir -p "$(dir $@)"
	ffmpeg -y -i "$(subst .ogg,.wav,$(subst $(OUTPUT_ASSETS_DIR)/,$(SOURCE_DIR)/,$@))" $(FFMPEG_OPTS) "$@"

$(OUTPUT_ASSETS_DIR):
	cp -r $(SOURCE_DIR) $@
	find $@ -name '*.wav' -delete

clean:
	$(RM) -rv *~ $(OBJS) $(EXE) $(OUTPUT_ASSETS_DIR)
