DEBUG = 0
CPP_BUILD ?= 0
EMSCRIPTEN_BUILD ?= 0

SRC_CPP_DIR = src/srcstub/sdl_rotate src/srcstub/gfx_primitives_surface src/srcstub/bump src/srcstub/bump/src src/srcstub src/srcstub/pd_api
SRC_C_DIR = src/srcgame
OBJ_DIR = ./obj
OUT_DIR = ./Source
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

ifeq ($(EMSCRIPTEN_BUILD), 1)
OUT_DIR = ./html
EXE = game.js
CC = emcc
CPP = em++
OPT_LEVEL = -O3
CFLAGS = -sUSE_SDL=2 -sUSE_SDL_TTF=2 -sUSE_SDL_GFX=2 -sUSE_SDL_MIXER=2 -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' -D_USE_MATH_DEFINES -DSDL2API -DTARGET_EXTENSION -Wall -Wextra -Wno-unused-parameter
LDLIBS = 
EMSCRIPTEN_MEMORY_SIZE=536870912
ifeq ($(EMSCRIPTEN_ASYNCIFY), 1)
LDLIBS += -sASYNCIFY
endif
LDLIBS += -sINITIAL_MEMORY=$(EMSCRIPTEN_MEMORY_SIZE) $(CFLAGS) --preload-file Source@
else
CFLAGS = -D_USE_MATH_DEFINES -DSDL2API -DTARGET_EXTENSION -Wall -Wextra -Wno-unused-parameter  `sdl2-config --cflags` #-g # -Wdouble-promotion # -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE
LDLIBS = `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx -lm
endif

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

$(EXE): $(OBJS) $(INC)
	mkdir -p $(OUT_DIR)
	$(CPP) -o $(OUT_DIR)/$@ -std=$(CPP_VERSION) $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CPP) -o $@ -std=$(CPP_VERSION) $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -o $@ $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)
	
clean:
	$(RM) -rv *~ $(OBJS) $(EXE)