# LIMNemu (C+SDL)

Emulates the LIMNstation fantasy computer, inspired by late 80s Unix workstations.

The long-term goal is to create a really neat (but useless) emulated desktop computer.

Ships with a pre-built [boot ROM](https://github.com/limnarch/a3x) binary.

## Running

Make sure you have SDL2 installed.

Building the emulator should be as simple as typing `make`.

Then, type `./graphical.sh` in the project directory.

Striking the right ALT key will switch the display between the framebuffer and the serial TTYs.

For performance reasons, disk and serial operations will appear to complete instantly by default,
and caches aren't simulated. In order to test for accuracy in systems programming these things can
be simulated using the switches `-asyncdisk`, `-asyncserial`, and `-cachesim` respectively.

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

	-cachesim
		Simulate an I-cache and a writethrough D-cache with a 4-entry writebuffer.

	-cachemiss
		Simulate cycle delays on cache misses.

	-cacheprint
		Print cache statistics every 2 seconds.

	-headless
	    Don't attach framebuffer, keyboard, or mouse.

	-serialrx [file]
		Specify a file (i.e. a FIFO) as the RX line for serial port B.

	-serialtx [file]
		Specify a file (i.e. a FIFO) as the TX line for serial port B.