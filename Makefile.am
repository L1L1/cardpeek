AUTOMAKE_OPTIONS = gnu subdir-objects

AM_CFLAGS=-Wall -pedantic -Wno-overlength-strings @LUA_CFLAGS@ @PCSC_CFLAGS@ @GLIB_CFLAGS@ @GTK_CFLAGS@ @CURL_CFLAGS@ 

bin_PROGRAMS= cardpeek

cardpeek_SOURCES = asn1.c asn1.h \
				   bytestring.c bytestring.h \
				   ui/gtk/gui_flexi_cell_renderer.c \
				   ui/gtk/gui_flexi_cell_renderer.h \
				   dyntree_model.c dyntree_model.h \
				   crypto.c crypto.h \
				   replay.c replay.h \
				   ui.c ui.h \
				   iso7816.c iso7816.h \
				   lua_nodes.c lua_nodes.h \
				   lua_asn1.c lua_asn1.h \
				   lua_bit.c lua_bit.h \
				   lua_bytes.c lua_bytes.h \
				   lua_card.c lua_card.h \
				   lua_crypto.c lua_crypto.h \
				   lua_ext.c lua_ext.h \
				   lua_log.c lua_log.h \
				   lua_ui.c lua_ui.h \
				   main.c \
				   misc.c misc.h \
				   pathconfig.c pathconfig.h \
				   smartcard.c smartcard.h \
				   system_info.c system_info.h \
				   a_string.c a_string.h \
				   ui/gtk/gui_core.c ui/gtk/gui_core.h \
				   ui/gtk/gui_about.c ui/gtk/gui_about.h \
				   ui/gtk/gui_toolbar.c ui/gtk/gui_toolbar.h \
				   ui/gtk/gui_logview.c ui/gtk/gui_logview.h \
				   ui/gtk/gui_cardview.c ui/gtk/gui_cardview.h \
				   ui/gtk/gui_readerview.c ui/gtk/gui_readerview.h \
				   ui/gtk/gui_scratchpad.c ui/gtk/gui_scratchpad.h \
				   ui/gtk/gui_inprogress.c ui/gtk/gui_inprogress.h \
				   ui/console/console_core.c ui/console/console_core.h \
				   ui/console/console_inprogress.c ui/console/console_inprogress.h \
				   ui/console/console_about.c ui/console/console_about.h \
				   http_download.c http_download.h \
				   lua_iconv.c lua_iconv.h \
				   cardpeek_update.c cardpeek_update.h \
				   cardpeek_public_key.h \
				   win32/config.h 

doc_DATA=doc/cardpeek_ref.en.pdf 

cardpeek_LDADD=cardpeek_resources.$(OBJEXT) @LUA_LIBS@ @PCSC_LIBS@ @GLIB_LIBS@ @GTK_LIBS@ @CURL_LIBS@ @ICONV_LIBS@

cardpeek_LDFLAGS=-g
#cardpeek_LDFLAGS=-O2

dist_man_MANS=cardpeek.1

DRIVERS=drivers/replay_driver.c  drivers/null_driver.c  drivers/pcsc_driver.c 
ICONS=icons/cardpeek-analyzer.png icons/cardpeek-item.png icons/cardpeek-record.png icons/cardpeek-smartcard.png icons/cardpeek-folder.png icons/cardpeek-logo.png icons/cardpeek-application.png icons/cardpeek-file.png icons/cardpeek-block.png icons/cardpeek-atr.png icons/cardpeek-header.png icons/cardpeek-body.png

CLEANFILES=cardpeek_resources.c dot_cardpeek.tar.gz 

EXTRA_DIST=cardpeek_resources.gresource.xml cardpeek_resources.gresource $(DRIVERS) $(ICONS) dot_cardpeek_dir doc/cardpeek_ref.en.pdf doc/cardpeek_ref.en.odt update_dot_cardpeek_dir.sh script_version.h cardpeek.desktop

GLIB_COMPILE_RESOURCES=@GLIB_COMPILE_RESOURCES@

desktopdir = $(datadir)/applications
desktop_DATA = cardpeek.desktop

iconsdir = $(datadir)/icons/hicolor/48x48/apps
icons_DATA = icons/cardpeek-logo.png


mostlyclean-local:
			rm -rf .cardpeek

dist-hook:
			rm -rf `find $(distdir)/ -name .svn`

dot_cardpeek.tar.gz:    dot_cardpeek_dir	
			@echo "  TAR      $<"
			$(AM_V_at)tar czf dot_cardpeek.tar.gz --directory $(srcdir)/dot_cardpeek_dir --exclude=.svn .

cardpeek_resources.$(OBJEXT):	dot_cardpeek.tar.gz $(ICONS) AUTHORS COPYING cardpeek_resources.gresource.xml
			@echo "  GLIB_COMPILE_RESOURCES cardpeek_resources.gresource.xml"
			$(AM_V_at)$(GLIB_COMPILE_RESOURCES) --generate-source --sourcedir=. --sourcedir=$(srcdir) --target=cardpeek_resources.c $(srcdir)/cardpeek_resources.gresource.xml
			$(AM_V_CC)$(COMPILE) -c cardpeek_resources.c

smartcard.$(OBJEXT):	smartcard.c smartcard.h $(DRIVERS)
			$(AM_V_CC)$(COMPILE) -c $<

