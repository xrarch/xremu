# XR/EMU (C+SDL)

Emulates the XR/station fantasy computer, inspired by late 80s RISC workstations.

The long-term goal is to create a really neat (but useless) emulated desktop computer.

For easy testing and experimentation, you can check out the [web emulator](https://xrarch.github.io).

Ships with a pre-built [boot ROM](https://github.com/xrarch/a4x) binary.

<img src="https://raw.githubusercontent.com/xrarch/xremu/master/17032.png" width="256">

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

    -cpus [count]
        Specify how many XR/17032 processors to simulate. Default is 1.

    -asyncdisk
        Simulate disk seek times.

    -asyncserial
        Simulate serial latency.

    -nocachesim
        Don't simulate the I-cache and D-cache.

    -cachemiss
        Simulate cycle delays on cache misses.

    -cacheprint
        Print cache statistics every 2 seconds. Only works if the emulator was compiled with PROFCPU=1 (which may slow down CPU emulation a bit).

    -diskprint
        Print disk accesses.

    -dumpram
        Dump the contents of RAM to a file called bank0.bin upon exit.

    -headless
        Don't attach framebuffer, keyboard, or mouse.

    -132column
        Use 132-column mode in the serial TTYs.

    -serialrx [file]
        Specify a file (i.e. a FIFO) as the RX line for serial port B.

    -serialtx [file]
        Specify a file (i.e. a FIFO) as the TX line for serial port B.

    -cpuhz [frequency]
        Specify the frequency that the CPU simulation should be run at. Default is 20000000 (20MHz).

WARNING: For performance reasons, this emulator does not make any attempt to be portable to big-endian host platforms! If you are on PowerPC for some reason, it will not run correctly!