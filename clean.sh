#!/bin/bash
[ -f Makefile ] && make clean
make -f Makefile.win32 clean
for p in $(find -type f -name "*.o"); do
    rm "$p"
done
for p in $(find -type f -name "*.dirstamp"); do
    rm "$p"
done
for p in $(find -type d -name "*.deps"); do
    rm -r "$p"
done
rm -f Makefile
rm -f Makefile.in
rm -f aclocal.m4
rm -f cardpeek
rm -f cardpeek.exe
rm -f cardpeek_resources.c
rm -f compile
rm -f config.guess
rm -f config.h
rm -f config.log
rm -f config.status
rm -f config.sub
rm -f configure
rm -f depcomp
rm -f install-sh
rm -f missing
rm -f stamp-h1
rm -f dot_cardpeek.tar.gz
rm -f win32/libwinscard.a
rm -rf autom4te.cache
