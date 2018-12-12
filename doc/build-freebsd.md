Specific installation instructions for FreeBSD
==============================================

If you get the following error, after using configure and make:
 
	invalid DSO for symbol `libiconv_open' definition

Try running the following commands:

	ICONV_LIBS="-l iconv" ./configure
	make
