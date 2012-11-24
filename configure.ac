dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])

dnl To set also in config.h.mingw
dnl defines ($PACKAGE_NAME, $PACKAGE_VERSION, etc.)
AC_INIT([EasyTAG], [2.1.8], [easytag-mailing@lists.sourceforge.net], [easytag], [http://easytag.sourceforge.net/])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/easytag.h])
AC_CONFIG_MACRO_DIR([m4])

dnl -------------------------------
dnl Init automake
dnl -------------------------------
AM_INIT_AUTOMAKE([1.11 -Wall no-define dist-xz no-dist-gzip foreign nostdinc subdir-objects])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

dnl -------------------------------
dnl Translation files.
dnl -------------------------------
ALL_LINGUAS="bg cs da de el es fr he hu it ja lt nl pl pt_BR ro ru sr sr@Latn sv uk zh_CN zh_TW"
AC_SUBST([GETTEXT_PACKAGE], [easytag])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Package name for gettext])

dnl -------------------------------
dnl Checks for programs.
dnl -------------------------------
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXXCPP
LT_PREREQ([2.2])
LT_INIT([disable-shared])
PKG_PROG_PKG_CONFIG([0.24])

dnl -------------------------
dnl Handle mingw32 or cygwin
dnl -------------------------
AC_MINGW32
AC_CYGWIN

dnl -------------------------------
dnl Configure switches.
dnl -------------------------------
AC_ARG_ENABLE([mp3],
              [AS_HELP_STRING([--disable-mp3], [Disable support for MP3 files (default=auto)])])

AC_ARG_ENABLE([id3v23],
              [AS_HELP_STRING([--disable-id3v23], [Disable support for writing id3v2.3 tags (default=enabled)])])

AC_ARG_ENABLE([ogg],
              [AS_HELP_STRING([--disable-ogg], [Disable support for Ogg Vorbis files (default=auto)])])

AC_ARG_ENABLE([speex],
              [AS_HELP_STRING([--disable-speex], [Disable support for Ogg Speex files (default=auto)])])

AC_ARG_ENABLE([flac],
              [AS_HELP_STRING([--disable-flac], [Disable support for FLAC files (default=auto)])])

AC_ARG_ENABLE([mp4],
              [AS_HELP_STRING([--disable-mp4], [Disable support for MP4 files (default=auto)])])

AC_ARG_ENABLE([wavpack],
              [AS_HELP_STRING([--disable-wavpack], [Disable support for Wavpack files (default=auto)])])


dnl -------------------------------
dnl Checks for libraries.
dnl -------------------------------

dnl GTK+-2 libraries
PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.12.0 glib-2.0 >= 2.8.0])


dnl Patch from Ben Taylor to fix a Solaris bug (for CDDB)
AC_CHECK_LIB([socket], [recv])


dnl ################################################
dnl # Ogg Vorbis libraries (rq : needed also for libFLAC)
dnl ################################################

OGG_DEPS="ogg >= 1.0 vorbis >= 1.0.1 vorbisfile"
AS_IF([test "x$enable_ogg" != "xno" -o "x$enable_flac" != "xno"],
      [PKG_CHECK_MODULES([OGG], [$OGG_DEPS], [have_ogg=yes], [have_ogg=no])],
      [have_ogg=no])

AS_IF([test "x$have_ogg" != "xno"],
      [AC_DEFINE([ENABLE_OGG], [], [Define for Ogg Vorbis support])],
      [AS_IF([test "x$enable_ogg" = "xyes"],
             [AC_MSG_ERROR([OGG Vorbis support requested but required dependencies ($OGG_DEPS) not found])])])

dnl ################################################
dnl # libSpeex library
dnl ################################################
dnl check for system libspeex
dnl Note that libvorbis is required for speex support, even if the user chooses --disable-ogg

SPEEX_DEPS="speex"
AS_IF([test "x$enable_speex" != "xno"],
      [PKG_CHECK_MODULES([SPEEX], [$SPEEX_DEPS], [have_speex=yes], [have_speex=no])],
      [have_speex=no])

AS_IF([test "x$have_speex" != "xno"],
      [AC_DEFINE([ENABLE_SPEEX], [], [Define for Speex support])],
      [AS_IF([test "x$enable_speex" = "xyes"],
             [AC_MSG_ERROR([OGG Speex support requested but required dependencies ($SPEEX_DEPS) not found])])])

dnl ################################################
dnl # libFLAC library
dnl ################################################
dnl check for system libflac
dnl Note that libvorbis is required for flac support, even if the user chooses --disable-ogg

FLAC_DEPS="flac >= 1.1.0"
AS_IF([test "x$enable_flac" != "xno"],
      [PKG_CHECK_MODULES([FLAC], [$FLAC_DEPS], [have_flac=yes], [have_flac=no])],
      [have_flac=no])

AS_IF([test "x$have_flac" != "xno"],
      [AC_DEFINE([ENABLE_FLAC], [], [Define for FLAC support])],
      [AS_IF([test "x$enable_flac" = "xyes"],
             [AC_MSG_ERROR([FLAC support requested but required dependencies ($FLAC_DEPS) not found])])])

dnl ################################################
dnl # libid3tag and Id3lib libraries
dnl ################################################
ID3TAG_DEPS="id3tag"
AS_IF([test "x$enable_mp3" != "xno"],
      [PKG_CHECK_MODULES([ID3TAG], [$ID3TAG_DEPS], [have_mp3=yes], [have_mp3=no])],
      [have_mp3=no])

AS_IF([test "x$have_mp3" != "xno"],
      [AC_DEFINE([ENABLE_MP3], [], [Define for MP3 support])],
      [AS_IF([test "x$enable_mp3" = "xyes"],
             [AC_MSG_ERROR([MP3 support requested but required dependencies ($ID3TAG_DEPS) not found])])])

AS_IF([test "x$have_mp3" = "xyes" -a "x$enable_id3v23" != "xno"],
      dnl Check which libs are required by id3lib, libid3.la is fucked up
      [LIBS_SAVE="$LIBS"
       AC_SEARCH_LIBS([ID3Tag_Link], ["id3" "id3 -lstdc++" "id3 -lz" "id3 -lz -lstdc++"], [have_id3lib=yes], [have_id3lib=no])
        
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
       LIBS="$LIBS_SAVE"
       AC_DEFINE_UNQUOTED([ID3LIB_MAJOR], [$ID3LIB_MAJOR], [id3lib major version])
       AC_DEFINE_UNQUOTED([ID3LIB_MINOR], [$ID3LIB_MINOR], [id3lib minor version])
       AC_DEFINE_UNQUOTED([ID3LIB_PATCH], [$ID3LIB_PATCH], [id3lib patch level])

       AC_SUBST([ID3LIB_LIBS], [$ac_cv_search_ID3Tag_Link])
       AC_DEFINE([ENABLE_ID3LIB], [], [Define for ID3v2.3 support])
       ID3LIB_VERSION="(id3lib-$ID3LIB_MAJOR.$ID3LIB_MINOR.$ID3LIB_PATCH)"])


dnl ################################################
dnl # libmp4v2 library
dnl ################################################

AS_IF([test "x$enable_mp4" != "xno"],
      [AC_CHECK_HEADER([mp4v2/mp4v2.h], [mp4_header=yes], [mp4_header=no])
       AC_CHECK_LIB([mp4v2], [MP4GetTrackMediaDataName], [mp4_lib=yes], [mp4_lib=no])
       AS_IF([test "x$mp4_header" = "xyes" -a "x$mp4_lib" = "xyes"],
             [have_mp4=yes
              AC_DEFINE([ENABLE_MP4], [], [Define for MP4 support])
              AC_SUBST([LIBMP4V2_LIBS], [-lmp4v2])],
             [have_mp4=no])],
      [have_mp4=no])

AS_IF([test "x$have_mp4" = "xyes"],
      [AC_DEFINE([ENABLE_MP4], [], [Define for MP4 support])
       AC_SUBST([LIBMP4V2_LIBS], [-lmp4v2])])

dnl ################################################
dnl # WavPack library
dnl ################################################
WAVPACK_DEPS="wavpack >= 4.40"
AS_IF([test "x$enable_wavpack" != "xno"],
      [PKG_CHECK_MODULES([WAVPACK], [$WAVPACK_DEPS], [have_wavpack=yes], [have_wavpack=no])],
      [have_wavpack=no])

AS_IF([test "x$have_wavpack" != "xno"],
      [AC_DEFINE([ENABLE_WAVPACK], [], [Define for Wavpack support])],
      [AS_IF([test "x$enable_wavpack" = "xyes"],
             [AC_MSG_ERROR([Wavpack support requested but required dependencies ($WAVPACK_DEPS) not found])])])

dnl Use -Wall if we have gcc.
AS_IF([test "x$GCC" = "xyes"],
  [AC_SUBST([WARN_CPPFLAGS], [-Wall])])

dnl To enable 'gprof' profiling
dnl CFLAGS="$CFLAGS -pg"


dnl -------------------------------
dnl Checks for header files.
dnl -------------------------------
AC_HEADER_DIRENT


dnl -------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.
dnl -------------------------------
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T


dnl -------------------------------
dnl Checks for library functions.
dnl -------------------------------
AC_FUNC_FORK
AC_CHECK_FUNCS(mkdir strstr strtol mkstemp)
AC_SEARCH_LIBS([strerror], [cposix])


dnl -------------------------------
dnl check for NLS
dnl -------------------------------
AM_GLIB_GNU_GETTEXT
dnl AM_GNU_GETTEXT_VERSION(0.10.40)



AC_CONFIG_FILES([ Makefile
                  easytag.spec
                  doc/easytag.1
                  po/Makefile.in
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
echo Linker ..................: $CC $LDFLAGS $LIBS
echo GTK2 version ............: `pkg-config --modversion gtk+-2.0`
echo MP3 file support ........: $have_mp3
echo ID3v2.3 tags support ....: $have_id3v23 $ID3LIB_VERSION
echo Ogg Vorbis file support .: $have_ogg
echo Ogg Speex file support ..: $have_speex
echo FLAC file support .......: $have_flac
echo MP4 file support ........: $have_mp4
echo WavPack support .........: $have_wavpack
echo NLS/gettext .............: $USE_NLS
echo Install prefix ..........: $prefix
echo 
echo Now type 'make' to build $PACKAGE_NAME $PACKAGE_VERSION,
echo and then 'make install' for installation.
echo
