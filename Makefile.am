AUTOMAKE_OPTIONS=dist-bzip2
 
SUBDIRS=src po

gnome_menudir=$(datadir)/applications
gnome_menu_DATA=easytag.desktop

textdir = $(datadir)/$(PACKAGE)
text_DATA = ChangeLog

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

pixmapdir = $(datadir)/$(PACKAGE)
pixmap_DATA = \
    pixmaps/EasyTAG_logo.xpm
#    pixmaps/artist_album.png \
#    pixmaps/EasyTAG_icon.png \
#    pixmaps/EasyTAG_logo.png \
#    pixmaps/scan.png


icondir = $(datadir)/pixmaps
icon_DATA = pixmaps/EasyTAG_icon.xpm
#icon_DATA = pixmaps/EasyTAG_icon.png


EXTRA_DIST= \
    $(gnome_menu_DATA) \
    $(text_DATA) \
    $(BITMAPS) \
    $(pixmap_DATA) \
    $(icon_DATA) \
    \
    autogen.sh \
    easytag.spec \
    easytag-win32-installer.nsi \
    COPYING \
    INSTALL \
    README \
    THANKS \
    TODO \
    USERS-GUIDE \
    \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/dirs \
    debian/docs \
    debian/easytag.1 \
    debian/easytag.xpm \
    debian/menu \
    debian/rules \
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


