#!/bin/sh

export DESTINATION="$HOME/Desktop/"

echo "This script will install the app in $DESTINATION"

iconutil -c icns cardpeek.iconset

gtk-mac-bundler cardpeek.bundle


