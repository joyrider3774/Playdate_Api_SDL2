DEBUG = 0

SRC_CPP_DIR = src/srcstub/bump src/srcstub/bump/src src/srcstub src/srcstub/pd_api
SRC_C_DIR = src/srcgame 
OBJ_DIR = ./obj
OUT_DIR = ./Source
EXE=game


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
CFLAGS = -DTARGET_EXTENSION -Wall -Wextra -Wno-unused-parameter  `sdl2-config --cflags` #-g # -Wdouble-promotion # -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE
LDLIBS = `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx -lm 

ifeq ($(DEBUG), 1)
LDLIBS += -mconsole
CFLAGS += -g
OPT_LEVEL =
endif

#MINGW does not have X11 and does not require it
#dont know about cygwin
ifneq ($(OS),Windows_NT)
LDLIBS += -lX11
endif

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS) $(INC)
	$(CPP) -o $(OUT_DIR)/$@ $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CPP) -o $@ $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -o $@ $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)
	
clean:
	$(RM) -rv *~ $(OBJS) $(EXE)