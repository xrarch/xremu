# LIMNemu (C+SDL)

Emulates the LIMNstation fantasy computer, inspired by late 80s Unix workstations.

The long-term goal is to create a really neat (but useless) emulated desktop computer.

Ships with a pre-built [boot ROM](https://github.com/limnarch/a3x) binary.

## Running

Make sure you have SDL2 installed.

Building the emulator should be as simple as typing `make`.

Then, type `./graphical.sh` in the project directory.

Striking the right ALT key will switch the display between the framebuffer and the serial TTYs.

    -ramsize [bytes]
	    Specify the size of RAM in bytes.

    -dks [diskimage]
	    Attach a file as a disk image.

    -nvram [file]
	    Specify an NVRAM file.

    -rom [file]
	    Specify a file to use as the boot ROM.

    -asyncdisk
	    Simulate disk seek times.

    -asyncserial
	    Simulate serial latency.

	-headless
	    Don't attach framebuffer, keyboard, or mouse.