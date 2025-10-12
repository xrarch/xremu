ifdef WEB_DEMO
	EMSCRIPTEN = 1
	SINGLE_THREAD_MP = 1
	FASTMEMORY = 1
endif

ifndef EMSCRIPTEN
	CFLAGS = -g -O3
	SDL2_CONFIG = sdl2-config
	RISC_CFLAGS = $(CFLAGS) -std=c99 `$(SDL2_CONFIG) --cflags --libs` -lpthread
	TARGET=xremu
	CC = clang
else
	CFLAGS = -O3 -sUSE_SDL=2 -sINITIAL_MEMORY=33554432 -sALLOW_MEMORY_GROWTH=1 -mtail-call
	RISC_CFLAGS = $(CFLAGS) -std=c99 --preload-file embin
	CC = emcc
	TARGET=xremu.html
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

CFILES = src/main.c \
	src/fastmutex.h src/queue.h \
	src/xr17032fast.c src/xr.h \
	src/ebus.c src/ebus.h \
	src/ram256.c src/ram256.h \
	src/lsic.c src/lsic.h \
	src/serial.c src/serial.h \
	src/pboard.c src/pboard.h \
	src/kinnowfb.c src/kinnowfb.h \
	src/amtsu.c src/amtsu.h \
	src/keybd.c src/keybd.h \
	src/dks.c src/dks.h \
	src/rtc.c src/rtc.h \
	src/mouse.c src/mouse.h \
	src/screen.c src/screen.h \
	src/text.c src/text.h \
	src/tty.c src/tty.h \
	src/scheduler.c src/scheduler.h \
	src/dbg.c

$(TARGET): $(CFILES)
ifdef EMSCRIPTEN
	mkdir -p embin
	cp bin/boot.bin embin/boot.bin
endif

	$(CC) -o $@ $(filter %.c, $^) $(RISC_CFLAGS)

clean:
	rm -rf xremu*
