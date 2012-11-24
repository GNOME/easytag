noinst_LIBRARIES = libapetag.a

libapetag_a_CPPFLAGS = \
	$(GTK_CFLAGS) \
	$(WARN_CPPFLAGS) \
	-DLOCALE=\"$(localedir)\"

libapetag_a_SOURCES = \
    apetaglib.c \
    apetaglib.h \
    is_tag.c \
    is_tag.h \
    info_mac.c \
    info_mac.h \
    info_mpc.c \
    info_mpc.h

dist_noinst_DATA = \
    COPYING.LGPL \
    README.apetag \
    id3v2_read.c \
    id3v2_read.h
