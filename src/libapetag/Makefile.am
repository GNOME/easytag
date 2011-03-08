CFLAGS = @CFLAGS@ @GTK_CFLAGS@

noinst_LIBRARIES = libapetag.a

INCLUDES = -DLOCALE=\"$(localedir)\"


libapetag_a_SOURCES = \
    apetaglib.c \
    apetaglib.h \
    is_tag.c \
    is_tag.h \
    info_mac.c \
    info_mac.h \
    info_mpc.c \
    info_mpc.h

EXTRA_DIST = \
    COPYING.LGPL \
    README.apetag \
    id3v2_read.c \
    id3v2_read.h
