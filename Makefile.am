ACLOCAL_AMFLAGS = -I m4
 
SUBDIRS = po

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
	src/id3lib/c_wrapper.cpp \
	src/libapetag/apetaglib.c \
	src/libapetag/is_tag.c \
	src/libapetag/info_mac.c \
	src/libapetag/info_mpc.c \
	src/libmpg123/common.c \
	src/libmpg123/dxhead.c \
	src/libmpg123/getbits.c \
	src/libmpg123/layer1.c \
	src/libmpg123/layer2.c \
	src/libmpg123/layer3.c \
	src/libmpg123/mpg123.c \
	src/about.c \
	src/ape_tag.c \
	src/bar.c \
	src/base64.c \
	src/browser.c \
	src/browser.h \
	src/cddb.c \
	src/charset.c \
	src/crc32.c \
	src/dlm.c \
	src/easytag.c \
	src/et_core.c \
	src/flac_header.c \
	src/flac_tag.c \
	src/id3_tag.c \
	src/id3v24_tag.c \
	src/log.c \
	src/misc.c \
	src/monkeyaudio_header.c \
	src/mpeg_header.c \
	src/mp4_header.c \
	src/mp4_tag.c \
	src/musepack_header.c \
	src/msgbox.c \
	src/ogg_header.c \
	src/ogg_tag.c \
	src/picture.c \
	src/prefs.c \
	src/scan.c \
	src/setting.c \
	src/vcedit.c \
	src/wavpack_header.c \
	src/wavpack_tag.c

easytag_headers = \
	src/id3lib/id3_bugfix.h \
	src/libapetag/apetaglib.h \
	src/libapetag/is_tag.h \
	src/libapetag/info_mac.h \
	src/libapetag/info_mpc.h \
	src/libmpg123/dxhead.h \
	src/libmpg123/getbits.h \
	src/libmpg123/huffman.h \
	src/libmpg123/l2tables.h \
	src/libmpg123/mpg123.h \
	src/about.h \
	src/ape_tag.h \
	src/bar.h \
	src/base64.h \
	src/cddb.h \
	src/charset.h \
	src/crc32.h \
	src/dlm.h \
	src/easytag.h \
	src/et_core.h \
	src/flac_header.h \
	src/flac_tag.h \
	src/genres.h \
	src/id3_tag.h \
	src/log.h \
	src/misc.h \
	src/monkeyaudio_header.h \
	src/mpeg_header.h \
	src/mp4_header.h \
	src/mp4_tag.h \
	src/musepack_header.h \
	src/msgbox.h \
	src/ogg_header.h \
	src/ogg_tag.h \
	src/picture.h \
	src/prefs.h \
	src/scan.h \
	src/setting.h \
	src/ui_manager.h \
	src/vcedit.h \
	src/wavpack_header.h \
	src/wavpack_tag.h

easytag_LDADD = \
	$(GTK_LIBS) \
	$(OGG_LIBS) \
	$(SPEEX_LIBS) \
	$(FLAC_LIBS) \
	$(WAVPACK_LIBS) \
	$(LIBMP4V2_LIBS) \
	$(ID3TAG_LIBS) \
	$(ID3LIB_LIBS)

desktopdir = $(datadir)/applications
dist_desktop_DATA = easytag.desktop

textdir = $(pkgdatadir)
dist_text_DATA = ChangeLog

man_MANS = doc/easytag.1

pixmaps = \
	pixmaps/add_folder.xpm \
	pixmaps/album.xpm \
	pixmaps/all_downcase.xpm \
	pixmaps/all_uppercase.xpm \
	pixmaps/artist.xpm \
	pixmaps/artist_album.xpm \
	pixmaps/black.xpm \
	pixmaps/blackwhite.xpm \
	pixmaps/closed_folder_readonly.xpm \
	pixmaps/closed_folder_unreadable.xpm \
	pixmaps/closed_folder.xpm \
	pixmaps/EasyTAG.ico \
	pixmaps/EasyTAG_icon.xpm \
	pixmaps/EasyTAG2.ico \
	pixmaps/EasyTAG2.xpm \
	pixmaps/first_letter_uppercase.xpm \
	pixmaps/first_letter_uppercase_word.xpm \
	pixmaps/forbidden.xpm \
	pixmaps/freedb.xpm \
	pixmaps/gnudb.xpm \
	pixmaps/grab.xpm \
	pixmaps/invert_selection.xpm \
	pixmaps/mask.xpm \
	pixmaps/musicbrainz.xpm \
	pixmaps/opened_folder.xpm \
	pixmaps/parent_folder.xpm \
	pixmaps/ram_disk.xpm \
	pixmaps/read_only.xpm \
	pixmaps/red_lines.xpm \
	pixmaps/scan.xpm \
	pixmaps/select_all.xpm \
	pixmaps/sequence_track.xpm \
	pixmaps/sound.xpm \
	pixmaps/unselect_all.xpm

pixmapdir = $(pkgdatadir)
dist_pixmap_DATA = \
	pixmaps/EasyTAG_logo.xpm

icondir = $(datadir)/pixmaps
dist_icon_DATA = \
	pixmaps/EasyTAG_icon.xpm


dist_noinst_DATA = \
	$(easytag_headers) \
	Makefile.mingw \
	src/id3lib/patch_id3lib_3.8.3_UTF16_writing_bug.diff \
	src/libapetag/COPYING.LGPL \
	src/libapetag/README.apetag \
	src/libapetag/id3v2_read.c \
	src/libapetag/id3v2_read.h \
	src/libmpg123/README \
	src/win32/easytag.rc \
	src/win32/gtkwin32dep.c \
	src/win32/gtkwin32dep.h \
	src/win32/resource.h \
	src/win32/win32dep.c \
	src/win32/win32dep.h \
	src/win32/win_easytag.c \
	src/win32/nsis/easytag-header.bmp \
	src/win32/nsis/easytag-install.bmp \
	src/win32/nsis/easytag-install.ico \
	src/win32/nsis/easytag-uninstall.bmp \
	$(pixmaps) \
	\
	easytag.spec \
	easytag-win32-installer.nsi \
	COPYING \
	README \
	THANKS \
	TODO \
	USERS-GUIDE \
	\
	config.h.mingw \
	Makefile.mingw \
	README_win32.txt \
	po/Makefile.mingw \
	\
	doc/easytag.1.in \
	doc/EasyTAG_Documentation*.html \
	doc/users_guide_*/* \
	doc/users_guide_*/images/* \
	doc/id3/* \
	doc/vorbis/* 

dist_noinst_SCRIPTS = \
    autogen.sh
