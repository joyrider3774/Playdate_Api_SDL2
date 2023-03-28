SRC_DIR = src/srcgame src/srcstub src/srcstub/pd_api
OBJ_DIR = ./obj
OUT_DIR = ./Source
EXE=game

INC = $(wildcard *.h $(foreach fd, $(SRC_DIR), $(fd)/*.h))
SRC = $(wildcard *.c $(foreach fd, $(SRC_DIR), $(fd)/*.c))
NODIR_SRC = $(notdir $(SRC))
OBJS = $(addprefix $(OBJ_DIR)/, $(SRC:c=o))
INC_DIRS = -I./ $(addprefix -I, $(SRC_DIR))

CC ?= gcc
CFLAGS = -g -DTARGET_EXTENSION -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-return-type `sdl2-config --cflags` -fexec-charset=UTF-8
OPT_LEVEL ?= -O2 
LDLIBS = `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx -lm -mconsole


#MINGW does not have X11 and does not require it
#dont know about cygwin
ifneq ($(OS),Windows_NT)
LDLIBS += -lX11
endif

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS) 
	$(CC) -o $(OUT_DIR)/$@ $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c $(INCS)
	mkdir -p $(@D)
	$(CC) -o $@ $(OPT_LEVEL) $(CFLAGS) -c $< $(INC_DIRS)

clean:
	$(RM) -rv *~ $(OBJS) $(EXE)