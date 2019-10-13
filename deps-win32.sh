#!/bin/bash
#
# Download and extract dependencies for cross-compilation from Debian for Windows.
# See doc/build-debian.md
#
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

PLATFORM=${PLATFORM=-win32}
DEPS=deps/$PLATFORM
if [[ `uname -s` == MINGW* ]] || [[ `uname -s` == MSYS* ]]; then
    echo -e "${RED}This script should not be run from msys${NC}"
    exit 1
fi

function import_signing_key {
    #
    # Import Alexey Pavlov (maintener of mingw-w64 repository) signing key, if necessary
    #
    local keyid=AD351C50AE085775EB59333B5F92EFC1A47D45A1
    gpg --list-keys --keyid-format LONG | grep $keyid > /dev/null
    if [ $? -eq 0 ]; then
        return 0
    fi
    echo -e "${YELLOW}Import signing key${NC}"
    gpg --recv-key $keyid
}

function get_package {
    #
    # Download (if necessary), verify, and extract a package.
    # @param $1: Name of the package
    #
    local name=$1
    local mplatform=mingw32
    local iplatform=i686
    if [ "$PLATFORM" == "win64" ]; then
        mplatform=mingw64
        iplatform=x86_64
    fi

    if [[ $name == mingw-w64-cross-* ]]; then
        # special package to retrieve libgcc_s_dw2-1.dll
        local archive_src=http://repo.msys2.org/msys/i686/$name.tar.xz
        local archive_dst=$DEPS/archives/$name.tar.xz

        local sign_src=http://repo.msys2.org/msys/i686/$name.tar.xz.sig
        local sign_dst=$DEPS/archives/$name.tar.xz.sig
    else
        local archive_src=http://repo.msys2.org/mingw/$iplatform/mingw-w64-$iplatform-$name.pkg.tar.xz
        local archive_dst=$DEPS/archives/mingw-w64-$iplatform-$name.pkg.tar.xz

        local sign_src=http://repo.msys2.org/mingw/$iplatform/mingw-w64-$iplatform-$name.pkg.tar.xz.sig
        local sign_dst=$DEPS/archives/mingw-w64-$iplatform-$name.pkg.tar.xz.sig
    fi

    mkdir -p $DEPS/archives
    
    # Download
    if [ ! -f $archive_dst ]; then
        echo -e "${YELLOW}Download $name (package)${NC}"
        curl -o $archive_dst $archive_src
    fi
    if [ ! -f $sign_dst ]; then
        echo -e "${YELLOW}Download $name (sig)${NC}"
        curl -o $sign_dst $sign_src
    fi
    
    # Verify
    echo -e "${YELLOW}Check $name${NC}"
    gpg --verify $sign_dst $archive_dst
    if [ ! $? -eq 0 ]; then
        echo -e "${RED}Signature error for $name${NC}"
        exit 1
    fi

    # Extract
    echo -e "${YELLOW}Extract $name${NC}"

    if [[ $name == mingw-w64-cross-* ]]; then
        tar -C $DEPS -xf $archive_dst
    else
        tar -C $DEPS -xf $archive_dst $mplatform/ --strip-components=1
    fi

    if [ ! $? -eq 0 ]; then
        echo -e "${RED}Cannot extract $name${NC}"
        exit 1
    fi
}

packages=(
    atk-2.30.0-1-any
    brotli-1.0.7-1-any
    bzip2-1.0.6-6-any
    cairo-1.16.0-1-any
    curl-7.62.0-1-any
    expat-2.2.6-1-any
    fontconfig-2.13.1-1-any
    freetype-2.9.1-1-any
    fribidi-1.0.5-1-any
    gdk-pixbuf2-2.38.0-2-any
    gettext-0.19.8.1-7-any
    glib2-2.58.1-1-any
    graphite2-1.3.9-1-any
    gtk3-3.24.1-1-any
    harfbuzz-2.2.0-1-any
    libdatrie-0.2.12-1-any
    libepoxy-1.5.3-1-any
    libffi-3.2.1-4-any
    libiconv-1.15-3-any
    libidn2-2.0.5-1-any
    libpng-1.6.36-1-any
    libpsl-0.20.2-1-any
    libthai-0.1.28-2-any
    libtiff-4.0.10-1-any
    libunistring-0.9.10-1-any
    libwinpthread-git-7.0.0.5273.3e5acf5d-1-any
    lua-5.3.5-1-any
    ncurses-6.1.20180908-1-any
    nghttp2-1.35.0-1-any
    openssl-1.1.1-4-any
    pango-1.42.4-2-any
    pcre-8.42-1-any
    pixman-0.36.0-1-any
    readline-7.0.005-1-any
    termcap-1.3.1-3-any
    zlib-1.2.11-5-any
)

# ------ Get mingw packages ------
import_signing_key
for name in "${packages[@]}"; do
   get_package $name
done

# ----- Get local files ------
if [ "$PLATFORM" == "win32" ]; then
    # We're cross-compiling for 32-bit Windows from Linux
    get_package mingw-w64-cross-gcc-7.3.0-2-i686.pkg # necessary to get libgcc_s_dw2-1.dll
    cp -u $DEPS/opt/lib/gcc/i686-w64-mingw32/7.3.0/libgcc_s_dw2-1.dll $DEPS/bin/
    cp -u /usr/lib/gcc/i686-w64-mingw32/6.3-win32/libgcc_s_sjlj-1.dll $DEPS/bin/
    cp -u /usr/lib/gcc/i686-w64-mingw32/6.3-win32/libstdc++-6.dll $DEPS/bin/
else
    # We're cross-compiling for 64-bit Windows from Linux
    cp -u /usr/lib/gcc/x86_64-w64-mingw32/6.3-win32/libgcc_s_seh-1.dll $DEPS/bin/
    cp -u /usr/lib/gcc/x86_64-w64-mingw32/6.3-win32/libstdc++-6.dll $DEPS/bin/
fi
