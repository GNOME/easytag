SUBDIRS = libmpg123 libapetag id3lib

bin_PROGRAMS = easytag

localedir = $(datadir)/locale
easytag_CPPFLAGS = \
	-I$(top_builddir) \
	$(GTK_CFLAGS) \
	$(OGG_CFLAGS) \
	$(SPEEX_CFLAGS) \
	$(FLAC_CFLAGS) \
	$(WAVPACK_CFLAGS) \
	$(ID3TAG_CFLAGS) \
	$(WARN_CPPFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"

easytag_SOURCES = \
	about.c \
	ape_tag.c \
	bar.c \
	base64.c \
	browser.c \
	browser.h \
	cddb.c \
	charset.c \
	crc32.c \
	dlm.c \
	easytag.c \
	et_core.c \
	flac_header.c \
	flac_tag.c \
	id3_tag.c \
	id3v24_tag.c \
	log.c \
	misc.c \
	monkeyaudio_header.c \
	mpeg_header.c \
	mp4_header.c \
	mp4_tag.c \
	musepack_header.c \
	msgbox.c \
	ogg_header.c \
	ogg_tag.c \
	picture.c \
	prefs.c \
	scan.c \
	setting.c \
	vcedit.c \
	wavpack_header.c \
	wavpack_tag.c

easytag_headers = \
	about.h \
	ape_tag.h \
	bar.h \
	base64.h \
	cddb.h \
	charset.h \
	crc32.h \
	dlm.h \
	easytag.h \
	et_core.h \
	flac_header.h \
	flac_tag.h \
	genres.h \
	id3_tag.h \
	log.h \
	misc.h \
	monkeyaudio_header.h \
	mpeg_header.h \
	mp4_header.h \
	mp4_tag.h \
	musepack_header.h \
	msgbox.h \
	ogg_header.h \
	ogg_tag.h \
	picture.h \
	prefs.h \
	scan.h \
	setting.h \
	ui_manager.h \
	vcedit.h \
	wavpack_header.h \
	wavpack_tag.h

dist_noinst_DATA = \
	$(easytag_headers) \
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

easytag_LDADD = \
	$(GTK_LIBS) \
	$(OGG_LIBS) \
	$(SPEEX_LIBS) \
	$(FLAC_LIBS) \
	$(WAVPACK_LIBS) \
	$(LIBMP4V2_LIBS) \
	$(ID3TAG_LIBS) \
	$(ID3LIB_LIBS) \
	libmpg123/libmpg123.a \
	libapetag/libapetag.a \
	id3lib/libid3bugfix.a
