dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])

dnl To set also in config.h.mingw
dnl defines ($PACKAGE_NAME, $PACKAGE_VERSION, etc.)
AC_INIT([EasyTAG], [2.5.1], [https://gitlab.gnome.org/GNOME/easytag/-/issues/], [easytag], [https://wiki.gnome.org/Apps/EasyTAG])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/easytag.h])
AC_CONFIG_MACRO_DIR([m4])

dnl -------------------------------
dnl Init automake
dnl -------------------------------
AM_INIT_AUTOMAKE([1.11.2 -Wall -Wno-portability check-news no-define dist-xz no-dist-gzip nostdinc subdir-objects])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

dnl -------------------------------
dnl Init Libtool
dnl -------------------------------
LT_PREREQ([2.2])
LT_INIT([disable-static])

dnl -------------------------------
dnl Translation files.
dnl -------------------------------
IT_PROG_INTLTOOL([0.50.0])
AC_SUBST([GETTEXT_PACKAGE], [$PACKAGE_TARNAME])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Package name for gettext])
AS_IF([test "x$USE_NLS" != "xno"],
      [AC_DEFINE([ENABLE_NLS], [1], [Native Language support is enabled])])

dnl AppData
m4_ifdef([APPDATA_XML], [APPDATA_XML],
         [AC_MSG_ERROR([appdata-tools is required for appdata-xml.m4])])

dnl -------------------------------
dnl Checks for programs.
dnl -------------------------------
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_SED
PKG_PROG_PKG_CONFIG([0.24])

AX_GENERATE_CHANGELOG

AC_SUBST([GLIB_COMPILE_RESOURCES], [`$PKG_CONFIG --variable glib_compile_resources gio-2.0`])

AC_PATH_PROG([GDK_PIXBUF_PIXDATA], [gdk-pixbuf-pixdata])
AS_IF([test "x$ac_cv_path_GDK_PIXBUF_PIXDATA" = "x"],
      [AC_MSG_ERROR([gdk-pixbuf-pixdata is required for image resources])])

AC_MSG_CHECKING([for glib-mkenums script])
GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AS_IF([test "x$GLIB_MKENUMS" = "x"],
    [AC_MSG_ERROR([glib-mkenums not listed in glib-2.0 pkg-config file])],
    [AC_SUBST([GLIB_MKENUMS])
     AC_MSG_RESULT([$GLIB_MKENUMS])])

dnl Man page, generated with xsltproc from DocBook XML.
AC_ARG_ENABLE([man],
    [AS_HELP_STRING([--disable-man], [Disable building the man page])])

AS_IF([test "x$enable_man" != "xno"],
    [AC_PATH_PROG([XSLTPROC], [xsltproc])
     JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.4//EN],
         [DocBook XML DTD V4.4], [have_docbookdtd=yes], [have_docbookdtd=no])
     JH_CHECK_XML_CATALOG(
         [http://docbook.sourceforge.net/release/xsl/current/html/refentry.xsl],
         [DocBook XSL Stylesheets], [have_docbookxsl=yes],
         [have_docbookxsl=no])
     AS_IF([test "x$ac_cv_path_XSLTPROC" != "x" \
         -a "x$have_docbookdtd" = "xyes" -a "x$have_docbookxsl" = "xyes"],
         [have_manutils=yes],
         [AS_IF([test "x$enable_man" = "xyes"],
             [AC_MSG_ERROR(
                 [man page generation requested but required utilities were not found])])
          have_manutils=no])],
    [have_manutils=no])

AM_CONDITIONAL([ENABLE_MAN], [test "x$have_manutils" = "xyes"])

dnl Check for testing utilities.
AC_ARG_ENABLE([tests],
              [AS_HELP_STRING([--disable-tests], [Disable support for tests run during make check])])

AS_IF([test "x$enable_tests" != "xno"],
      [AC_PATH_PROG([DESKTOP_FILE_VALIDATE], [desktop-file-validate])
       AS_IF([test "x$ac_cv_path_DESKTOP_FILE_VALIDATE" != "xyes"],
             [have_desktop_file_validate=yes],
             [have_desktop_file_validate=no])
       AC_PATH_PROG([GTESTER], [gtester])
       AC_PATH_PROG([GTESTER_REPORT], [gtester-report])
       AS_IF([test "x$ac_cv_path_GTESTER" != "xyes" -a "x$ac_cv_path_GTESTER_REPORT" != "xyes"],
             [have_gtester=yes],
             [have_gtester=no])],
      [have_desktop_file_validate=no
       have_gtester=no])

AS_IF([test "x$have_desktop_file_validate" = "xyes" -a "x$have_gtester" = "xyes"],
      [testing_utilities=yes],
      [testing_utilities=no
       AS_IF([test "x$enable_tests" = "xyes"],
             [AC_MSG_ERROR([tests were requested but the required utilities were not found])])])

AM_CONDITIONAL([EASYTAG_ENABLE_TESTS], [test "x$testing_utilities" = "xyes"])

dnl Help.
m4_ifdef([YELP_HELP_INIT], [YELP_HELP_INIT],
         [AC_MSG_ERROR([yelp-tools is required to build EasyTAG help])])

AC_PATH_PROG([YELP], [yelp])
AS_IF([test "x$ac_cv_path_YELP" = "x"],
      [AC_MSG_WARN([Yelp is required at runtime for user help])])

dnl -------------------------------
dnl Handle cross-compiling with mingw32.
dnl -------------------------------
AC_CANONICAL_HOST
AC_MSG_CHECKING([whether to build with mingw32 for a Windows host])
AS_CASE([$host_os],
        [*mingw*], [windows_host=yes],
        [windows_host=no])
AC_MSG_RESULT([$windows_host])

AM_CONDITIONAL([WINDOWS_HOST], [test "x$windows_host" = "xyes"])

AS_IF([test "x$windows_host" = "xyes"],
      [AC_CHECK_TOOL([WINDRES], [windres])
       AS_IF([test -z "$WINDRES"],
             [AC_MSG_ERROR([windres is required when building for a Windows host])])])

dnl -------------------------------
dnl Configure switches.
dnl -------------------------------
AC_ARG_ENABLE([mp3],
              [AS_HELP_STRING([--disable-mp3], [Disable support for MP3 files (default=auto)])])

AC_ARG_ENABLE([id3v23],
              [AS_HELP_STRING([--disable-id3v23], [Disable support for writing id3v2.3 tags (default=enabled)])])

AC_ARG_ENABLE([ogg],
              [AS_HELP_STRING([--disable-ogg], [Disable support for Ogg Vorbis files (default=auto)])])

AC_ARG_ENABLE([opus],
              [AS_HELP_STRING([--disable-opus], [Disable support for Opus Vorbis files (default=auto)])])

AC_ARG_ENABLE([speex],
              [AS_HELP_STRING([--disable-speex], [Disable support for Ogg Speex files (default=auto)])])

AC_ARG_ENABLE([flac],
              [AS_HELP_STRING([--disable-flac], [Disable support for FLAC files (default=auto)])])

AC_ARG_ENABLE([mp4],
              [AS_HELP_STRING([--disable-mp4], [Disable support for MP4 files (default=auto)])])

AC_ARG_ENABLE([wavpack],
              [AS_HELP_STRING([--disable-wavpack], [Disable support for Wavpack files (default=auto)])])

AC_ARG_ENABLE([cddb],
              [AS_HELP_STRING([--disable-cddb], [Disable CDDB support (default=auto)])])
AC_ARG_ENABLE([nautilus_actions],
              [AS_HELP_STRING([--disable-nautilus-actions], [do not build the nautilus context menu actions (default=auto)])])



dnl -------------------------------
dnl Checks for libraries.
dnl -------------------------------

dnl ################################################
dnl # Ogg Vorbis libraries
dnl ################################################

OGG_DEPS="ogg >= 1.0 vorbis >= 1.0.1 vorbisfile"
AS_IF([test "x$enable_ogg" != "xno"],
      [PKG_CHECK_EXISTS([$OGG_DEPS], [have_ogg=yes], [have_ogg=no])],
      [have_ogg=no])

AS_IF([test "x$have_ogg" != "xno"],
      [AC_DEFINE([ENABLE_OGG], [], [Define for Ogg Vorbis support])],
      [OGG_DEPS=""
       AS_IF([test "x$enable_ogg" = "xyes"],
             [AC_MSG_ERROR([OGG Vorbis support requested but required dependencies ($OGG_DEPS) not found])])])

dnl ################################################
dnl # Opus libraries
dnl ################################################

OPUS_DEPS="opus >= 1.0 opusfile"
AS_IF([test "x$enable_opus" != "xno"],
      [PKG_CHECK_EXISTS([$OPUS_DEPS], [have_opus=yes], [have_opus=no])],
      [have_opus=no])

AS_IF([test "x$have_opus" != "xno"],
      [AS_IF([test "x$have_ogg" = "xno"],
             [AC_MSG_ERROR([Opus support requested but Ogg support disabled. Ogg support is required.])])
       AC_DEFINE([ENABLE_OPUS], [], [Define for Ogg Opus support])],
      [OPUS_DEPS=""
       AS_IF([test "x$enable_opus" = "xyes"],
             [AC_MSG_ERROR([Opus support requested but required dependencies ($OPUS_DEPS) not found])])])

dnl ################################################
dnl # libSpeex library
dnl ################################################

SPEEX_DEPS="speex"
AS_IF([test "x$enable_speex" != "xno"],
      [PKG_CHECK_EXISTS([$SPEEX_DEPS], [have_speex=yes], [have_speex=no])],
      [have_speex=no])

AS_IF([test "x$have_speex" != "xno"],
      [AS_IF([test "x$have_ogg" = "xno"],
             [AC_MSG_ERROR([Speex support requested but Ogg support disabled. Ogg support is required.])])
       AC_DEFINE([ENABLE_SPEEX], [], [Define for Speex support])],
      [SPEEX_DEPS=""
       AS_IF([test "x$enable_speex" = "xyes"],
             [AC_MSG_ERROR([OGG Speex support requested but required dependencies ($SPEEX_DEPS) not found])])])

dnl ################################################
dnl # libFLAC library
dnl ################################################

FLAC_DEPS="flac >= 1.1.4"
AS_IF([test "x$enable_flac" != "xno"],
      [PKG_CHECK_EXISTS([$FLAC_DEPS], [have_flac=yes], [have_flac=no])],
      [have_flac=no])

AS_IF([test "x$have_flac" != "xno"],
      [AC_DEFINE([ENABLE_FLAC], [], [Define for FLAC support])],
      [FLAC_DEPS=""
       AS_IF([test "x$enable_flac" = "xyes"],
             [AC_MSG_ERROR([FLAC support requested but required dependencies ($FLAC_DEPS) not found])])])

dnl ################################################
dnl # libid3tag and Id3lib libraries
dnl ################################################
ID3TAG_DEPS="id3tag"
AS_IF([test "x$enable_mp3" != "xno"],
      [PKG_CHECK_EXISTS([$ID3TAG_DEPS], [have_mp3=yes], [have_mp3=no])],
      [have_mp3=no])

AS_IF([test "x$have_mp3" != "xno"],
      [AC_DEFINE([ENABLE_MP3], [], [Define for MP3 support])],
      [ID3TAG_DEPS=""
       AS_IF([test "x$enable_mp3" = "xyes"],
             [AC_MSG_ERROR([MP3 support requested but required dependencies ($ID3TAG_DEPS) not found])])])

AS_IF([test "x$have_mp3" = "xyes" -a "x$enable_id3v23" != "xno"],
      dnl Check which libs are required by id3lib, libid3.la is fucked up
      [LIBS_SAVE="$LIBS"
       AC_LANG_PUSH([C++])
       AC_SEARCH_LIBS([ID3Tag_Link], ["id3" "id3 -lz"], [have_id3lib=yes], [have_id3lib=no])
        
       dnl expected version for cross compiling
       ID3LIB_MAJOR=3
       ID3LIB_MINOR=8
       ID3LIB_PATCH=0

       AC_MSG_CHECKING([the id3lib version])
       AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <id3.h>
#include <stdio.h>
]],                                   [[
    FILE *output;
    output=fopen("conftest.id3","w");
    fprintf(output,"ID3LIB_MAJOR=%d\nID3LIB_MINOR=%d\nID3LIB_PATCH=%d\n",ID3LIB_MAJOR_VERSION,ID3LIB_MINOR_VERSION,ID3LIB_PATCH_VERSION);
    fclose(output);
    return 0;
]])], [. ./conftest.id3; AC_MSG_RESULT([${ID3LIB_MAJOR}.${ID3LIB_MINOR}.${ID3LIB_PATCH}])], [AC_MSG_ERROR([could not determine id3lib version])], [echo $ac_n "cross compiling; assuming ${ID3LIB_MAJOR}.${ID3LIB_MINOR}.${ID3LIB_PATCH} $ac_c"])
       AC_LANG_POP([C++])
       LIBS="$LIBS_SAVE"
       AC_DEFINE_UNQUOTED([ID3LIB_MAJOR], [$ID3LIB_MAJOR], [id3lib major version])
       AC_DEFINE_UNQUOTED([ID3LIB_MINOR], [$ID3LIB_MINOR], [id3lib minor version])
       AC_DEFINE_UNQUOTED([ID3LIB_PATCH], [$ID3LIB_PATCH], [id3lib patch level])

       AC_SUBST([ID3LIB_LIBS], [$ac_cv_search_ID3Tag_Link])
       AC_DEFINE([ENABLE_ID3LIB], [], [Define for ID3v2.3 support])
       ID3LIB_VERSION="(id3lib-$ID3LIB_MAJOR.$ID3LIB_MINOR.$ID3LIB_PATCH)"
       AC_CHECK_DECLS([ID3Field_SetEncoding,
                       ID3Field_GetEncoding,
                       ID3Field_IsEncodable],
                       [], [],
                      [[#include <id3.h>]])],
      [have_id3lib=no])


dnl ################################################
dnl # taglib library
dnl ################################################

TAGLIB_DEPS="taglib >= 1.9.1"
AS_IF([test "x$enable_mp4" != "xno"],
      [PKG_CHECK_EXISTS([$TAGLIB_DEPS], [have_taglib=yes], [have_taglib=no])],
      [have_taglib=no])

AS_IF([test "x$have_taglib" != "xno"],
      [AC_DEFINE([ENABLE_MP4], [], [Define for taglib MP4 support])],
      [TAGLIB_DEPS=""
       AS_IF([test "x$enable_mp4" = "xyes"],
             [AC_MSG_ERROR([Taglib MP4 support requested but required dependencies ($TAGLIB_DEPS) not found])])])

dnl ################################################
dnl # WavPack library
dnl ################################################
WAVPACK_DEPS="wavpack >= 4.40"
AS_IF([test "x$enable_wavpack" != "xno"],
      [PKG_CHECK_EXISTS([$WAVPACK_DEPS], [have_wavpack=yes], [have_wavpack=no])],
      [have_wavpack=no])

AS_IF([test "x$have_wavpack" != "xno"],
      [AC_DEFINE([ENABLE_WAVPACK], [], [Define for Wavpack support])],
      [WAVPACK_DEPS=""
       AS_IF([test "x$enable_wavpack" = "xyes"],
             [AC_MSG_ERROR([Wavpack support requested but required dependencies ($WAVPACK_DEPS) not found])])])

dnl ################################################
dnl # CDDB (libsoup)
dnl ################################################
CDDB_DEPS="libsoup-2.4 >= 2.42"
AS_IF([test "x$enable_cddb" != "xno"],
      [PKG_CHECK_EXISTS([$CDDB_DEPS], [have_cddb=yes], [have_cddb=no])],
      [have_cddb=no])

AS_IF([test "x$enable_cddb" != "xno"],
      [AC_DEFINE([ENABLE_CDDB], [], [Define for CDDB support])],
      [CDDB_DEPS=""
       AS_IF([test "x$enable_cddb" = "xyes"],
             [AC_MSG_ERROR([CDDB support requested but required dependences ($CDDB_DEPS) not found])])])

dnl ################################################
dnl # Nautilus
dnl ################################################
NAUTILUS_DEPS="libnautilus-extension >= 3.0 gio-unix-2.0"
AS_IF([test "x$enable_nautilus_actions" != "xno"],
      [PKG_CHECK_EXISTS([$NAUTILUS_DEPS], [have_libnautilus_extension=yes], [have_libnautilus_extension=no])],
      [have_libnautilus_extension=no])

AS_IF([test "x$have_libnautilus_extension" = "xno"],
      [NAUTILUS_DEPS=""
       AS_IF([test "x$enable_nautilus_actions" = "xyes"],
             [AC_MSG_ERROR([Nautilus Actions build requested but required dependencies ($NAUTILUS_DEPS) not found])])],
      [PKG_CHECK_MODULES([NAUTILUS], [$NAUTILUS_DEPS])
       NAUTILUS_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libnautilus-extension`
       AC_SUBST([NAUTILUS_EXTENSION_DIR], [$NAUTILUS_EXTENSION_DIR])])

AM_CONDITIONAL([ENABLE_NAUTILUS_ACTIONS], [test x"$have_libnautilus_extension" != x"no"])

dnl Check the pkg-config dependencies
GIO_DEPS="gio-2.0 >= 2.40.0"
AC_SUBST([GLIB_DEPRECATION_FLAGS],
         ["-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_40 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_40"])
GTK_DEPS="gtk+-3.0 >= 3.14.0"
AC_SUBST([GTK_DEPRECATION_FLAGS],
         ["-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_14 -DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_14"])
AC_SUBST([SOUP_DEPRECATION_FLAGS],
         ["-DSOUP_VERSION_MIN_REQUIRED=SOUP_VERSION_2_42 -DSOUP_VERSION_MAX_ALLOWED=SOUP_VERSION_2_42"])
PKG_CHECK_MODULES([EASYTAG], [$CDDB_DEPS $GIO_DEPS $GTK_DEPS $OPUS_DEPS $OGG_DEPS $SPEEX_DEPS $FLAC_DEPS $ID3TAG_DEPS $TAGLIB_DEPS $WAVPACK_DEPS])

dnl Set warning flags
AX_IS_RELEASE([git-directory])
AX_COMPILER_FLAGS([WARN_CFLAGS], [WARN_LDFLAGS],
                  [$ax_is_release])

GLIB_GSETTINGS

AC_CONFIG_FILES([ Makefile
                  doc/version.xml
                  help/Makefile
                  po/Makefile.in
                  src/win32/easytag.rc
               ])
AC_OUTPUT

echo 
echo Configuration for $PACKAGE_NAME $PACKAGE_VERSION :
echo --------------------------------
echo 
echo Source code location ....: $srcdir
echo Host System Type ........: $host
echo Preprocessor ............: $CC $CPPFLAGS
echo Compiler ................: $CC $CFLAGS
echo Compiler warnings .......: $WARN_CFLAGS
echo C++ Compiler warnings ...: $WARN_CXXFLAGS
echo Linker ..................: $CC $LDFLAGS $LIBS
echo CDDB support ............: $have_cddb
echo MP3 file support ........: $have_mp3
echo ID3v2.3 tags support ....: $have_id3lib $ID3LIB_VERSION
echo Ogg Vorbis file support .: $have_ogg
echo Ogg Speex file support ..: $have_speex
echo Ogg Opus file support ...: $have_opus
echo FLAC file support .......: $have_flac
echo MP4 file support ........: $have_taglib
echo WavPack support .........: $have_wavpack
echo NLS/gettext .............: $USE_NLS
echo Nautilus actions ........: $have_libnautilus_extension
echo Tests during make check .: $testing_utilities
echo Install prefix ..........: $prefix
echo 
echo Now type 'make' to build $PACKAGE_NAME $PACKAGE_VERSION,
echo and then 'make install' for installation.
echo
