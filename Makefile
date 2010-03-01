CC=gcc

# GENERAL DEFS

CFLAGS_Linux=-Wall -pedantic -c -Wno-overlength-strings \
	`pkg-config --exists lua5.1 && pkg-config lua5.1 --cflags || pkg-config lua --cflags` \
	`pkg-config libpcsclite gtk+-2.0 openssl --cflags` 

LFLAGS_Linux=-Wall `pkg-config --exists lua5.1 && pkg-config lua5.1 --libs || pkg-config lua --libs`\
	`pkg-config libpcsclite gtk+-2.0 openssl --libs`

CFLAGS_FreeBSD=-Wall -pedantic -c -Wno-overlength-strings \
	`pkg-config --exists lua-5.1 && pkg-config lua-5.1 --cflags || pkg-config lua --cflags` \
	`pkg-config libpcsclite gtk+-2.0 openssl --cflags`
 
LFLAGS_FreeBSD=-Wall `pkg-config --exists lua-5.1 && pkg-config lua-5.1 --libs || pkg-config lua --libs`\
	`pkg-config libpcsclite gtk+-2.0 openssl --libs`

# SPECIFIC DEFS
#   Replace 'Linux' by 'FreeBSD' in both definitions
#   below if you are using FreeBSD.

CFLAGS=$(CFLAGS_Linux)

LFLAGS=$(LFLAGS_Linux)

OBJECTS=bytestring.o asn1.o misc.o config.o smartcard.o cardtree.o gui.o \
	iso7816.o lua_ext.o main.o dot_cardpeek.o emulator.o crypto.o \
        attributes.o lua_asn1.o lua_bit.o lua_bytes.o lua_crypto.o lua_ui.o \
        lua_log.o lua_card.o

DRIVERS=drivers/emul_driver.c  drivers/null_driver.c  drivers/pcsc_driver.c drivers/acg_driver.c

all:		cardpeek

cardpeek:	icons.c $(OBJECTS)
		$(CC) $(LFLAGS) $(OBJECTS) -o cardpeek

icons.c:	icons/
		gdk-pixbuf-csource --raw --name icon_item        icons/item.png > icons.c
		gdk-pixbuf-csource --raw --name icon_card        icons/drive.png >> icons.c
		gdk-pixbuf-csource --raw --name icon_application icons/application.png >> icons.c
		gdk-pixbuf-csource --raw --name icon_file        icons/document.png >> icons.c
		gdk-pixbuf-csource --raw --name icon_record      icons/card.png >> icons.c
		gdk-pixbuf-csource --raw --name icon_block       icons/block.png >> icons.c
		gdk-pixbuf-csource --raw --name icon_smartcard   icons/smartcard.png >> icons.c
		touch gui.c

%.o:		%.c %.h
		$(CC) $(CFLAGS) $<

gui.o:		gui.c gui.h icons.c
		$(CC) $(CFLAGS) -Wno-write-strings $<

smartcard.o:	smartcard.c smartcard.h $(DRIVERS)
		$(CC) $(CFLAGS) $<

lua_ext.o:	lua_ext.c lua_ext.h lua_asn1.c  lua_bit.c  lua_bytes.c lua_card.c lua_log.c lua_ui.c lua_crypto.c
		$(CC) $(CFLAGS) -Wno-write-strings $<

dot_cardpeek.o:	dot_cardpeek_dir
		cp -R dot_cardpeek_dir .cardpeek
		tar cvzf dot_cardpeek.tar.gz --exclude=.svn .cardpeek
		rm -rf .cardpeek
		$(CC) $(CFLAGS) script.S -o $@
		rm -f dot_cardpeek.tar.gz

docs:
		cd doc; make

docs-clean:
		cd doc; make clean

package:	clean
		cd ..; tar zcvf cardpeek-`date +%Y%m%d`.tgz --exclude-vcs cardpeek/

clean:		docs-clean
		rm -f cardpeek *.o icons.c

