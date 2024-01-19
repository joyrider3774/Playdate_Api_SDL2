#when debugging in windows need to supply -mconsole to see output of log statements
ifeq ($(DEBUG), 1)
LDFLAGS += -mconsole
endif

#MINGW / windows does not have X11 and does not require it
LDUSEX11 = 0