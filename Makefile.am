ACLOCAL_AMFLAGS = -I m4
 
SUBDIRS = src po

desktopdir = $(datadir)/applications
dist_desktop_DATA = easytag.desktop

textdir = $(pkgdatadir)
dist_text_DATA = ChangeLog

man_MANS = doc/easytag.1

BITMAPS = \
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
dist_icon_DATA = pixmaps/EasyTAG_icon.xpm


dist_noinst_DATA = \
    $(BITMAPS) \
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
