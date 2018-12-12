Build on Windows
================

**Produced binaries do not run yet - See issue ipamo/cardweek#1**

## Pre-requisites

Download and install [msys2](https://www.msys2.org/), which provide gcc toolchains for 32-bit and 64-bit Windows (both powered by MinGW-w64).

## Build for 32-bit Windows

Open a 32-bit mingw shell (usually `C:\msys64\mingw32.exe`).

Install dependencies:

    pacman -S tar make pkg-config mingw32/mingw-w64-i686-gcc mingw32/mingw-w64-i686-gtk3 mingw32/mingw-w64-i686-lua mingw32/mingw-w64-i686-curl

Clean:

    ./clean.sh

Build:

    make -f Makefile.win32
    make install -f Makefile.win32

## Build for 64-bit Windows

Open a 64-bit mingw shell (usually `C:\msys64\mingw64.exe`).

Install dependencies:

    pacman -S tar make pkg-config mingw64/mingw-w64-x86_64-gcc mingw64/mingw-w64-x86_64-gtk3 mingw64/mingw-w64-x86_64-lua mingw64/mingw-w64-x86_64-curl

Clean:

    ./clean.sh

Build:

    make -f Makefile.win32 DEPS=/mingw64 MPLATFORM=mingw64 TARGET="/C/Program Files/Cardpeek"
    make install -f Makefile.win32 DEPS=/mingw64 MPLATFORM=mingw64 TARGET="/C/Program Files/Cardpeek"
