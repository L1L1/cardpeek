#!/usr/bin/env bash
# Created @ 13.01.2015 by Christian Mayer <http://fox21.at>


# You also need to install DBus:
#   https://gist.github.com/TheFox/dc3f1b88757ba0a8a7a9
#1. Xcode 4.6.2 Command Line Tools.
#2. Homebrew (http://mxcl.github.io/homebrew/), used to install libgtk+, liblua and libssl.
#3. XQuartz (http://xquartz.macosforge.org/).

# Install 'gnome-icon-theme' for GTK+ 3.
brew install gnome-icon-theme
brew install openssl
brew link openssl --force
brew install glib
brew install gtk+3
brew install curl
brew install lua

# Check out the source.
pwd

# Create files for compiling.
autoreconf -fi

# './configure' may fail with
#   Package 'xyz', required by 'abc', not found
# So we need to set the path for PkgConfig.
export PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig
./configure

# Compile.
make

# Alternatively you can install
# cardpeek into your system.
#make install
