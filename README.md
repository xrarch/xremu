# XR/EMU (C+SDL)

Emulates the XR/station fantasy computer, inspired by late 80s workstation computers.

The long term goal is to have a sophisticated (and fully self-hosted) software environment within the emulated workstation computer.

For easy testing and experimentation, you can check out the [web emulator](https://xrarch.github.io).

Note, if you fear binary blobs, that the emulator ships with a pre-built [boot ROM](https://github.com/xrarch/a4x) binary.

<img src="https://raw.githubusercontent.com/xrarch/xremu/master/17032.png" width="256">

## Building

Make sure you have SDL2 installed.

A newish clang is required. Older versions have a code generation bug that breaks the emulator core. (Found this out the hard way).

Building the emulator should be as simple as typing `make`.

### Fast

If maximum performance is desired, rather than realism, `make FASTMEMORY=1` will compile an alternate memory subsystem into the emulator that is geared for performance. `-cpuhz` can then be used to crank the CPU speed up much higher than is normally possible (typically into the 300MHz+ range, and as high as 750MHz has been seen on some machines). Note that this eliminates Icache and Dcache simulation and shouldn't be used for system development purposes as cache invalidation bugs will then go undetected.

## Running

Type `./graphical.sh` in the project directory to see the boot ROM prompt. Review the options below to get it to do more interesting things.

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

    -threads [count]
        Specify how many host threads to spread simulated CPU load across. Default is half of CPU count.

    -asyncdisk
        Simulate disk seek times.

    -asyncserial
        Simulate serial latency.

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

WARNING: This emulator does not make any attempt to be portable to big-endian host platforms! If you are on PowerPC for some reason, it will not run correctly!