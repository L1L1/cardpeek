AUTOMAKE_OPTIONS = gnu

AM_CFLAGS=-Wall -pedantic -Wno-overlength-strings @LUA_CFLAGS@ @PCSC_CFLAGS@ @GLIB_CFLAGS@ @GTK_CFLAGS@

#AM_LDFLAGS=@LUA_LIBS@ @PCSC_LIBS@ @GTK_LIBS@

bin_PROGRAMS= cardpeek

cardpeek_SOURCES = asn1.c asn1.h bytestring.c bytestring.h gui_flexi_cell_renderer.c gui_flexi_cell_renderer.h dyntree_model.c dyntree_model.h cardtree.c cardtree.h crypto.c crypto.h emulator.c emulator.h gui.c gui.h iso7816.c iso7816.h lua_asn1.c lua_asn1.h lua_bit.c lua_bit.h lua_bytes.c lua_bytes.h lua_card.c lua_card.h lua_crypto.c lua_crypto.h lua_ext.c lua_ext.h lua_log.c lua_log.h lua_ui.c lua_ui.h main.c misc.c misc.h pathconfig.c pathconfig.h smartcard.c smartcard.h system_info.c system_info.h a_string.c a_string.h gui_about.c gui_about.h 

doc_DATA=doc/cardpeek_ref.en.pdf 

cardpeek_LDADD=dot_cardpeek.$(OBJEXT) @LUA_LIBS@ @PCSC_LIBS@ @GLIB_LIBS@ @GTK_LIBS@

dist_man_MANS=cardpeek.1

DRIVERS=drivers/emul_driver.c  drivers/null_driver.c  drivers/pcsc_driver.c drivers/acg_driver.c
ICONS=icons/analyzer.png icons/item.png icons/card.png icons/smartcard.png icons/folder.png icons/cardpeek.png icons/application.png icons/document.png icons/block.png

CLEANFILES=icons.c dot_cardpeek.tar.gz 

EXTRA_DIST=script.S $(DRIVERS) $(ICONS) dot_cardpeek_dir doc/cardpeek_ref.en.pdf doc/cardpeek_ref.en.odt update_dot_cardpeek_dir.sh script_version.h cardpeek.desktop

GDK_PIXBUF_CSOURCE=@GDK_PIXBUF_CSOURCE@

desktopdir = $(datadir)/applications
desktop_DATA = cardpeek.desktop

iconsdir = $(datadir)/icons/hicolor/48x48/apps
icons_DATA = icons/cardpeek.png


mostlyclean-local:
			rm -rf .cardpeek

dist-hook:
			rm -rf `find $(distdir)/ -name .svn`

dot_cardpeek.$(OBJEXT):	dot_cardpeek_dir
			cp -R $(srcdir)/dot_cardpeek_dir .cardpeek
			chmod -R u+w .cardpeek/
			tar czf dot_cardpeek.tar.gz --exclude=.svn .cardpeek
			$(COMPILE) -c $(srcdir)/script.S -o $@

icons.c:		$(ICONS)
			@echo "Storing icons in icon.c"
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_item        $(srcdir)/icons/item.png > icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_application $(srcdir)/icons/application.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_file        $(srcdir)/icons/document.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_record      $(srcdir)/icons/card.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_block       $(srcdir)/icons/block.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_smartcard   $(srcdir)/icons/smartcard.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_analyzer    $(srcdir)/icons/analyzer.png >> icons.c
			@$(GDK_PIXBUF_CSOURCE) --raw --name icon_cardpeek    $(srcdir)/icons/cardpeek.png >> icons.c
			touch $(srcdir)/gui.c

gui.$(OBJEXT):		gui.c gui.h icons.c
			$(COMPILE) -Wno-write-strings -c $<

smartcard.$(OBJEXT):	smartcard.c smartcard.h $(DRIVERS)
			$(COMPILE) -c $<

