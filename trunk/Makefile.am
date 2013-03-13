AUTOMAKE_OPTIONS = gnu

AM_CFLAGS=-Wall -pedantic -Wno-overlength-strings @LUA_CFLAGS@ @PCSC_CFLAGS@ @GTK_CFLAGS@

#AM_LDFLAGS=@LUA_LIBS@ @PCSC_LIBS@ @GTK_LIBS@

bin_PROGRAMS= cardpeek

cardpeek_SOURCES = asn1.c asn1.h attributes.c attributes.h bytestring.c bytestring.h gui_flexi_cell_renderer.c gui_flexi_cell_renderer.h dyntree_model.c dyntree_model.h cardtree.c cardtree.h crypto.c crypto.h emulator.c emulator.h gui.c gui.h iso7816.c iso7816.h lua_asn1.c lua_asn1.h lua_bit.c lua_bit.h lua_bytes.c lua_bytes.h lua_card.c lua_card.h lua_crypto.c lua_crypto.h lua_ext.c lua_ext.h lua_log.c lua_log.h lua_ui.c lua_ui.h main.c misc.c misc.h pathconfig.c pathconfig.h smartcard.c smartcard.h 

doc_DATA=doc/cardpeek_ref.*.pdf 

cardpeek_LDADD=dot_cardpeek.$(OBJEXT) @LUA_LIBS@ @PCSC_LIBS@ @GTK_LIBS@

dist_man_MANS=cardpeek.1

DRIVERS=drivers/emul_driver.c  drivers/null_driver.c  drivers/pcsc_driver.c drivers/acg_driver.c

CLEANFILES=icons.c dot_cardpeek.tar.gz

EXTRA_DIST=script.S $(DRIVERS) icons/*.png dot_cardpeek_dir doc/*.pdf update_dot_cardpeek_dir.sh script_version.h cardpeek.desktop

GDK_PIXBUF_CSOURCE=@GDK_PIXBUF_CSOURCE@

desktopdir = $(datadir)/applications
desktop_DATA = cardpeek.desktop

iconsdir = $(datadir)/icons/hicolor/48x48/apps
icons_DATA = icons/cardpeek.png


dist-hook:
			rm -rf `find $(distdir)/ -name .svn`

dot_cardpeek.$(OBJEXT):	dot_cardpeek_dir
			@rm -rf .cardpeek
			@cp -R dot_cardpeek_dir .cardpeek
			tar czf dot_cardpeek.tar.gz --exclude=.svn .cardpeek
			@rm -rf .cardpeek
			$(COMPILE) -c script.S -o $@

icons.c:		icons/
			@echo "Storing icons in icon.c"
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_item        icons/item.png > icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_card        icons/drive.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_application icons/application.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_file        icons/document.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_record      icons/card.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_block       icons/block.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_smartcard   icons/smartcard.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_cardpeek    icons/cardpeek.png >> icons.c
			touch gui.c

gui.$(OBJEXT):		gui.c gui.h icons.c
			$(COMPILE) -Wno-write-strings -c $<

smartcard.$(OBJEXT):	smartcard.c smartcard.h $(DRIVERS)
			$(COMPILE) -c $<

