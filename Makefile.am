ACLOCAL_AMFLAGS = -I m4
 
SUBDIRS = po

bin_PROGRAMS = easytag

localedir = $(datadir)/locale
easytag_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(DEPRECATED_CPPFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"
easytag_CFLAGS = \
	$(WARN_CFLAGS) \
	$(EASYTAG_CFLAGS)

if WINDOWS_HOST
win32_sources = \
	src/win32/win32dep.c
endif

easytag_SOURCES = \
	src/id3lib/c_wrapper.cpp \
	src/libapetag/apetaglib.c \
	src/libapetag/is_tag.c \
	src/libapetag/info_mac.c \
	src/libapetag/info_mpc.c \
	src/libmpg123/common.c \
	src/libmpg123/dxhead.c \
	src/libmpg123/layer1.c \
	src/libmpg123/layer2.c \
	src/libmpg123/layer3.c \
	src/libmpg123/mpg123.c \
	src/about.c \
	src/ape_tag.c \
	src/bar.c \
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
	src/gtk2_compat.c \
	src/id3_tag.c \
	src/id3v24_tag.c \
	src/log.c \
	src/misc.c \
	src/monkeyaudio_header.c \
	src/mpeg_header.c \
	src/mp4_header.c \
	src/mp4_tag.c \
	src/musepack_header.c \
	src/ogg_header.c \
	src/ogg_tag.c \
	src/picture.c \
	src/prefs.c \
	src/scan.c \
	src/setting.c \
	src/vcedit.c \
	src/wavpack_header.c \
	src/wavpack_tag.c \
	$(win32_sources)

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
	src/cddb.h \
	src/charset.h \
	src/crc32.h \
	src/dlm.h \
	src/easytag.h \
	src/et_core.h \
	src/flac_header.h \
	src/flac_tag.h \
	src/genres.h \
	src/gtk2_compat.h \
	src/id3_tag.h \
	src/log.h \
	src/misc.h \
	src/monkeyaudio_header.h \
	src/mpeg_header.h \
	src/mp4_header.h \
	src/mp4_tag.h \
	src/musepack_header.h \
	src/ogg_header.h \
	src/ogg_tag.h \
	src/picture.h \
	src/prefs.h \
	src/scan.h \
	src/setting.h \
	src/ui_manager.h \
	src/vcedit.h \
	src/wavpack_header.h \
	src/wavpack_tag.h \
	src/win32/win32dep.h

easytag_LDADD = \
	$(EASYTAG_LIBS) \
	$(ID3LIB_LIBS)

# NSIS installer script.
# TODO: Run script from dist dir.
easytag-win32-installer.nsi: easytag-win32-installer.nsi.in Makefile
	$(AM_V_GEN)$(SED) -e "s|\@PACKAGE_NAME\@|$(PACKAGE_NAME)|" \
		-e "s|\@PACKAGE_TARNAME\@|$(PACKAGE_TARNAME)|" \
		-e "s|\@PACKAGE_URL\@|$(PACKAGE_URL)|" \
		-e "s|\@PACKAGE_VERSION\@|$(PACKAGE_VERSION)|" $< > $@

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = easytag.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

textdir = $(pkgdatadir)
dist_text_DATA = ChangeLog

man_MANS = doc/easytag.1

pixmaps = \
	data/pixmaps/add_folder.xpm \
	data/pixmaps/album.xpm \
	data/pixmaps/all_downcase.xpm \
	data/pixmaps/all_uppercase.xpm \
	data/pixmaps/artist.xpm \
	data/pixmaps/artist_album.xpm \
	data/pixmaps/black.xpm \
	data/pixmaps/blackwhite.xpm \
	data/pixmaps/closed_folder_readonly.xpm \
	data/pixmaps/closed_folder_unreadable.xpm \
	data/pixmaps/closed_folder.xpm \
	data/pixmaps/first_letter_uppercase.xpm \
	data/pixmaps/first_letter_uppercase_word.xpm \
	data/pixmaps/forbidden.xpm \
	data/pixmaps/freedb.xpm \
	data/pixmaps/gnudb.xpm \
	data/pixmaps/grab.xpm \
	data/pixmaps/invert_selection.xpm \
	data/pixmaps/mask.xpm \
	data/pixmaps/musicbrainz.xpm \
	data/pixmaps/opened_folder.xpm \
	data/pixmaps/parent_folder.xpm \
	data/pixmaps/ram_disk.xpm \
	data/pixmaps/read_only.xpm \
	data/pixmaps/red_lines.xpm \
	data/pixmaps/sequence_track.xpm \
	data/pixmaps/sound.xpm \
	data/pixmaps/unselect_all.xpm

iconthemedir = $(datadir)/icons/hicolor

appicon16dir = $(iconthemedir)/16x16/apps
appicon32dir = $(iconthemedir)/32x32/apps
appicon48dir = $(iconthemedir)/48x48/apps
appicon64dir = $(iconthemedir)/64x64/apps
appicon128dir = $(iconthemedir)/128x128/apps
appicon256dir = $(iconthemedir)/256x256/apps
appiconscalabledir = $(iconthemedir)/scalable/apps

dist_appicon16_DATA = \
	data/icons/16x16/easytag.png
dist_appicon32_DATA = \
	data/icons/32x32/easytag.png
dist_appicon48_DATA = \
	data/icons/48x48/easytag.png
dist_appicon64_DATA = \
	data/icons/64x64/easytag.png
dist_appicon128_DATA = \
	data/icons/128x128/easytag.png
dist_appicon256_DATA = \
	data/icons/256x256/easytag.png
dist_appiconscalable_DATA = \
	data/icons/scalable/easytag.svg

UPDATE_ICON_CACHE = gtk-update-icon-cache --ignore-theme-index --force

install-data-hook: install-update-icon-cache
uninstall-hook: uninstall-update-icon-cache

install-update-icon-cache:
	$(AM_V_at)$(POST_INSTALL)
	test -n "$(DESTDIR)" || $(UPDATE_ICON_CACHE) "$(iconthemedir)"

uninstall-update-icon-cache:
	$(AM_V_at)$(POST_UNINSTALL)
	test -n "$(DESTDIR)" || $(UPDATE_ICON_CACHE) "$(iconthemedir)"

dist_doc_DATA = \
	HACKING \
	THANKS \
	TODO \
	USERS-GUIDE

dist_noinst_DATA = \
	$(desktop_in_files) \
	$(easytag_headers) \
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
	src/win32/win_easytag.c \
	data/icons/win32/easytag.ico \
	data/nsis/easytag-header.bmp \
	data/nsis/easytag-install.bmp \
	data/nsis/easytag-install.ico \
	data/nsis/easytag-uninstall.bmp \
	$(pixmaps) \
	\
	easytag.spec \
	easytag-win32-installer.nsi.in \
	\
	README_win32.txt \
	\
	doc/easytag.1.in \
	doc/EasyTAG_Documentation.html \
	doc/EasyTAG_Documentation_fr.html \
	doc/EasyTAG_Documentation_sv.html \
	doc/users_guide_french/USERS-GUIDE-11_fr1.html \
	doc/users_guide_french/USERS-GUIDE-11_fr2.html \
	doc/users_guide_french/USERS-GUIDE-11_fr3.html \
	doc/users_guide_french/USERS-GUIDE-11_fr4.html \
	doc/users_guide_french/USERS-GUIDE-11_fr5.html \
	doc/users_guide_french/USERS-GUIDE-11_fr6.html \
	doc/users_guide_french/USERS-GUIDE.css \
	doc/users_guide_french/images/bouton_arbo.png \
	doc/users_guide_french/images/bouton_img1.png \
	doc/users_guide_french/images/bouton_img2.png \
	doc/users_guide_french/images/bouton_img3.png \
	doc/users_guide_french/images/bouton_scan1.png \
	doc/users_guide_french/images/bouton_scan2.png \
	doc/users_guide_french/images/bouton_scan3.png \
	doc/users_guide_french/images/bouton_scan4.png \
	doc/users_guide_french/images/bouton_scan5.png \
	doc/users_guide_french/images/bouton_scan6.png \
	doc/users_guide_french/images/bouton_scan7.png \
	doc/users_guide_french/images/fen_boutons.png \
	doc/users_guide_french/images/fen_cddb1.png \
	doc/users_guide_french/images/fen_cddb2.png \
	doc/users_guide_french/images/fen_fich.png \
	doc/users_guide_french/images/fen_fic_txt.png \
	doc/users_guide_french/images/fen_lect1.png \
	doc/users_guide_french/images/fen_lect2.png \
	doc/users_guide_french/images/fen_playlist.png \
	doc/users_guide_french/images/fen_rech1.png \
	doc/users_guide_french/images/fen_scan1.png \
	doc/users_guide_french/images/fen_scan2.png \
	doc/users_guide_french/images/fen_scan3.png \
	doc/users_guide_french/images/fen_scan4.png \
	doc/users_guide_french/images/fen_sel1.png \
	doc/users_guide_french/images/fen_sel2.png \
	doc/users_guide_french/images/fen_sel3.png \
	doc/users_guide_french/images/fen_tag_bout1.png \
	doc/users_guide_french/images/fen_tag_bout2.png \
	doc/users_guide_french/images/fen_tag_gen.png \
	doc/users_guide_french/images/fen_tag_img.png \
	doc/users_guide_french/images/interface.png \
	doc/users_guide_french/images/menu_context1.png \
	doc/users_guide_french/images/menu_context2.png \
	doc/users_guide_french/images/menu_context3.png \
	doc/users_guide_french/images/menu_div.png \
	doc/users_guide_french/images/menu_fic.png \
	doc/users_guide_french/images/menu_pref.png \
	doc/users_guide_french/images/menu_recherche.png \
	doc/users_guide_french/images/menu_recherche1.png \
	doc/users_guide_french/images/menu_recherche2.png \
	doc/users_guide_french/images/menu_scan.png \
	doc/users_guide_french/images/menu_sel.png \
	doc/users_guide_french/images/sousmenu_fic1.png \
	doc/users_guide_french/images/sousmenu_fic2.png \
	doc/users_guide_german/bedienung1.htm \
	doc/users_guide_german/bedienung2.htm \
	doc/users_guide_german/easy.css \
	doc/users_guide_german/einleitung.htm \
	doc/users_guide_german/images/EasyTAG.png \
	doc/users_guide_german/images/eigensch.png \
	doc/users_guide_german/images/filelist.png \
	doc/users_guide_german/images/playlist.png \
	doc/users_guide_german/images/search.png \
	doc/users_guide_german/images/t_easytag.jpg \
	doc/users_guide_german/images/treeview.png \
	doc/id3/easy.html \
	doc/id3/id3v2.3.0.html \
	doc/id3/id3v2_blocks.gif \
	doc/id3/mpeghdr-19991222.htm \
	doc/vorbis/v-comment.html \
	doc/vorbis/white-ogg.png

dist_noinst_SCRIPTS = \
	autogen.sh

CLEANFILES = \
	$(desktop_DATA) \
	easytag-win32-installer.nsi

DISTCLEANFILES = \
	$(man_MANS) \
	intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache

git-changelog-hook:
	$(AM_V_at)if $(top_srcdir)/build-aux/missing --run git \
		--git-dir=$(top_srcdir)/.git --work-tree=$(top_srcdir) log \
		--stat -M -C --name-status --no-color \
		| fmt --split-only >.ChangeLog.tmp; \
	then mv -f .ChangeLog.tmp "$(top_distdir)/ChangeLog"; \
	else rm -f .ChangeLog.tmp; exit 1; fi

dist-hook: git-changelog-hook

.PHONY: git-changelog-hook
