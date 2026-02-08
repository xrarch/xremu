ifdef WEB_DEMO
	EMSCRIPTEN = 1
	SINGLE_THREAD_MP = 1
	FASTMEMORY = 1
endif

CFILES = src/main.c \
	src/xr17032fast.c \
	src/ebus.c \
	src/ram256.c \
	src/lsic.c \
	src/serial.c \
	src/pboard.c \
	src/kinnowfb.c \
	src/amtsu.c \
	src/keybd.c \
	src/dks.c \
	src/rtc.c \
	src/mouse.c \
	src/screen.c \
	src/text.c \
	src/tty.c \
	src/scheduler.c \
	src/dbg.c

HEADERS = src/fastmutex.h src/queue.h \
	src/xr.h \
	src/ebus.h \
	src/ram256.h \
	src/lsic.h \
	src/serial.h \
	src/pboard.h \
	src/kinnowfb.h \
	src/amtsu.h \
	src/keybd.h \
	src/dks.h \
	src/rtc.h \
	src/mouse.h \
	src/screen.h \
	src/text.h \
	src/tty.h \
	src/scheduler.h \
	src/xraccess.inc.c \
	src/xrfastaccess.inc.c \
	src/xrdefs.h

ifndef EMSCRIPTEN
	CFLAGS = -g -O3 -std=c99 `$(SDL2_CONFIG) --cflags`
	SDL2_CONFIG = sdl2-config
	RISC_CFLAGS = $(CFLAGS) `$(SDL2_CONFIG) --libs` -lpthread
	TARGET=xremu
	CC = clang
	OBJECTS = $(CFILES:.c=.o)
else
	CFLAGS = -O3 -mtail-call -sUSE_SDL=2
	RISC_CFLAGS = $(CFLAGS) -sINITIAL_MEMORY=33554432 -sALLOW_MEMORY_GROWTH=1 --preload-file embin
	CC = emcc
	TARGET=xremu.html
	OBJECTS = $(CFILES:.c=.emo)
endif

ifdef PROFCPU
	CFLAGS += -DPROFCPU
endif

ifdef FASTMEMORY
	CFLAGS += -DFASTMEMORY
endif

ifdef SINGLE_THREAD_MP
	CFLAGS += -DSINGLE_THREAD_MP
endif

ifdef DBG
	CFLAGS += -DDBG
endif

$(TARGET): $(OBJECTS)
ifdef EMSCRIPTEN
	mkdir -p embin
	cp bin/boot.bin embin/boot.bin
endif

	$(CC) $(RISC_CFLAGS) -o $@ $^

%.emo %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf xremu*
