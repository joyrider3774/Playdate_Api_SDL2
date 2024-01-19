
TOOLCHAIN = /opt/trimui-sdk/aarch64-linux-gnu-7.5.0-linaro
TOOLCHAIN_USR = /opt/trimui-sdk/aarch64-linux-gnu-7.5.0-linaro/usr

CC = $(TOOLCHAIN)/bin/aarch64-linux-gnu-gcc
CPP = $(TOOLCHAIN)/bin/aarch64-linux-gnu-g++
SDL2CONFIG = $(TOOLCHAIN_USR)/bin/sdl2-config
LDUSEX11 = 0
CFLAGS_EXTRA = -DTRIMUI_SMART_PRO
LDFLAGS = -lSDL2_image -lSDL2_ttf -lSDL2_mixer $(TOOLCHAIN_USR)/lib/libSDL2_gfx.a -lpthread
