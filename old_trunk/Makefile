OBJECTS =  smartcard.o misc.o bytestring.o asn1.o cardtree.o gui.o config.o lua_ext.o \
	   main.o dot_cardpeek.o
CC = gcc
CFLAGS = -Wall -pedantic -I /usr/include/PCSC `pkg-config lua5.1 --cflags` `pkg-config gtk+-2.0 --cflags` -c
LFLAGS = -Wall -l pcsclite -L/usr/include/lua5.1 `pkg-config lua5.1 --libs` `pkg-config gtk+-2.0 --libs`

all:			$(OBJECTS)
			$(CC) $(LFLAGS) $(OBJECTS) -o cardpeek

%.o:			%.c %.h
			$(CC) $(CFLAGS) $<

gui.o:			gui.c gui.h
			$(CC) $(CFLAGS) -Wno-write-strings $<

dot_cardpeek.o:		dot_cardpeek_dir
			cp -R dot_cardpeek_dir .cardpeek
			tar cvzf dot_cardpeek.tar.gz .cardpeek
			rm -rf .cardpeek
			objcopy -v -B i386 -I binary -O elf32-i386 dot_cardpeek.tar.gz dot_cardpeek.o
			rm -f dot_cardpeek.tar.gz

clean:
			rm -f *.o cardpeek dot_cardpeek.tar.gz
