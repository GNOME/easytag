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
AC_PROG_MAKE_SET
LT_PREREQ([2.2])
LT_INIT([disable-shared])
AC_ISC_POSIX
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
if test "x$enable_mp3" = "xyes"; then
    AC_MSG_CHECKING(for MP3 file support)
    AC_CHECK_HEADER(id3tag.h, [], [libid3tag_available=no])
    AC_CHECK_LIB(id3tag, id3_frame_field, [], [libid3tag_available=no], [-lz])
    if test "x$libid3tag_available" = "xno"; then
        AC_MSG_RESULT(no)
        enable_mp3="no"
        echo "***"
        echo "*** Warning: MP3 file support disabled, libid3tag missing"
        echo "***"
    else
        AC_MSG_RESULT(yes)
        AC_DEFINE(ENABLE_MP3,1,[Define for MP3 support])
    fi

    if test "x$enable_id3v23" = "xyes"; then
        dnl Check which libs are required by id3lib, libid3.la is fucked up
        AC_SEARCH_LIBS(ID3Tag_Link,"id3" "id3 -lstdc++" "id3 -lz" "id3 -lz -lstdc++",,AC_MSG_WARN(id3lib not found); id3lib_available=no)
         
        dnl Needed to link correctly with id3lib 3.8.0
        LIBS="-lz -lstdc++ $LIBS"
        dnl AC_CHECK_LIB(z,uncompress,AC_DEFINE_UNQUOTED(HAVE_ZLIB))
        
        dnl expected version for cross compiling
        ID3LIB_MAJOR=3
        ID3LIB_MINOR=8
        ID3LIB_PATCH=0
        
        AC_MSG_CHECKING(for id3lib version)
              AC_TRY_RUN([
        #include <id3.h>
        #include <stdio.h>
        int main ()
        {
            FILE *output;
            output=fopen("conftest.id3","w");
            fprintf(output,"ID3LIB_MAJOR=%d\nID3LIB_MINOR=%d\nID3LIB_PATCH=%d\n",ID3LIB_MAJOR_VERSION,ID3LIB_MINOR_VERSION,ID3LIB_PATCH_VERSION);
            fclose(output);
            exit(0);
        }
        ], . ./conftest.id3; echo "${ID3LIB_MAJOR}.${ID3LIB_MINOR}.${ID3LIB_PATCH}", AC_MSG_WARN(could not determine id3lib version); id3lib_available=no,[echo $ac_n "cross compiling; assuming ${ID3LIB_MAJOR}.${ID3LIB_MINOR}.${ID3LIB_PATCH} $ac_c"])
        AC_DEFINE_UNQUOTED(ID3LIB_MAJOR, $ID3LIB_MAJOR, [id3lib major version])
        AC_DEFINE_UNQUOTED(ID3LIB_MINOR, $ID3LIB_MINOR, [id3lib minor version])
        AC_DEFINE_UNQUOTED(ID3LIB_PATCH, $ID3LIB_PATCH, [id3lib patch level])
        
        AC_MSG_CHECKING(for MP3 file support)
        if test "x$id3lib_available" = "xno"; then
            AC_MSG_RESULT(no)
            echo "***"
            echo "*** Warning: id3v23 tags support disabled, id3lib missing"
            echo "***"
        else
            AC_MSG_RESULT(yes)
            AC_DEFINE(ENABLE_ID3LIB,1,[Define for id3v23 support])
            ID3LIB_VERSION="(id3lib-$ID3LIB_MAJOR.$ID3LIB_MINOR.$ID3LIB_PATCH)"
        fi
    fi
fi


dnl ################################################
dnl # libmp4v2 library
dnl ################################################
if test "x$enable_mp4" = "xyes"; then
    dnl Librairies required for mp4 files, if not found 'enable_mp4' is disabled
    AC_MSG_CHECKING(for MP4 file support)
    AC_CHECK_HEADER(mp4v2/mp4v2.h, [], [mp4_available=no])
    AC_CHECK_LIB(mp4v2, MP4GetTrackMediaDataName, [], [mp4_available=no], [-lmp4v2])
fi

AC_MSG_CHECKING(for MP4/AAC file support)
if test "x$enable_mp4" = "xyes"; then
    if test "x$mp4_available" = "xno"; then
        AC_MSG_RESULT(no)
        enable_mp4="no"
        echo "***"
        echo "*** Warning: MP4 file support disabled"
        echo "*** (Install libmp4v2 >= 1.9.0 to enable it)"
        echo "***"
    else

        dnl Check version of mp4v2 installed
        LIBS_SAVE="$LIBS"
        CFLAGS_SAVE="$CFLAGS"
        LIBS="$LIBS $LIBMP4V2_LIBS"
        CFLAGS="$CFLAGS $LIBMP4V2_CFLAGS"
        AC_MSG_CHECKING(for libmp4v2 version)
            AC_TRY_RUN([
                #include <stdio.h>
                #include <mp4v2/mp4v2.h>
                int main (void)
                {
                    FILE *output;
                    output=fopen("conftest.mp4v2","w");
                    fprintf(output,"MP4V2_PROJECT_name=%s\nMP4V2_PROJECT_version=%s\n",PACKAGE_TARNAME,PACKAGE_VERSION);
                    fclose(output);
                    exit(0);
                }
            ], . ./conftest.mp4v2; echo "${LIBMP4V2_NAME}-${LIBMP4V2_VERSION}", AC_MSG_WARN(could not determine libmp4v2 version),[echo $ac_n "cross compiling; assuming ${LIBMP4V2_NAME}-${LIBMP4V2_VERSION} $ac_c"])
        LIBS="$LIBS_SAVE"
        CFLAGS="$CFLAGS_SAVE"
        if test "$LIBMP4V2_VERSION" != ""; then
            LIBMP4V2_VERSION="($LIBMP4V2_NAME-$LIBMP4V2_VERSION)"
        fi

        dnl Version 1.6 of libmp4v2 introduces an index argument for MP4GetMetadataCoverart. So we define 'NEWMP4' if it is the case
        AC_COMPILE_IFELSE([
           #include <mp4v2/mp4v2.h>
           main () {
           MP4FileHandle hFile;
           MP4GetMetadataCoverArt(hFile, NULL, NULL,0); }
        ], AC_DEFINE(NEWMP4,1,[Define for MP4 lib with multiple image support]),)

        AC_MSG_RESULT(yes)
        AC_DEFINE(ENABLE_MP4,1,[Define for MP4 support])
    fi
else
    AC_MSG_RESULT(no)
    enable_mp4="no"
    echo "***"
    echo "*** MP4 file support disabled on commandline"
    echo "*** (Install libmp4v2 and use --enable-mp4 to enable it)"
    echo "***"
fi


dnl ################################################
dnl # WavPack library
dnl ################################################
if test "x$enable_wavpack" = "xyes"; then
    dnl Librairies required for wavpack files, if not found 'enable_wavpack' is disabled
    PKG_CHECK_MODULES(WAVPACK, wavpack >= 4.40, , wavpack_available=no)
    if test "x$wavpack_available" = "xno"; then
        echo "***"
        echo "*** Warning: wavpack >= 4.40 needed"
        echo "***"
    fi
    AC_CHECK_LIB(m, cos)
    AC_CHECK_LIB(wavpack, WavpackOpenFileInput, , wavpack_available=no)
fi

AC_MSG_CHECKING(for Wavpack file support)
if test "x$enable_wavpack" = "xyes"; then
    if test "x$wavpack_available" = "xno"; then
        AC_MSG_RESULT(no)
        enable_wavpack="no"
        echo "***"
        echo "*** Warning: Wavpack file support disabled"
        echo "*** (Install libwavpack to enable it)"
        echo "***"
    else
        AC_MSG_RESULT(yes)
        AC_DEFINE(ENABLE_WAVPACK,1,[Define for Wavpack support])
    fi
else
    AC_MSG_RESULT(no)
    enable_wavpack="no"
    echo "***"
    echo "*** Wavpack file support disabled on commandline"
    echo "*** (Install libwavpack, and use --enable-wavpack to enable it)"
    echo "***"
fi

dnl Use -Wall if we have gcc.
AS_IF([test "x$GCC" = "xyes"],
  [AC_SUBST([WARN_CPPFLAGS], [-Wall])])

dnl To enable 'gprof' profiling
dnl CFLAGS="$CFLAGS -pg"


dnl -------------------------------
dnl Checks for header files.
dnl -------------------------------
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h unistd.h)


dnl -------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.
dnl -------------------------------
AC_C_CONST
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM


dnl -------------------------------
dnl Checks for library functions.
dnl -------------------------------
AC_FUNC_FORK
AC_FUNC_STAT
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(mkdir strstr strtol mkstemp)


dnl -------------------------------
dnl check for NLS
dnl -------------------------------
AM_GLIB_GNU_GETTEXT
dnl AM_GNU_GETTEXT_VERSION(0.10.40)



AC_CONFIG_FILES([ Makefile
                  easytag.spec
                  doc/easytag.1
                  src/Makefile
                  src/id3lib/Makefile
                  src/libapetag/Makefile
                  src/libmpg123/Makefile
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
echo MP3 file support ........: $enable_mp3
echo ID3v2.3 tags support ....: $enable_id3v23 $ID3LIB_VERSION
echo Ogg Vorbis file support .: $have_ogg
echo Ogg Speex file support ..: $have_speex
echo FLAC file support .......: $have_flac
echo MP4 file support ........: $enable_mp4 $LIBMP4V2_VERSION
echo WavPack support .........: $enable_wavpack
echo NLS/gettext .............: $USE_NLS
echo Install prefix ..........: $prefix
echo 
echo Now type 'make' to build $PACKAGE_NAME $PACKAGE_VERSION,
echo and then 'make install' for installation.
echo
