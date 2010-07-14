AUTOMAKE_OPTIONS=dist-bzip2
 
SUBDIRS=src po

gnome_menudir=$(datadir)/applications
gnome_menu_DATA=easytag.desktop

textdir = $(datadir)/$(PACKAGE)
text_DATA = ChangeLog

man_MANS = doc/easytag.1

BITMAPS = \
    pixmaps/add.xpm \
    pixmaps/add_folder.xpm \
    pixmaps/all_downcase.xpm \
    pixmaps/all_uppercase.xpm \
    pixmaps/artist_album.xpm \
    pixmaps/black.xpm \
    pixmaps/blackwhite.xpm \
    pixmaps/closed_folder_locked.xpm \
    pixmaps/closed_folder.xpm \
    pixmaps/EasyTAG.ico \
    pixmaps/EasyTAG2.ico \
    pixmaps/EasyTAG.xpm \
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
pixmap_DATA = pixmaps/EasyTAG_logo.xpm

icondir = $(datadir)/pixmaps
icon_DATA = pixmaps/EasyTAG.xpm


EXTRA_DIST= \
    $(gnome_menu_DATA) \
    $(text_DATA) \
    $(BITMAPS) \
    $(pixmap_DATA) \
    $(icon_DATA) \
    \
    autogen.sh \
    easytag.spec \
    easytag.win32.setup.iss \
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
    doc/EasyTAG_Documentation.html \
    doc/EasyTAG_Documentation_fr.html \
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
    doc/users_guide_french/images/fen_fic_txt.png \
    doc/users_guide_french/images/fen_fich.png \
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
    doc/users_guide_french/images/menu_recherche1.png \
    doc/users_guide_french/images/menu_recherche2.png \
    doc/users_guide_french/images/menu_recherche.png \
    doc/users_guide_french/images/menu_scan.png \
    doc/users_guide_french/images/menu_sel.png \
    doc/users_guide_french/images/sousmenu_fic1.png \
    doc/users_guide_french/images/sousmenu_fic2.png \
    doc/users_guide_german/bedienung1.htm \
    doc/users_guide_german/bedienung2.htm \
    doc/users_guide_german/easy.css \
    doc/users_guide_german/einleitung.htm\
    doc/users_guide_german/images/EasyTAG.png \
    doc/users_guide_german/images/filelist.png \
    doc/users_guide_german/images/playlist.png \
    doc/users_guide_german/images/t_easytag.jpg \
    doc/users_guide_german/images/eigensch.png \
    doc/users_guide_german/images/search.png \
    doc/users_guide_german/images/treeview.png \
    doc/id3/easy.html \
    doc/id3/id3v2.3.0.html \
    doc/id3/id3v2_blocks.gif \
    doc/id3/mpeghdr-19991222.htm \
    doc/vorbis/v-comment.html \
    doc/vorbis/white-ogg.png
