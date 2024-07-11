
TOOLCHAIN = /opt/aarch64-buildroot-linux-gnu_sdk-buildroot/
TOOLCHAIN_USR = /opt/aarch64-buildroot-linux-gnu_sdk-buildroot/aarch64-buildroot-linux-gnu/sysroot/usr

CC = $(TOOLCHAIN)/bin/aarch64-linux-gcc
CPP = $(TOOLCHAIN)/bin/aarch64-linux-g++
SDL2CONFIG = $(TOOLCHAIN_USR)/bin/sdl2-config
LDUSEX11 = 0
CFLAGS_EXTRA = -DRG35XX_PLUS_BATOCERA
LDFLAGS = -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx -lpthread
