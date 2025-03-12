
TOOLCHAIN = /opt/funkey-sdk
TOOLCHAIN_USR = /opt/funkey-sdk/arm-funkey-linux-musleabihf/sysroot/usr

CC = $(TOOLCHAIN)/bin/arm-funkey-linux-musleabihf-gcc
CPP = $(TOOLCHAIN)/bin/arm-funkey-linux-musleabihf-g++
SDL2CONFIG = $(TOOLCHAIN_USR)/bin/sdl2-config
LDUSEX11 = 0
CFLAGS_EXTRA = -DFUNKEY
LDFLAGS = -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx -lpthread
OUTPUT_ASSETS_DIR = assets