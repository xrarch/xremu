CFLAGS = -g -Ofast
SDL2_CONFIG = sdl2-config

RISC_CFLAGS = $(CFLAGS) -std=c99 `$(SDL2_CONFIG) --cflags --libs`

CFILES = src/main.c \
	src/ebus.c src/ebus.h \
	src/ram256.c src/ram256.h \
	src/lsic.c src/lsic.h \
	src/serial.c src/serial.h \
	src/pboard.c src/pboard.h \
	src/limn2500.c src/cpu.h \
	src/kinnowfb.c src/kinnowfb.h \
	src/amtsu.c src/amtsu.h \
	src/keybd.c src/keybd.h \
	src/dks.c src/dks.h

limnemu: $(CFILES)
	$(CC) -o $@ $(filter %.c, $^) $(RISC_CFLAGS)

cleanup:
	rm -f limnemu