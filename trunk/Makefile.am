AUTOMAKE_OPTIONS = gnu

AM_CFLAGS=-Wall -pedantic -Wno-overlength-strings @LUA_CFLAGS@ @PCSC_CFLAGS@ @GLIB_CFLAGS@ @GTK_CFLAGS@ @CURL_CFLAGS@

#AM_LDFLAGS=@LUA_LIBS@ @PCSC_LIBS@ @GTK_LIBS@

bin_PROGRAMS= cardpeek

cardpeek_SOURCES = asn1.c asn1.h bytestring.c bytestring.h gui_flexi_cell_renderer.c gui_flexi_cell_renderer.h dyntree_model.c dyntree_model.h crypto.c crypto.h replay.c replay.h gui.c gui.h iso7816.c iso7816.h lua_nodes.c lua_nodes.h lua_asn1.c lua_asn1.h lua_bit.c lua_bit.h lua_bytes.c lua_bytes.h lua_card.c lua_card.h lua_crypto.c lua_crypto.h lua_ext.c lua_ext.h lua_log.c lua_log.h lua_ui.c lua_ui.h main.c misc.c misc.h pathconfig.c pathconfig.h smartcard.c smartcard.h system_info.c system_info.h a_string.c a_string.h gui_about.c gui_about.h gui_toolbar.c gui_toolbar.h gui_logview.c gui_logview.h gui_cardview.c gui_cardview.h gui_readerview.c gui_readerview.h gui_scratchpad.c gui_scratchpad.h gui_inprogress.c gui_inprogress.h

doc_DATA=doc/cardpeek_ref.en.pdf 

cardpeek_LDADD=cardpeek_resources.$(OBJEXT) @LUA_LIBS@ @PCSC_LIBS@ @GLIB_LIBS@ @GTK_LIBS@ @CURL_LIBS@

cardpeek_LDFLAGS=-O2

dist_man_MANS=cardpeek.1

DRIVERS=drivers/replay_driver.c  drivers/null_driver.c  drivers/pcsc_driver.c 
ICONS=icons/analyzer.png icons/item.png icons/record.png icons/smartcard.png icons/folder.png icons/cardpeek.png icons/application.png icons/file.png icons/block.png icons/atr.png icons/header.png icons/body.png

CLEANFILES=cardpeek_resources.c dot_cardpeek.tar.gz 

EXTRA_DIST=cardpeek_resources.gresource.xml cardpeek_resources.gresource $(DRIVERS) $(ICONS) dot_cardpeek_dir doc/cardpeek_ref.en.pdf doc/cardpeek_ref.en.odt update_dot_cardpeek_dir.sh script_version.h cardpeek.desktop

GLIB_COMPILE_RESOURCES=@GLIB_COMPILE_RESOURCES@

desktopdir = $(datadir)/applications
desktop_DATA = cardpeek.desktop

iconsdir = $(datadir)/icons/hicolor/48x48/apps
icons_DATA = icons/cardpeek.png


mostlyclean-local:
			rm -rf .cardpeek

dist-hook:
			rm -rf `find $(distdir)/ -name .svn`

dot_cardpeek.tar.gz:    dot_cardpeek_dir	
			@echo "  TAR      $<"
			@cp -R $(srcdir)/dot_cardpeek_dir .cardpeek
			@chmod -R u+w .cardpeek/
			@tar czf dot_cardpeek.tar.gz --exclude=.svn .cardpeek

cardpeek_resources.$(OBJEXT):	dot_cardpeek.tar.gz $(ICONS) AUTHORS COPYING cardpeek_resources.gresource.xml
			@echo "  GLIB_COMPILE_RESOURCES cardpeek_resources.gresource.xml"
			@$(GLIB_COMPILE_RESOURCES) --generate-source --sourcedir=. --sourcedir=$(srcdir) --target=cardpeek_resources.c $(srcdir)/cardpeek_resources.gresource.xml
			$(AM_V_CC)$(COMPILE) -c cardpeek_resources.c

smartcard.$(OBJEXT):	smartcard.c smartcard.h $(DRIVERS)
			$(AM_V_CC)$(COMPILE) -c $<

