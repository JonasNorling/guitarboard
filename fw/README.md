This directory contains firmware for the Guitar Board.

== Contents

If you just type 'make' in this directory. it will attempt to build a number of
executables for both the embedded ARM target and for host (i.e. the computer
you are building on, which may well be an ARM too). The different executables
are test programs, examples, and a more complete "FX box" program, all built
around the same core framework. The host executables run much of the same code
as on the target board, but using the computers sound card for audio I/O.


== Dependencies and third party software

* libopencm3 <http://libopencm3.org/> - included as a git submodule.
* Jack - the audio streaming framework is used on the host.
* dfu-util - for programming the board with DFU over USB
* OpenOCD 0.9.0 or later - for programming the board with an ST-Link v2


== Building the firmware

The firmware can be built with a recent ARM gcc toolchain and GNU Make. I am
using the GCC ARM Embedded toolchain <https://launchpad.net/gcc-arm-embedded>
running on Linux with great success. The same toolchain is distributed for
other popular platforms as well, and using MacOS X for this kind of work seems
to work almost as well as Linux.

Do something like this:

```sh
make
make flash # Flash the default program onto the target using OpenOCD, or
make dfu # Flash the default program onto the target over USB (short BOOT0 to VCC)
```

== Notes

On Mac OS X, libopencm3 fails to generate the linker script properly. The gsub
lines in scripts/genlink.awk need some more escaping to work with the local awk.
