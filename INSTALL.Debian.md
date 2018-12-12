Build instructions for Debian 9 (Stretch)
=========================================

**!!! Produced binaries do not run yet - See [issue #1](https://github.com/ipamo/cardpeek/issues/1) !!!**

## For local Debian host

Required libraries:

    sudo apt install autoconf make libglib2.0-dev libgtk-3-dev liblua5.2-dev libcurl4-openssl-dev libssl-dev

Build:

    autoreconf --install
    ./configure
    make

Install (use sudo if necessary):

    make install


## Cross-compile for Windows (32 bits)

Requirements:

    sudo apt install libpcsclite1:i386 dirmngr mingw-w64 mingw-w64-tools binutils-mingw-w64

Download mingw-W64 package dependencies either from a Windows MSys2 installation, or directly from [MSys2 repository](http://repo.msys2.org/mingw/i686/),
unpack the packages and merge them into a new directory called `deps/win32` (which will then contain subdirectories `bin`, `lib`, `include`, etc):

    ./deps-win32.sh

Build:

    make -f Makefile.win32 CCPRE=i686-w64-mingw32- DEPS=deps/win32 TARGET=/opt/cardpeek-win32

Install:

    make install -f Makefile.win32 CCPRE=i686-w64-mingw32- DEPS=deps/win32 TARGET=/opt/cardpeek-win32

Run:

    wine build/win32/cardpeek.exe


## Cross-compile for Windows (64 bits)

Adapt previous commands as follow

    ./clean.sh
    PLATFORM=win64 ./deps-win32.sh
    make -f Makefile.win32 CCPRE=x86_64-w64-mingw32- DEPS=deps/win64 MPLATFORM=mingw64 TARGET=/opt/cardpeek-win64
    make install -f Makefile.win32 CCPRE=x86_64-w64-mingw32- DEPS=deps/win64 MPLATFORM=mingw64 TARGET=/opt/cardpeek-win64
