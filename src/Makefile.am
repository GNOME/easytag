CFLAGS = @CFLAGS@ @GTK_CFLAGS@

SUBDIRS = libmpg123 libapetag id3lib

bin_PROGRAMS = easytag

easytag_SOURCES = \
 about.c \
 about.h \
 ape_tag.c \
 ape_tag.h \
 bar.c \
 bar.h \
 base64.c \
 base64.h \
 browser.c \
 browser.h \
 cddb.c \
 cddb.h \
 charset.c \
 charset.h \
 crc32.c \
 crc32.h \
 dlm.c \
 dlm.h \
 easytag.c \
 easytag.h \
 et_core.c \
 et_core.h \
 flac_header.c \
 flac_header.h \
 flac_tag.c \
 flac_tag.h \
 genres.h \
 id3_tag.c \
 id3_tag.h \
 id3v24_tag.c \
 log.c \
 log.h \
 misc.c \
 misc.h \
 monkeyaudio_header.c \
 monkeyaudio_header.h \
 mpeg_header.c \
 mpeg_header.h \
 mp4_header.c \
 mp4_header.h \
 mp4_tag.c \
 mp4_tag.h \
 musepack_header.c \
 musepack_header.h \
 msgbox.c \
 msgbox.h \
 ogg_header.c \
 ogg_header.h \
 ogg_tag.c \
 ogg_tag.h \
 picture.c \
 picture.h \
 prefs.c \
 prefs.h \
 scan.c \
 scan.h \
 setting.c \
 setting.h \
 ui_manager.h \
 vcedit.c \
 vcedit.h \
 wavpack_header.c \
 wavpack_header.h \
 wavpack_tag.c \
 wavpack_tag.h

EXTRA_DIST = \
 Makefile.mingw \
 win32/easytag.rc \
 win32/gtkwin32dep.c \
 win32/gtkwin32dep.h \
 win32/resource.h \
 win32/win32dep.c \
 win32/win32dep.h \
 win32/win_easytag.c \
 win32/nsis/easytag-header.bmp \
 win32/nsis/easytag-install.bmp \
 win32/nsis/easytag-install.ico \
 win32/nsis/easytag-uninstall.bmp 

easytag_LDADD = $(GTK_LIBS) libmpg123/libmpg123.a libapetag/libapetag.a  id3lib/libid3bugfix.a

localedir = $(datadir)/locale
PACKAGE_DATA_DIR = $(datadir)/$(PACKAGE)
INCLUDES = -DLOCALEDIR=\"$(localedir)\" \
           -DPACKAGE_DATA_DIR=\"$(PACKAGE_DATA_DIR)\"
