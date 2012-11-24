noinst_LIBRARIES = libmpg123.a

AM_CPPFLAGS = \
	$(GTK_CFLAGS) \
	$(WARN_CPPFLAGS) \
	-DLOCALE=\"$(localedir)\"

libmpg123_a_SOURCES = \
	common.c \
	dxhead.h \
	dxhead.c \
	getbits.c \
	getbits.h \
	huffman.h \
	l2tables.h \
	layer1.c \
	layer2.c \
	layer3.c \
	mpg123.c \
	mpg123.h


dist_noinst_DATA = \
	README
