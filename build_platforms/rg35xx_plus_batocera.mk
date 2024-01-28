
TOOLCHAIN = /opt/rg35xx/
TOOLCHAIN_USR = /opt/rg35xx/arm-buildroot-linux-gnueabihf/sysroot/usr

CC = $(TOOLCHAIN)/bin/arm-linux-gcc
CPP = $(TOOLCHAIN)/bin/arm-linux-g++
SDL2CONFIG = $(TOOLCHAIN_USR)/bin/sdl2-config
LDUSEX11 = 0
CFLAGS_EXTRA = -DRG35XX_PLUS_BATOCERA
LDFLAGS = -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx -lpthread
